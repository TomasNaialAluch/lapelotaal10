# Saturación multibanda — investigación

Notas de investigación sobre cómo separar las 4 bandas y qué modelo de saturación usar en cada una.

## 1. Filtros crossover para separar las 4 bandas

Bandas objetivo: < 250 Hz, 250–400 Hz, 400–2000 Hz, 2000–20000 Hz.

### Opciones de crossover
- **Linkwitz-Riley (LR24, LR48, etc.):** el estándar de facto en crossovers de audio. Suma plana en magnitud al recombinar las bandas (las pendientes de los filtros pasa-bajos y pasa-altos se cruzan a -6 dB y se cancelan al sumar). Introduce desfasaje (phase shift) que varía según el orden del filtro, pero es predecible y bien documentado.
- **Filtros IIR Butterworth en cascada (no LR):** más simples pero no dan suma plana al recombinarse sin ajustes adicionales; no es lo recomendado para este caso.
- **FIR lineal de fase (linear-phase crossover):** no introduce desfasaje, pero agrega **latencia** (proporcional al largo del filtro) y consume más CPU. Es la opción si la fase es crítica y la latencia es aceptable (más viable en mastering que en mezcla en vivo/baja latencia).
- **Filtros IIR de fase mínima (mínimum-phase, no lineales):** baja latencia, pero con desfasaje no lineal entre bandas — puede generar cancelaciones sutiles al recombinar si no se diseña con cuidado.

### Problema central a resolver
Al separar en 4 bandas, saturar cada una con una curva distinta, y volver a sumarlas, las no-linealidades de la saturación generan armónicos que caen en otras bandas de frecuencia. Esto significa que la suma final **no es simplemente "LR perfecto"** como en un crossover lineal pasivo: hay que validar de oído y con analizador de espectro que la recombinación post-saturación no genere cancelaciones o acumulaciones raras de fase.

### A investigar/probar puntualmente
- Empezar con Linkwitz-Riley de 4º orden (24 dB/oct) por banda como punto de partida estándar, y comparar contra una versión lineal de fase si la latencia resultante es aceptable para el caso de uso (mezcla vs mastering).
- Medir la respuesta de fase y magnitud de la suma de las 4 bandas *después* de aplicar saturación en cada una (no solo en la separación limpia sin saturar), porque ahí es donde puede aparecer el problema real.

## 2. Modelado de saturación tipo tubo (válvula)

- Las válvulas generan principalmente **armónicos pares** (2do armónico dominante) además de impares, lo cual se percibe como "calidez" — distinto a la saturación de transistor/diodo que tiende a generar más armónicos impares (sonido más "duro").
- La curva de transferencia de una válvula es **asimétrica** (no es lo mismo la mitad positiva que la negativa de la onda), a diferencia de un clipper simétrico tipo tanh.
- **Modelo de Koren:** ecuación que aproxima la curva característica de una tríodo (corriente de placa en función de voltajes de grilla/placa). Es el modelo de referencia más citado para modelado "físico" de preamplificadores a válvula. Más complejo de implementar que una simple no-linealidad estática, porque normalmente involucra resolver una relación implícita (a veces vía Wave Digital Filters, WDF) para modelar el circuito completo (no solo la válvula aislada).
- **Aproximación más simple:** una función de transferencia asimétrica estática (por ejemplo, una curva polinómica o una tanh con offset/asimetría) que no modela el circuito físico pero captura el rasgo perceptual clave: asimetría + énfasis en 2do armónico.

### A investigar puntualmente
- Decidir nivel de fidelidad: modelo físico de circuito (Koren + WDF, más preciso pero mucho más trabajo de DSP) vs curva estática asimétrica ajustada de oído (mucho más simple de implementar y de hacer andar en tiempo real con bajo costo de CPU).
- Buscar papers/recursos sobre modelado de preamplificadores a válvula vía Wave Digital Filters si se opta por el enfoque físico.

## 3. Modelado de saturación tipo diodo

- Un **clipper de diodo** (como los de pedales de distorsión clásicos) usa la ecuación de **Shockley** (la ecuación del diodo ideal) para relacionar voltaje y corriente, y produce un clipping con una rodilla (knee) cuya dureza depende de la configuración del circuito (diodos en paralelo a tierra vs en el feedback de un op-amp, cantidad de diodos en serie, etc.).
- **Knee duro vs suave:** un clipper de diodo simple (pocos diodos, configuración básica) da un knee relativamente abrupto → más armónicos altos, sonido más agresivo. Agregar diodos en serie o usar configuraciones más elaboradas suaviza la transición → menos armónicos altos, sonido más controlado.
- A diferencia de la válvula, el diodo tiende a generar más simetría en la curva (si el circuito es simétrico) y un espectro de armónicos más cargado hacia las frecuencias altas — coherente con por qué el usuario lo asocia a las bandas más altas (400 Hz–20 kHz) en lugar de a los bajos.

### A investigar puntualmente
- Implementar la ecuación de Shockley como no-linealidad estática es viable y barato en CPU; lo más importante a definir es la curva exacta (cuántos "diodos virtuales", qué tan dura la rodilla) según qué tan agresivo se quiere el carácter en esas bandas.
- Revisar diseños clásicos de clippers de diodo (ej. circuitos de pedales de distorsión documentados) como referencia de curvas ya validadas auditivamente.

## 4. Modelado de saturación tipo cinta (tape)

- La saturación de cinta no es un simple clipping: incluye **histéresis** (la salida depende no solo de la entrada actual sino del estado magnético previo de la cinta) y compresión suave dependiente de nivel.
- **Modelo de Jiles-Atherton:** modelo físico de histéresis magnética, el más preciso para emular cinta de forma realista, pero computacionalmente más caro y más complejo de implementar y estabilizar en tiempo real.
- **Aproximación simplificada (tanh u otras curvas suaves sin memoria):** ignora la histéresis real pero captura buena parte del carácter perceptual (saturación suave, compresión de los picos) con mucho menos costo de CPU. Es el enfoque típico en plugins comerciales que no necesitan modelado "físico" exacto.
- Efectos adicionales asociados a cinta real (a evaluar si se quieren incluir o no): "head bump" (resonancia en graves característica de cabezales de cinta), wow/flutter (variación de velocidad). Probablemente fuera de alcance para este proyecto si el objetivo es solo el carácter de saturación, no una emulación completa de una grabadora de cinta.

### A investigar puntualmente
- Decidir si vale la pena implementar Jiles-Atherton (mayor fidelidad, mayor costo/complejidad) o si una curva tipo tanh con algo de memoria simple (un filtro de un polo modelando algo de "lag" sin histéresis completa) alcanza para el resultado sonoro buscado.
- Si se usa Jiles-Atherton, investigar métodos de integración numérica estables para tiempo real (Euler simple puede ser inestable; métodos tipo Runge-Kutta o soluciones semi-analíticas son más robustos).

## 5. Qué distingue "warm" de "tube" en términos de DSP

Esto **no es un término técnico estándar** — hay que definirlo nosotros mismos antes de implementarlo. Algunas hipótesis de partida a evaluar y afinar de oído:

- "Tube" → curva asimétrica con énfasis claro en el 2do armónico (par), más "color" armónico identificable.
- "Warm" → quizás una saturación más sutil/suave en general (menos armónicos generados, más parecido a una compresión suave de los picos que a una distorsión con carácter propio), pensada para los graves donde no se quiere agregar "brillo" ni armónicos altos, solo redondear transientes y agregar densidad.
- Otra hipótesis: "warm" podría ser simplemente una versión de menor intensidad/mix del mismo tipo de curva que "tube", en vez de un modelo completamente distinto.

### A investigar/decidir puntualmente
- Esto se resuelve más por prueba auditiva que por research bibliográfico: armar 2-3 candidatos de curva para "warm" (variantes de baja intensidad de tanh/asimetría) y compararlos en A/B sobre la banda de graves antes de fijar el modelo definitivo.
- Documentar la decisión final acá una vez probada, para que quede como referencia de diseño y no como term suelto.

## 6. Oversampling y aliasing

- Cualquier no-linealidad (saturación) genera armónicos que pueden superar la frecuencia de Nyquist de la sample rate de trabajo, generando **aliasing** (artefactos audibles, sonido "sucio" no intencional) si no se controla.
- Las bandas más agresivas en términos de armónicos generados (probablemente 400 Hz–20 kHz, donde están los modelos de diodo/cinta más "duros") son las más críticas para necesitar oversampling.
- **2x oversampling** suele ser un mínimo razonable para saturación moderada; **4x** da más margen para saturaciones más agresivas o con harmónicos muy altos, a costa de más CPU.
- Importante: el oversampling debe aplicarse banda por banda *si* cada banda satura distinto (oversamplear todo el bus antes de splitear es menos eficiente y no necesariamente resuelve el aliasing generado dentro de cada banda después del split).
- Costo: oversampling implica upsample (interpolación + filtro anti-aliasing) antes de saturar y downsample (filtro + decimación) después, lo cual agrega **latencia** (generalmente pequeña pero no nula) y **consumo de CPU** (más notorio si se hace por banda y no una sola vez global).

### A investigar/decidir puntualmente
- Medir cuánto aliasing real genera cada modelo de saturación (tubo/diodo/cinta) en cada banda a la sample rate típica de trabajo (44.1/48 kHz) antes de decidir si 2x alcanza o se necesita 4x.
- Evaluar oversampling selectivo: aplicarlo solo en las bandas/franjas donde el análisis muestre aliasing audible, en vez de aplicarlo a ciegas en las 4 bandas, para ahorrar CPU.
- Revisar técnicas de oversampling eficientes para plugins en tiempo real (filtros polifásicos) en vez de upsampling/downsampling naive.

## Fuentes a revisar (buscar documentación/papers vigentes, sin asumir URLs específicas)
- Papers sobre crossovers Linkwitz-Riley y su comportamiento de fase.
- Papers/recursos sobre modelado de válvulas vía ecuación de Koren y Wave Digital Filters.
- Documentación/circuitos clásicos de clippers de diodo (ecuación de Shockley aplicada a audio).
- Papers sobre modelo de histéresis de Jiles-Atherton aplicado a emulación de cinta magnética.
- Recursos sobre oversampling y filtros polifásicos para procesamiento no lineal en tiempo real (DSP de audio).
