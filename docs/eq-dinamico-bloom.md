# EQ dinámico tipo Bloom — investigación

Notas de investigación sobre el concepto detrás de Bloom (oeksound) y cómo implementar el análisis espectral en tiempo real.

## 1. Cómo funciona conceptualmente Bloom (oeksound)

Bloom **no es un EQ dinámico tradicional** (banda fija + threshold + ratio, tipo "si supera -10 dB en esta banda, atenuá X dB"). La diferencia conceptual clave:

- Un EQ dinámico clásico trabaja con **bandas predefinidas por el usuario** (frecuencia central, Q, threshold fijo) y reacciona solo cuando el nivel en esa banda específica cruza el umbral. Es reactivo y localizado.
- Bloom analiza el **balance espectral relativo** de la señal en tiempo real (cómo está distribuida la energía a lo largo de todo el espectro, no banda por banda con reglas fijas) y ajusta continuamente la curva de EQ para que esa distribución se mantenga "balanceada" según un criterio interno, sin que el usuario defina frecuencias ni thresholds manualmente.
- El comportamiento se describe más como "nivelar" el espectro de forma continua y adaptativa: zonas que sobresalen relativo al resto del espectro se atenúan un poco, zonas que faltan se realzan un poco, de forma suave y constante, no como un gate/compresor de banda que dispara solo al cruzar un umbral puntual.
- Tiene pocos controles expuestos al usuario (a diferencia de un EQ dinámico tradicional con N bandas configurables), lo cual es consistente con que el algoritmo decide internamente dónde y cuánto actuar, en vez de que el usuario defina las bandas.

### Lectura de esto para nuestro plugin
Lo que necesitamos modelar no es "una banda de EQ dinámico con threshold", sino un **análisis continuo del espectro completo** que determine, en cada instante, qué tan desbalanceada está la energía relativa entre regiones de frecuencia, y aplique una corrección suave y continua (no on/off, no por umbral) para acercar esa distribución a un balance objetivo.

### A investigar puntualmente
- No existe documentación técnica pública de oeksound sobre el algoritmo interno exacto de Bloom (es propiedad de la empresa) — lo que se puede investigar es el *comportamiento* (escuchando/probando el plugin real, leyendo descripciones de marketing/reviews) y aproximar conceptualmente con técnicas de análisis espectral conocidas (ver punto 2), no esperar encontrar el algoritmo exacto publicado.
- Buscar si existen papers académicos sobre "spectral balancing" o "adaptive spectral leveling" en tiempo real que se acerquen al comportamiento descripto (probablemente bajo términos de "dynamic spectral shaping" o similar en el campo de procesamiento de audio).

## 2. FFT-based vs banco de filtros para análisis espectral en tiempo real

Para implementar este tipo de balanceo espectral continuo hace falta, en algún punto, "ver" cómo está distribuida la energía en frecuencia, momento a momento. Dos enfoques:

### Basado en FFT
- Se toma la señal en bloques (frames), se aplica FFT (con ventaneo, overlap-add o similar) y se obtiene la distribución de energía en bins de frecuencia.
- **Ventajas:** resolución de frecuencia muy alta y uniforme en términos de bins (mejor para detectar desbalances finos), más directo de implementar el análisis en sí (hay librerías FFT muy optimizadas).
- **Desventajas:** la FFT introduce **latencia** (depende del tamaño de ventana — ventanas más grandes dan mejor resolución en frecuencia pero peor resolución temporal y más latencia; es el trade-off clásico tiempo/frecuencia). También hay que reconstruir la señal procesada (IFFT + overlap-add), lo que agrega más latencia y complejidad si el procesamiento no es puramente de "análisis" sino que también modifica la señal en el dominio espectral directamente.
- Para uso solo de *análisis* (decidir cuánto boost/atenuación aplicar) combinado con un banco de filtros para el *procesamiento* real, la latencia de la FFT puede aislarse al camino de control (side-chain de análisis) sin que la señal de audio principal pase por FFT/IFFT — esto reduce mucho el problema de latencia.

### Basado en banco de filtros (filterbank)
- Se divide la señal en múltiples bandas mediante filtros IIR (o FIR) en paralelo, y se mide energía/nivel en cada banda en el dominio del tiempo.
- **Ventajas:** latencia mucho más baja (los filtros IIR pueden ser de orden bajo y casi sin latencia agregada), más económico en CPU para una cantidad moderada de bandas, comportamiento más predecible y fácil de ajustar en tiempo real (attack/release por banda, como en un multibanda clásico).
- **Desventajas:** resolución en frecuencia más limitada/menos uniforme que una FFT con muchos bins (a menos que se usen muchas bandas, lo cual sube el costo de CPU), y diseñar un banco de muchas bandas que sume plano (igual que el problema de los crossovers, ver investigación de saturación multibanda) requiere cuidado.

### Comparación para este caso de uso
- Si el objetivo es una corrección **suave y de baja latencia, todo el tiempo activa** (que es como se describe el comportamiento de Bloom), un banco de filtros con bandas relativamente finas puede ser más práctico que FFT completa, justamente por la latencia.
- Si se prioriza la precisión de detección de desbalances finos (por ejemplo, distinguir picos angostos vs anchos) y se puede tolerar algo más de latencia (esta etapa va después de la saturación, en medio de una cadena que probablemente ya tiene oversampling y otras etapas con FFT como el control de resonancias, así que la latencia total del plugin ya no sería "cero" de todas formas), FFT puede dar mejores resultados.
- Híbrido razonable: usar FFT solo para el **análisis** (decidir la curva de corrección) en un camino paralelo de baja prioridad de tiempo real estricto, y aplicar la corrección resultante mediante un banco de filtros/EQ paramétrico de baja latencia sobre la señal real. Esto es, conceptualmente, similar a cómo varios plugins de "auto-EQ" comerciales separan análisis de procesamiento.

### A investigar puntualmente
- Definir presupuesto de latencia total del plugin completo (sumando saturación + esta etapa + control de resonancias) para decidir si FFT en el camino de señal es aceptable o conviene el enfoque híbrido (FFT solo en análisis).
- Si se opta por banco de filtros, investigar diseños de bancos de muchas bandas con suma plana garantizada (ej. bancos basados en filtros Linkwitz-Riley en cascada o en diseños tipo "Bark scale"/perceptuales, que distribuyen las bandas de forma similar a cómo el oído percibe el espectro en vez de lineal en Hz).
- Revisar tamaños de ventana FFT típicos usados en plugins de análisis espectral en tiempo real (suelen rondar 1024–4096 muestras a 44.1/48kHz) como punto de partida si se usa FFT.

## Fuentes a revisar (buscar documentación/recursos vigentes, sin asumir URLs específicas)
- Página/descripción de producto de oeksound Bloom (para entender el comportamiento descripto por el fabricante, no el algoritmo).
- Recursos sobre "spectral balancing" / "dynamic spectral shaping" en procesamiento de audio.
- Recursos sobre diseño de bancos de filtros perceptuales (Bark scale, ERB) aplicados a EQ multibanda.
- Recursos sobre trade-off tiempo/frecuencia en STFT (tamaño de ventana, overlap, latencia) para procesamiento de audio en tiempo real.
