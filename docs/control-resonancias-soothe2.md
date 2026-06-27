# Control de resonancias tipo Soothe2 — investigación

Notas de investigación sobre cómo detectar picos resonantes dinámicos (no fijos) y atenuarlos sin afectar el resto de la señal.

## 1. Detección de picos resonantes dinámicos

### Por qué "dinámico" es la parte difícil
Un EQ estático con una banda angosta puede atenuar una resonancia *fija* (una frecuencia que siempre resuena, por ejemplo un modo de una caja de batería o un formante problemático constante). Pero las resonancias reales en material musical suelen:
- Cambiar de frecuencia exacta de un momento a otro (vibrato, deslizamientos de afinación, diferencias entre notas).
- Aparecer y desaparecer según el contenido (una resonancia molesta en una nota puntual, no en todo el pasaje).
- Variar en ancho de banda (a veces es un pico muy angosto, a veces más ancho).

Por eso una banda fija de EQ no sirve: hace falta **detectar en cada instante dónde está el pico resonante** (que se mueve) y actuar solo ahí, solo cuando corresponde.

### Enfoque conceptual de detección
- Analizar el espectro en tiempo real (FFT con ventaneo, o banco de filtros de alta resolución — mismo trade-off que en la investigación de [EQ dinámico](eq-dinamico-bloom.md)).
- Para cada frame, identificar **picos que sobresalen de forma anómala respecto a la envolvente espectral general** (no respecto a un threshold absoluto fijo, sino respecto a la forma general del espectro en ese instante). Esto es clave: una resonancia se define por ser un pico relativo, angosto, que sobresale de sus vecinos — no por superar un nivel absoluto en dB.
- Una forma de hacer esto es comparar el espectro instantáneo contra una versión **suavizada** del mismo espectro (un "promedio espectral local", como una envolvente suavizada en frecuencia, no en el tiempo): donde el espectro real supera mucho a esa envolvente suavizada de forma angosta, hay candidato a resonancia.
- También se puede incorporar seguimiento temporal (tracking): si un pico angosto persiste o se mueve de forma continua entre frames consecutivos (en vez de aparecer y desaparecer de forma errática, que podría ser solo ruido de análisis), es más confiable tratarlo como una resonancia real y no como un artefacto del análisis.

### Distinguir resonancia de contenido tonal deseado
El mayor desafío práctico: una nota sostenida de un instrumento también es, espectralmente, "un pico angosto que sobresale". Hay que evitar atenuar contenido musical legítimo (por ejemplo, la fundamental de una nota) tratándolo como si fuera una resonancia indeseada. Algunas pistas para diferenciar:
- Las resonancias indeseadas suelen ser más angostas en Q y más erráticas en el tiempo que una nota musical sostenida.
- Las resonancias suelen acumularse en frecuencias específicas del instrumento/sala de forma repetida a lo largo del material, mientras que el contenido tonal deseado varía con la melodía/armonía.
- Esto sugiere que el algoritmo no debería basarse solo en "es un pico angosto" sino combinar eso con algún criterio de persistencia/contexto para no comerse notas reales.

### A investigar puntualmente
- Definir el ancho de banda mínimo/máximo que se considera "candidato a resonancia" vs "contenido tonal", y calibrarlo con material real (esto es, otra vez, más heurística calibrada de oído que fórmula cerrada).
- Revisar técnicas de "spectral envelope smoothing" (suavizado de envolvente espectral, usado también en vocoders y en análisis de timbre) como base para detectar qué tan "anómalo" es un pico respecto a su entorno.

## 2. Atenuar sin afectar el resto de la señal

Una vez detectado un pico resonante (frecuencia central + ancho aproximado + cuánto sobresale), hay que atenuarlo de forma quirúrgica:

### Filtrado adaptativo / notch dinámico
- La idea es un filtro **notch (rechaza-banda) cuya frecuencia central, ancho de banda y profundidad de atenuación se ajustan dinámicamente, frame a frame**, siguiendo al pico detectado, en vez de un notch fijo configurado a mano.
- Esto requiere que los coeficientes del filtro se recalculen continuamente (a una tasa de actualización razonable, no necesariamente muestra por muestra) a partir de los parámetros que entrega la etapa de detección.
- Punto crítico de diseño: la **velocidad de adaptación** del filtro. Si cambia demasiado rápido, puede generar artefactos audibles (clicks, modulación audible del filtro, "bombeo" espectral). Si cambia demasiado lento, no sigue bien a resonancias que se mueven rápido (por ejemplo, vibrato). Esto es análogo al trade-off de attack/release en un compresor, pero aplicado a la frecuencia central del filtro en vez de a la ganancia.
- La profundidad de atenuación también debería ser dinámica y proporcional a cuánto sobresale el pico detectado (no un "todo o nada"), para que la corrección sea proporcional y no se note como un agujero artificial en el espectro cuando la resonancia es leve.

### Alternativa: procesamiento directo en el dominio espectral (FFT)
- En vez de un filtro notch adaptativo en el dominio del tiempo, se puede atenuar directamente los bins de la FFT que correspondan al pico detectado, y luego reconstruir la señal (IFFT + overlap-add).
- Ventaja: permite una resolución y precisión muy altas (se ataca exactamente el o los bins correspondientes al pico, sin afectar bins vecinos).
- Desventaja: agrega la latencia propia de la FFT/IFFT (mismo tema que en la investigación de EQ dinámico), y hay que tener cuidado con artefactos de reconstrucción si se modifican bins de forma muy abrupta entre frames consecutivos (puede generar "musical noise", un artefacto clásico de procesamiento espectral agresivo, conocido sobre todo de algoritmos de reducción de ruido).

### Comparación para este caso
- Notch adaptativo en el dominio del tiempo: menor latencia, más simple de integrar en una cadena que ya tiene varias etapas con presupuesto de latencia ajustado, pero requiere más cuidado en el diseño de la velocidad de adaptación para que no se note el movimiento del filtro.
- Procesamiento espectral directo (FFT): más preciso y quirúrgico, pero con latencia adicional y riesgo de artefactos de reconstrucción si no se maneja bien el suavizado entre frames.
- Dado que esta etapa va al final de la cadena (después de saturación, EQ dinámico y multibanda, que ya consumen presupuesto de latencia), probablemente convenga priorizar el enfoque de menor latencia (notch adaptativo en el tiempo) salvo que las pruebas muestren que la precisión del enfoque espectral es necesaria para resultados aceptables.

### A investigar puntualmente
- Probar ambos enfoques (notch adaptativo vs FFT directo) sobre el mismo material de prueba y comparar tanto la calidad del resultado como la latencia/CPU real, antes de decidir cuál usar en la versión final.
- Revisar diseños de filtros notch de coeficientes variables en tiempo real (IIR de segundo orden tipo "peaking/notch" con actualización de coeficientes por frame) como base de implementación si se opta por el enfoque en el dominio del tiempo.
- Revisar técnicas para evitar "musical noise" en procesamiento espectral si se opta por el enfoque FFT (suavizado temporal de la máscara de atenuación entre frames, no solo en frecuencia).

## Fuentes a revisar (buscar documentación/recursos vigentes, sin asumir URLs específicas)
- Página de producto de oeksound Soothe2 (para el comportamiento descripto por el fabricante, no el algoritmo interno, que no está publicado).
- Recursos sobre detección de picos espectrales y suavizado de envolvente espectral (spectral envelope smoothing), usados también en vocoders y análisis de timbre.
- Recursos sobre diseño de filtros notch/peaking de coeficientes variables en tiempo real (IIR de segundo orden, tipo "RBJ biquad" con actualización dinámica de parámetros).
- Literatura sobre "musical noise" en supresión de ruido espectral, como referencia de qué evitar si se procesa en el dominio FFT.
