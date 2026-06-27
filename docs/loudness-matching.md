# Loudness / matching de nivel — investigación

Notas de investigación sobre qué métrica de nivel usar para igualar la salida con la entrada, cómo decidir automáticamente cuál aplicar según el material, y cómo evitar artefactos de "bombeo" en la compensación de ganancia.

## 1. Diferencia entre Peak, RMS y LUFS

### Peak
- Mide el valor instantáneo más alto de la forma de onda (en muestra, o "true peak" si se mide sobremuestreado para detectar picos entre muestras que el peak normal a la sample rate original no ve).
- Muy sensible a transientes puntuales: un solo golpe de percusión puede definir el "peak" de todo el material, aunque el resto de la señal sea mucho más bajo en energía promedio.
- Útil para control de **headroom/clipping** (evitar distorsión digital), pero mal indicador de "qué tan fuerte se percibe" el sonido en general.

### RMS (Root Mean Square)
- Mide la energía promedio de la señal en una ventana de tiempo (raíz cuadrada del promedio del cuadrado de la señal).
- Mejor proxy de "energía sostenida" que el peak, pero sigue siendo una medida puramente matemática sobre la forma de onda — no tiene en cuenta cómo el oído humano percibe distintas frecuencias con distinta sensibilidad.
- Depende mucho del tamaño de la ventana de medición usada (una ventana muy corta se parece más a peak; una muy larga promedia demasiado y pierde resolución temporal).

### LUFS (Loudness Units Full Scale) — estándar ITU-R BS.1770 / EBU R128
- Es una medida de **loudness percibido**, no solo de energía de la señal: aplica un filtro de ponderación (K-weighting, que se acerca a la sensibilidad del oído humano a distintas frecuencias) antes de medir la energía, y define ventanas de medición estandarizadas:
  - **Momentary loudness:** ventana corta (~400 ms).
  - **Short-term loudness:** ventana de 3 segundos.
  - **Integrated loudness:** promedio de todo el programa/material, con un gate que excluye silencios y partes muy bajas (para no falsear el promedio con silencios).
- Es el estándar usado en broadcast (EBU R128) y cada vez más en plataformas de streaming musical para normalización de volumen entre distintas pistas/programas. Es, conceptualmente, la métrica más alineada con "cuánto se percibe de fuerte" un material, más que RMS o Peak puro.
- Requiere implementar el filtro K-weighting y la lógica de gating del estándar — no es tan trivial de implementar "a mano" como RMS o Peak, pero está completamente especificado en el estándar (ITU-R BS.1770), así que no hay ambigüedad de diseño, es una cuestión de implementación correcta.

### Conclusión para este proyecto
- **Peak** es necesario igual como salvaguarda (evitar clipping en la salida), independientemente de qué métrica se use para el matching de "volumen percibido".
- Para el objetivo de "que la salida se perciba con el mismo volumen que la entrada" (que es lo que se busca, no solo igualar números de energía), **LUFS (o al menos RMS con K-weighting aproximado) es conceptualmente más correcto que RMS plano**, porque tiene en cuenta percepción y no solo matemática de la forma de onda.
- Probable enfoque: usar **LUFS short-term/momentary** como base del matching continuo (ya que el plugin procesa en tiempo real, no se puede esperar a tener "todo el programa" para calcular integrated loudness), reservando Peak como límite de seguridad aparte.

### A investigar puntualmente
- Revisar la especificación completa de ITU-R BS.1770 (filtro K-weighting, fórmulas exactas) para implementar la medición de LUFS correctamente, no aproximada.
- Decidir qué ventana de LUFS (momentary vs short-term) usar como base del matching en tiempo real, considerando el trade-off entre reactividad (ventanas cortas) y estabilidad de la medición (ventanas más largas).

## 2. Decidir automáticamente qué métrica usar "según el tipo de sonido"

La idea original plantea que el plugin debería ajustar el criterio de matching según si el material es más transiente o más sostenido. Esto requiere primero **clasificar el tipo de señal** de alguna forma automática.

### Factor de cresta (crest factor) como indicador
- El **factor de cresta** es la relación entre el valor de Peak y el valor RMS de una señal (generalmente expresado en dB). Es, justamente, una medida directa de "qué tan transiente vs sostenido" es el material:
  - Factor de cresta **alto** → señal con picos puntuales mucho más altos que su energía promedio (ej. percusión, transientes marcados, material muy dinámico).
  - Factor de cresta **bajo** → señal con energía más pareja en el tiempo, picos no muy por encima del promedio (ej. material ya comprimido, sostenidos tipo sintetizador/órgano, ruido).
- Esto es coherente con la intuición de la idea original: medir el factor de cresta (en una ventana de análisis continua, no de todo el material de una sola vez, dado que el plugin procesa en tiempo real) da una métrica simple y barata de calcular para decidir hacia qué lado inclinarse.

### Cómo usarlo para decidir la métrica de matching
- Con factor de cresta **alto** (material transiente): dar más peso relativo a una métrica más sensible a picos (Peak o LUFS momentary, ventana corta) para no perder de vista los transientes al hacer el matching.
- Con factor de cresta **bajo** (material sostenido/ya comprimido): dar más peso a una métrica de ventana más larga (LUFS short-term o RMS), porque ahí "lo que importa" es la energía sostenida, no picos puntuales que casi no existen.
- En vez de un cambio abrupto de métrica (todo Peak vs todo RMS/LUFS según un umbral fijo), tiene más sentido una mezcla continua ponderada por el factor de cresta medido (un crossfade gradual entre ambos criterios), para que no haya saltos de comportamiento perceptibles cuando el material cambia de carácter (por ejemplo, de una sección sostenida a una con más percusión dentro de la misma canción).

### A investigar/calibrar puntualmente
- Definir el rango de factor de cresta que se considera "transiente" vs "sostenido" calibrando con material real variado (esto, otra vez, es ajuste empírico más que fórmula cerrada).
- Definir la ventana de tiempo sobre la que se mide el factor de cresta de forma continua (debe ser lo bastante larga para ser estable, pero lo bastante corta para reaccionar a cambios de sección dentro del material).

## 3. Suavizado / look-ahead para evitar "bombeo"

### El problema
Si la ganancia de compensación se ajusta de forma instantánea cada vez que la medición de nivel cambia, el resultado es un efecto de "bombeo" (pumping): cambios de ganancia audibles y bruscos que se notan como un artefacto, similar al pumping de un compresor mal ajustado (de hecho es el mismo fenómeno, aplicado a una etapa de matching de nivel en vez de a una de compresión clásica).

### Suavizado (smoothing)
- La solución estándar es no aplicar el valor de ganancia "crudo" calculado en cada instante, sino suavizarlo en el tiempo (similar a un attack/release de compresor): la ganancia se mueve gradualmente hacia el valor objetivo, en vez de saltar directamente.
- Attack/release demasiado rápidos → poco suavizado, más bombeo. Demasiado lentos → la compensación no sigue bien los cambios reales de nivel del material (puede sonar como que "no está corrigiendo nada" en cambios de sección importantes).
- Al igual que con el notch dinámico de la etapa de resonancias, esto es un trade-off de diseño que se calibra con material real, no una fórmula única correcta para todos los casos.

### Look-ahead
- Un buffer de look-ahead permite que el algoritmo "vea" el material un poco antes de que llegue a la salida, y así anticipe cambios de nivel (por ejemplo, un transiente fuerte que se viene) antes de que ocurran, en vez de reaccionar después del hecho.
- Esto permite una respuesta más suave y precisa (evita que el ajuste de ganancia "llegue tarde" a un cambio brusco de nivel), a costa de agregar **latencia** (el tamaño del buffer de look-ahead se traduce directamente en latencia adicional del plugin).
- Dado que esta es la última etapa de la cadena (después de saturación, EQ dinámico, multibanda, auto-EQ y control de resonancias), y varias de esas etapas ya pueden tener su propio presupuesto de latencia (oversampling, FFT), hay que evaluar el presupuesto total de latencia del plugin completo antes de decidir cuánto look-ahead es aceptable acá.

### A investigar/calibrar puntualmente
- Definir attack/release de la compensación de ganancia probando con material de distinto carácter (transiente vs sostenido) hasta encontrar un balance que no genere bombeo perceptible ni se sienta "lento" para reaccionar a cambios reales de nivel.
- Sumar el presupuesto de latencia de esta etapa al de las etapas anteriores (saturación con oversampling, EQ dinámico, control de resonancias) para tener una cifra total de latencia del plugin, y decidir si es aceptable para el caso de uso previsto (mezcla en vivo de baja latencia vs mastering offline, donde más latencia es tolerable).

## Fuentes a revisar (buscar documentación/recursos vigentes, sin asumir URLs específicas)
- Especificación ITU-R BS.1770 (medición de loudness, filtro K-weighting) y EBU R128 (uso práctico del estándar en broadcast).
- Recursos sobre "crest factor" aplicado a clasificación de material de audio (transiente vs sostenido).
- Literatura estándar de procesamiento de dinámica de audio sobre attack/release y look-ahead en compresores, aplicable por analogía a esta etapa de matching de ganancia.
