# La Pelota al 10

VST3 (Windows + macOS) con interfaz circular. Es un procesador de masterización/mezcla todo-en-uno: entra una señal y sale saturada, con mejor curva tonal, sin auto-enmascaramiento y sin resonancias molestas, manteniendo un nivel de salida similar al de entrada.

## Idea general

El plugin combina, en una sola cadena automática:

1. **Saturador multibanda** con distinto carácter armónico por rango de frecuencia.
2. **EQ dinámico** post-saturación que reacomoda el balance espectral (estilo Bloom de oeksound).
3. **Compresor multibanda automático** (estilo Pro-MB de FabFilter) configurado solo, sin intervención manual.
4. **Auto-boost "Hi-Fi"**: el plugin detecta qué frecuencias están débiles y les da boost para lograr una curva tonal tipo Hi-Fi.
5. **Control de resonancias** al final de la cadena (estilo Soothe2 de oeksound).
6. **Matching de nivel de salida**: la salida mantiene un RMS o Peak similar al de entrada (según el tipo de señal), para que el procesamiento no se perciba como un cambio de volumen.

Objetivo final: el audio entra, sale saturado y con mejor curva, "respira" mejor (no se enmascara solo) y no tiene resonancias, sin que el usuario tenga que tocar nada.

## Cadena de señal (signal chain)

```
Entrada
  │
  ▼
[1] Saturador multibanda (4 bandas, carácter distinto por banda)
  │
  ▼
[2] EQ dinámico (balance espectral automático, tipo Bloom)
  │
  ▼
[3] Compresor multibanda automático (tipo Pro-MB, full auto)
  │
  ▼
[4] Auto-EQ "Hi-Fi" (boost de frecuencias débiles según curva objetivo)
  │
  ▼
[5] Control de resonancias (tipo Soothe2)
  │
  ▼
[6] Matching de loudness (RMS/Peak salida ≈ entrada)
  │
  ▼
Salida
```

## Detalle de cada etapa

### 1. Saturador multibanda

Cuatro bandas, cada una con un tipo de saturación distinto pensado para imitar comportamiento de gear analógico:

| Rango | Carácter |
|---|---|
| < 250 Hz | "Warm" (saturación suave, tipo consola/transformador) |
| 250 – 400 Hz | Tipo tubo (tube) |
| 400 – 2000 Hz | Tipo diodo o cinta (diode/tape) |
| 2000 – 20000 Hz | Tipo cinta/diodo (tape/diode) |

La idea es que cada banda tenga su propia curva de transferencia no lineal, no un único saturador aplicado a todo el espectro.

#### Control manual: perilla de Énfasis

Además del comportamiento automático de las 6 etapas, el plugin tiene **un control manual** sobre el saturador multibanda: una perilla de "Énfasis" que corre de izquierda a derecha.

- **Tirada a la izquierda:** pone el énfasis en el low end. Ajusta la curva de EQ acompañando ese énfasis, va desactivando progresivamente los saturadores que trabajan en las frecuencias altas, y deja que el carácter "warm" (el saturador pensado originalmente solo para < 250 Hz) se extienda a todo el sonido, no solo a la banda baja.
- **Tirada a la derecha:** comportamiento opuesto — énfasis en las frecuencias altas, los saturadores de baja frecuencia se van desactivando, y el carácter de las bandas altas (tape/diode) pasa a dominar el sonido completo.
- **Al centro:** comportamiento por defecto, cada banda satura con su carácter propio tal como está definido en la tabla de arriba, sin que ninguna domine sobre las demás.

El objetivo de esta perilla es que el mismo plugin sirva tanto para una línea de bajo (todo hacia la izquierda, énfasis grave/warm) como para un lead (posición intermedia) o un hi-hat/elemento de alta frecuencia (todo hacia la derecha, énfasis en agudos/tape-diode), sin tener que cambiar de plugin ni de preset según el tipo de fuente.

Es, en la práctica, un **control de mezcla/morph entre las 4 bandas de saturación** (no un control independiente nuevo): redistribuye cuánto pesa cada banda y qué tan extendido está su carácter sobre el resto del espectro, en vez de operar como una etapa separada de la cadena.

### 2. EQ dinámico (estilo Bloom)

Después de saturar, un EQ dinámico reacomoda el balance espectral en tiempo real, suavizando picos y valles que la saturación pudo introducir o acentuar, buscando una respuesta más pareja.

### 3. Compresor multibanda automático (estilo Pro-MB)

Multibanda con bandas, thresholds, ratios, ataque/release definidos automáticamente a partir del análisis de la señal de entrada (sin controles manuales expuestos al usuario, o con un único "modo auto").

### 4. Auto-boost "Hi-Fi"

Etapa de análisis + corrección: el plugin mide qué frecuencias están por debajo de una curva objetivo ("Hi-Fi") y aplica boost ahí, de forma automática y continua.

### 5. Control de resonancias (estilo Soothe2)

Última etapa antes del matching de nivel: detecta picos resonantes dinámicos (no fijos) y los atenúa ("aplasta") sin tocar el resto del espectro.

### 6. Matching de loudness

El plugin mide la señal de entrada y de salida (RMS o Peak, dependiendo del tipo de material/transientes) y ajusta la ganancia de salida para que el nivel percibido sea similar al de entrada, evitando que todo el procesamiento se confunda con "subir el volumen".

## Formato y plataforma

- **Formato:** VST3
- **Plataformas:** Windows y macOS
- **UI:** forma circular (no rectangular), a definir el mapeo de controles sobre esa forma.

## Qué hay que investigar

### Framework / desarrollo
- VST3 SDK de Steinberg vs JUCE (JUCE simplifica mucho cross-platform y tiene soporte de VST3, pero hay que ver flexibilidad para UI no rectangular).
- Cómo crear una ventana de plugin con forma circular (no rectangular) en Windows y en macOS: máscaras de ventana, transparencia, regiones clickeables vs zonas transparentes.
- Code signing y notarización en macOS (requisito para que el plugin cargue en hosts modernos sin warnings).
- Empaquetado/instalador para ambas plataformas.

### Saturación multibanda
- Diseño de filtros crossover (Linkwitz-Riley u otros) para separar las 4 bandas sin generar problemas de fase al recombinar.
- Modelado de saturación tipo tubo (curvas asimétricas, armónicos pares/impares, ej. modelo de Koren u otros modelos de válvulas).
- Modelado de saturación tipo diodo (clippers de diodo, ecuación de Shockley, knee duro vs suave).
- Modelado de saturación tipo cinta (histeresis, compresión de cinta, modelo Jiles-Atherton u otras aproximaciones más simples tipo tanh).
- Qué distingue sonoramente "warm" de "tube" en términos de DSP (esto hay que definirlo, no es un término técnico estándar).
- Necesidad de oversampling (2x/4x) en las bandas con saturación más agresiva para evitar aliasing, y su costo de CPU/latencia.

### EQ dinámico tipo Bloom
- Investigar cómo funciona conceptualmente Bloom (oeksound): balanceo espectral dinámico, no es un EQ dinámico tradicional banda por banda con threshold fijo.
- FFT-based vs banco de filtros para el análisis espectral en tiempo real.

### Multibanda automático
- Cómo determinar automáticamente la cantidad de bandas, los puntos de corte, thresholds y ratios a partir del contenido espectral de la señal (esto es más un problema de heurísticas/análisis de señal que de DSP puro).
- Confirmar la referencia: "MB de pro filters" probablemente apunta a FabFilter Pro-MB — confirmar si es ese el comportamiento a imitar.

### Auto-EQ "Hi-Fi"
- Definir qué es una "curva Hi-Fi" objetivo: ¿curva Harman, X-curve, una curva propia? Hay que elegir una referencia concreta.
- Cómo detectar enmascaramiento entre frecuencias (referencia: Soundtheory Gullfoss, que hace algo conceptualmente similar).

### Control de resonancias tipo Soothe2
- Investigar el enfoque de detección de picos resonantes dinámicos (no resonancias fijas) y cómo atenuarlos sin afectar el resto de la señal.
- Filtrado adaptativo / notch dinámico como posible enfoque.

### Loudness / matching de nivel
- Diferencia entre Peak, RMS y LUFS (estándar ITU-R BS.1770 / EBU R128).
- Cómo decidir automáticamente qué métrica usar "según el tipo de sonido" (¿factor de cresta para distinguir material transiente vs sostenido?).
- Suavizado/look-ahead para que la compensación de ganancia no genere "bombeo".

### Rendimiento y latencia
- Presupuesto de latencia total (oversampling + análisis FFT en varias etapas) y cómo reportarlo correctamente como Plugin Delay Compensation en VST3.
- Optimización para que las 6 etapas corran en tiempo real sobre múltiples pistas simultáneas (SIMD, multithreading).

### Validación
- Set de señales de prueba (sine sweeps, distintos géneros, voces, batería, mezclas completas) para validar cada etapa por separado y la cadena completa.

## Preguntas abiertas

- ¿"MB de pro filters" se refiere específicamente a FabFilter Pro-MB?
- ¿Cuántas bandas debería tener el compresor multibanda automático (independiente de las 4 bandas del saturador)?
- ¿La curva "Hi-Fi" es un estándar a elegir o algo a definir por oído/referencia propia?
- Confirmado: el plugin tiene al menos un control manual, la perilla de Énfasis (ver sección de saturador multibanda). Falta definir si habrá otros controles manuales además de esa, o si el resto de la cadena queda 100% automática.
- ¿Uso previsto: mezcla, mastering, o ambos? Esto afecta cuánta latencia es aceptable.
