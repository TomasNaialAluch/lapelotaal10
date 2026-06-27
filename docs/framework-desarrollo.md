# Framework / Desarrollo — investigación

Notas de investigación sobre la base técnica del plugin: qué framework usar, cómo lograr una ventana circular, y cómo firmar/empaquetar el plugin para Windows y macOS.

## 1. VST3 SDK de Steinberg vs JUCE

### VST3 SDK (Steinberg, nativo)
- Acceso directo y completo a la API de VST3, sin capas intermedias.
- No trae un sistema de GUI propio: hay que integrar VSTGUI (la librería de UI que ofrece Steinberg) o dibujar a mano con las APIs nativas de cada plataforma (HWND/GDI/Direct2D en Windows, NSView/Core Graphics en macOS).
- Más control, pero todo el manejo cross-platform (ventanas, eventos de mouse, render) queda a cargo nuestro.
- Licencia dual: GPLv3 o licencia comercial de Steinberg (a pagar si no se quiere abrir el código).

### JUCE
- Framework de alto nivel en C++ que envuelve VST3 (y también AU, AAX, standalone) desde un único código fuente.
- Sistema propio de gráficos (`juce::Graphics`, `juce::Path`) que permite dibujar cualquier forma, incluida una circular, dentro del componente.
- Mucho menos boilerplate para compilar en Windows y macOS desde el mismo proyecto (vía CMake o Projucer).
- Punto importante a verificar: JUCE facilita dibujar una forma circular *dentro* de la ventana, pero la ventana en sí (el marco que entrega el host) sigue siendo rectangular — ver punto 2.
- Licencia dual: GPL o comercial (JUCE 8 tiene un esquema de licencia gratuita hasta cierto nivel de ingresos, a confirmar contra los términos vigentes al momento de decidir).

### Conclusión preliminar
JUCE es el camino más rápido para cross-platform real (un solo código, un solo build pipeline). El SDK nativo de Steinberg solo tendría sentido si JUCE termina siendo una limitación real para la UI circular o el procesamiento de audio (no debería serlo para esto). **Recomendación: arrancar con JUCE.**

## 2. Ventana circular (no rectangular) en Windows y macOS

Punto clave a entender antes de diseñar la UI: **en un plugin VST3, el host es quien entrega el marco/ventana contenedora** (el plugin no abre su propia ventana top-level como lo haría una app standalone). Esto cambia el problema:

- **Enfoque más viable ("circular visual", no circular real de OS):** dejar el fondo del componente transparente y dibujar solo el círculo con sus controles. El host muestra su marco rectangular alrededor, pero visualmente el contenido "útil" es un círculo. Es el enfoque que usan la mayoría de los plugins con UI redonda que existen en el mercado.
- **Enfoque "ventana realmente circular" (recorte a nivel de OS):**
  - Windows: existe `SetWindowRgn` para recortar la región visible de un HWND. Es aplicable al HWND hijo que el host nos da, pero hay que probar cómo se comporta dentro de cada DAW (algunos hosts redimensionan o redibujan ese HWND de formas que rompen el recorte).
  - macOS: se puede usar una `NSView` con `CALayer` y una máscara (`mask` con un `CAShapeLayer` en forma de círculo) sobre la vista que el host nos entrega.
  - Riesgo: el comportamiento de redimensionado, fondo y repintado varía por host (Ableton, Logic, Cubase, Reaper, FL Studio, etc.), así que cualquier recorte a nivel de OS hay que probarlo en varios hosts antes de asumir que funciona en todos.
- **Hit-testing (qué responde a clicks):** independientemente del enfoque visual, hay que asegurarse de que los clicks fuera del círculo no hagan nada. En JUCE esto se controla sobreescribiendo `hitTest()` del componente para que devuelva `false` fuera del área circular.

### A investigar/probar puntualmente
- Probar `SetWindowRgn` y `CAShapeLayer` mask dentro de un plugin JUCE real, en al menos 3 hosts distintos, antes de comprometerse con el enfoque de "ventana realmente recortada".
- Si el recorte por OS resulta poco confiable entre hosts, usar el enfoque de fondo transparente + dibujo de círculo (más simple y más portable).

## 3. Code signing y notarización en macOS

Requisito para que el plugin cargue sin warnings de Gatekeeper en macOS moderno (Catalina en adelante).

Pasos generales (a confirmar contra la documentación vigente de Apple al momento de implementar, porque las herramientas cambian):

1. Tener cuenta de **Apple Developer Program** (de pago, anual).
2. Generar un certificado **Developer ID Application** (para firmar el binario/plugin) y, si se va a distribuir como instalador `.pkg`, también **Developer ID Installer**.
3. Firmar el bundle `.vst3` (que en macOS es una carpeta/bundle, no un archivo único) con `codesign`, habilitando **hardened runtime** (`--options runtime`), firmando primero cualquier binario anidado y al final el bundle completo.
4. Enviar el bundle firmado al servicio de notarización de Apple (`xcrun notarytool submit`) y esperar el resultado.
5. "Graparle" (`staple`) el ticket de notarización al bundle (`xcrun stapler staple`) para que funcione incluso sin conexión a internet en la primera apertura.
6. Si el plugin carga librerías externas no firmadas por Apple, revisar entitlements de hardened runtime (puede haber que desactivar library validation, algo a evitar si es posible).

### A investigar puntualmente
- Confirmar el flujo actual de `notarytool` (reemplazó a `altool`, que quedó deprecado) contra la documentación oficial de Apple al momento de implementar.
- Definir si el `.pkg` instalador también necesita notarizarse por separado del binario (sí, normalmente sí).

## 4. Empaquetado / instalador

### Windows
- Herramientas típicas: **Inno Setup**, **NSIS** o **WiX Toolset**.
- Ruta estándar de instalación de VST3: `C:\Program Files\Common Files\VST3\`.
- Firmar el instalador/ejecutable con un certificado Authenticode es opcional pero recomendable para evitar warnings de SmartScreen (un certificado EV evita el warning directamente; uno estándar igual ayuda a la reputación con el tiempo).

### macOS
- Empaquetar con `pkgbuild` / `productbuild` para generar un `.pkg`.
- Firmar el `.pkg` con el certificado **Developer ID Installer** (distinto del usado para firmar el binario).
- Notarizar el `.pkg` igual que el binario (ver punto 3) y graparle el ticket.
- Rutas de instalación: `~/Library/Audio/Plug-Ins/VST3/` (solo el usuario actual) o `/Library/Audio/Plug-Ins/VST3/` (todo el sistema, requiere permisos de administrador desde el instalador).

### Pipeline integrado
- JUCE soporta generación de proyectos vía **CMake**, lo que permite automatizar build + (con `CPack` u otro paso) empaquetado en un mismo pipeline de CI para ambas plataformas, en vez de armar el instalador a mano en cada release.

### A investigar puntualmente
- Definir si conviene instalador único (Inno Setup/NSIS en Windows, pkg en Mac) o distribución sin instalador (copiar el `.vst3` manualmente) para las primeras versiones de prueba.
- Evaluar si vale la pena un certificado Authenticode en Windows desde el día uno o recién antes del lanzamiento público.

## Fuentes a revisar (sin URLs específicas, buscar la documentación oficial vigente)
- Documentación oficial de Steinberg sobre VST3 SDK (steinberg.net).
- Documentación oficial de JUCE (juce.com), en particular la sección de CMake y de `juce::Component`/`hitTest`.
- Documentación de Apple Developer sobre notarización y hardened runtime (developer.apple.com).
- Documentación de Inno Setup, NSIS y WiX Toolset para el instalador de Windows.
