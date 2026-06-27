# Rendimiento y latencia — investigación

Notas de investigación sobre el presupuesto total de latencia de las 6 etapas de la cadena, cómo reportarlo correctamente en VST3, y cómo optimizar el plugin para que sea viable sobre múltiples pistas a la vez.

## 1. Presupuesto de latencia total

### De dónde viene la latencia en cada etapa
Cada etapa de la cadena puede aportar latencia por motivos distintos (ver investigaciones previas para el detalle de cada una):

| Etapa | Fuente de latencia |
|---|---|
| [Saturación multibanda](saturacion-multibanda.md) | Crossover (más si es lineal de fase) + oversampling por banda (upsample/downsample) |
| [EQ dinámico tipo Bloom](eq-dinamico-bloom.md) | Análisis FFT (si se usa) en el camino de control, o filtros del banco si se usa esa vía |
| Multibanda automático | Crossover (comparte el mismo problema que la saturación) + ventana de análisis para thresholds/ratios automáticos |
| [Auto-EQ Hi-Fi](auto-eq-hifi.md) | Análisis espectral perceptual (Bark/ERB) para detección de enmascaramiento |
| [Control de resonancias](control-resonancias-soothe2.md) | FFT/IFFT si se usa el enfoque espectral directo, o mínima si es notch adaptativo en el tiempo |
| [Loudness matching](loudness-matching.md) | Ventana de medición LUFS + posible buffer de look-ahead |

### El problema central
Cada etapa fue investigada por separado asumiendo trade-offs propios de latencia, pero **la latencia total del plugin es la suma de todas**, y varias decisiones de diseño (FFT vs filtros, lineal de fase vs mínima fase, look-ahead sí/no) se tomaron etapa por etapa sin un presupuesto global todavía. Antes de fijar la latencia de cada etapa de forma aislada, hace falta:

1. Definir un **presupuesto total aceptable** según el caso de uso previsto (ver más abajo, esto depende de si el plugin se piensa para mezcla en vivo/monitoreo en tiempo real, o para mastering offline donde la latencia casi no importa).
2. Sumar la latencia mínima necesaria de cada etapa si se usan los enfoques de mayor latencia (FFT, lineal de fase, look-ahead) en todas, y ver si el total entra en el presupuesto.
3. Si no entra, decidir en qué etapas conviene sacrificar latencia (yendo a la opción más rápida/mínima fase/sin look-ahead) y en cuáles vale la pena pagar el costo porque la calidad lo justifica.

### Casos de uso y su tolerancia a latencia
- **Mezcla con monitoreo en tiempo real (tocando un instrumento en vivo a través del plugin, o mezclando con baja latencia general):** la tolerancia es baja, idealmente unos pocos milisegundos en total. Esto empujaría a elegir las opciones de mínima fase/sin look-ahead/sin FFT en la mayoría de las etapas.
- **Mezcla "normal" en DAW (sin tocar en vivo a través del plugin, solo escuchando la mezcla mientras se ajusta):** la tolerancia es más alta porque la mayoría de los DAWs compensan la latencia entre pistas automáticamente (ver punto de PDC más abajo) — unos 10-30 ms no debería ser un problema grave en la mayoría de los flujos de trabajo, aunque sigue siendo mejor cuanto menos.
- **Mastering offline (procesar un archivo ya mezclado, sin necesidad de tocar nada en vivo):** la latencia casi no importa, puede ser de cientos de milisegundos sin problema real, porque no hay interacción en tiempo real que dependa de ella.

### A investigar/decidir puntualmente
- Definir explícitamente para qué caso de uso principal se diseña el plugin (esto cambia radicalmente cuánta latencia es aceptable, y por lo tanto qué enfoque elegir en cada etapa de las investigadas antes).
- Una vez definido el presupuesto, volver sobre cada documento de investigación anterior y fijar la decisión concreta de enfoque (FFT vs filtros, fase lineal vs mínima, con/sin look-ahead) en función de ese presupuesto, en vez de dejarlo abierto etapa por etapa.

## 2. Reportar la latencia como Plugin Delay Compensation (PDC) en VST3

### Qué es y por qué importa
La mayoría de los DAWs modernos compensan automáticamente la latencia que introduce un plugin, para que todas las pistas queden alineadas en el tiempo aunque algunas tengan plugins con más latencia que otras. Para que esto funcione, **el plugin tiene que declarar correctamente cuánta latencia introduce**, no puede ser un valor adivinado por el host.

### Cómo funciona en VST3 (a nivel conceptual)
- VST3 tiene un método en la interfaz del processor para reportar latencia en muestras (la cantidad exacta de muestras de delay que el plugin introduce entre la entrada y la salida).
- Si la latencia del plugin es **fija** (no cambia mientras el audio está sonando, por ejemplo porque el oversampling y los buffers de FFT son de tamaño constante), alcanza con reportar ese valor fijo una vez.
- Si la latencia **cambiara dinámicamente** (por ejemplo, si alguna etapa decidiera cambiar de tamaño de ventana de análisis en tiempo real según el contenido, lo cual no es lo recomendado), hay que notificar al host que la latencia cambió para que recalcule la compensación — esto es mucho más delicado y puede generar glitches notables en la reproducción, así que es preferible evitarlo: **fijar la latencia de cada etapa en un valor constante durante toda la vida del plugin**, no variable según el análisis de la señal.

### Consecuencia de diseño importante
Esto refuerza que, aunque varias etapas usan análisis adaptativo del contenido (heurísticas de multibanda automático, detección de resonancias, etc.), el **tamaño de los buffers/ventanas que determinan la latencia debe quedar fijo** (decidido en el diseño, no recalculado dinámicamente en tiempo real según la señal), aunque los *parámetros* que se calculan a partir de esas ventanas sí sean dinámicos.

### A investigar puntualmente
- Revisar la documentación oficial del VST3 SDK sobre el método exacto para reportar latencia (nombre de la función/interfaz puede variar entre versiones del SDK) y confirmar el flujo correcto al momento de implementar.
- Si se usa JUCE (ver [investigación de framework](framework-desarrollo.md)), revisar cómo JUCE expone esto (suele tener un método propio, ej. algo del estilo `setLatencySamples`, que internamente lo traduce a la llamada correcta de VST3) para no tener que lidiar directamente con la interfaz nativa del SDK.
- Confirmar que el valor reportado sea la suma exacta de la latencia real de todas las etapas (oversampling + FFT + look-ahead, sumados), no una estimación aproximada — un valor incorrecto desalinea el audio en el DAW de forma sutil pero real.

## 3. Optimización para múltiples pistas simultáneas (SIMD, multithreading)

### Por qué importa
Un plugin de este tipo (saturación multibanda + análisis espectral en varias etapas + multibanda automático) es bastante más pesado en CPU que un EQ o compresor simple. Si el usuario lo pone en muchas pistas a la vez (lo cual es plausible si se piensa como procesador "todo en uno" de uso general), el costo de CPU total puede volverse un problema práctico real, no solo teórico.

### SIMD (Single Instruction, Multiple Data)
- Permite procesar varias muestras de audio (o varios canales, o varias bandas) en paralelo usando instrucciones vectoriales del procesador, en vez de una muestra a la vez.
- Es especialmente aplicable a las partes de procesamiento "puramente numéricas" de cada etapa: aplicar la no-linealidad de saturación a un bloque de muestras, calcular FFTs (las librerías de FFT optimizadas ya usan SIMD internamente), aplicar ganancia, etc.
- JUCE tiene utilidades propias (`juce::dsp::SIMDRegister` y el namespace `juce::dsp` en general) pensadas para esto si se elige JUCE como framework (ver investigación de framework).
- Es más una cuestión de implementación cuidadosa (estructurar el código en bloques vectorizables, evitar ramas condicionales dentro de los loops más calientes) que de investigación conceptual — pero vale la pena tenerlo en mente desde el diseño de cada etapa, no como optimización tardía.

### Multithreading
- Permite repartir trabajo entre varios núcleos del procesador. Dos niveles posibles:
  - **Dentro de una instancia del plugin:** repartir el trabajo de las distintas bandas o etapas entre threads (por ejemplo, procesar las 4 bandas de saturación en paralelo en vez de secuencialmente). Esto agrega complejidad real (sincronización, evitar condiciones de carrera, latencia adicional si hay que esperar a que todos los threads terminen antes de seguir a la siguiente etapa) y los DAWs ya suelen paralelizar entre pistas distintas a nivel de host, así que el beneficio de paralelizar *dentro* de una sola instancia hay que evaluarlo con cuidado (puede no compensar la complejidad si el host ya reparte las pistas entre núcleos).
  - **Entre instancias (pistas distintas):** esto normalmente lo maneja el propio DAW host, repartiendo el procesamiento de audio de distintas pistas/plugins entre núcleos disponibles — no es algo que el plugin tenga que implementar él mismo, es responsabilidad del host.
- Conclusión preliminar: probablemente no hace falta implementar multithreading propio *dentro* de cada instancia del plugin en una primera versión; conviene primero optimizar bien con SIMD y algoritmos eficientes, y solo considerar paralelizar internamente si las mediciones de CPU muestran que es un cuello de botella real incluso después de esa optimización.

### A investigar/medir puntualmente
- Una vez que haya una primera implementación funcional, medir el costo real de CPU por instancia (con un profiler) para identificar cuáles de las 6 etapas son las más caras en la práctica, en vez de asumir de antemano cuál lo será (es probable que las etapas con FFT/oversampling sean las más pesadas, pero hay que confirmarlo con datos reales).
- Recién con esos datos decidir si vale la pena la complejidad de paralelizar internamente alguna etapa puntual, o si alcanza con optimización SIMD + buen diseño de algoritmos.
- Revisar técnicas de oversampling y FFT eficientes (filtros polifásicos, librerías de FFT ya optimizadas con SIMD como punto de partida) en vez de implementaciones naive, antes de llegar a la etapa de multithreading.

## Fuentes a revisar (buscar documentación/recursos vigentes, sin asumir URLs específicas)
- Documentación oficial del VST3 SDK de Steinberg sobre reporte de latencia (Plugin Delay Compensation).
- Documentación de JUCE sobre `setLatencySamples` y el namespace `juce::dsp` (SIMD, FFT, filtros).
- Recursos generales sobre optimización de DSP en tiempo real (vectorización, diseño de loops calientes sin ramas condicionales, librerías de FFT con soporte SIMD).
