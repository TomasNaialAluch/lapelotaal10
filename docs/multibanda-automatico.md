# Compresor multibanda automático — investigación

Notas de investigación sobre cómo automatizar las decisiones de un compresor multibanda (bandas, thresholds, ratios) y sobre a qué plugin de referencia apunta la idea original.

## 1. Determinar automáticamente bandas, puntos de corte, thresholds y ratios

Esto es, como bien lo plantea la idea original, **más un problema de heurísticas y análisis de señal que de DSP puro**: el procesamiento (compresión multibanda) ya está bien resuelto técnicamente; lo difícil es decidir *automáticamente* los parámetros que normalmente configura una persona a mano.

### Cantidad de bandas y puntos de corte
- Un enfoque es analizar el contenido espectral (vía FFT o banco de filtros, ver investigación de [EQ dinámico](eq-dinamico-bloom.md)) y detectar dónde hay **concentraciones o transiciones naturales de energía** en el espectro, en vez de usar puntos de corte fijos.
- Métricas candidatas para guiar esta decisión:
  - **Centroide espectral** (spectral centroid): da una idea de "dónde está el centro de masa" del espectro, útil para saber si el material es grave-dominante, medio o brillante, y ajustar las bandas en consecuencia.
  - **Flatness espectral** (spectral flatness): ayuda a distinguir contenido tonal (picos definidos, como una nota sostenida) de contenido ruidoso/percusivo (energía más distribuida), lo cual puede influir en dónde conviene poner un corte de banda.
  - **Detección de picos en el espectro promediado** (energía por banda de tercio de octava o similar) para ubicar los cortes en los "valles" entre concentraciones de energía, en vez de partir bandas en puntos arbitrarios.
- Una alternativa más simple y pragmática: no determinar dinámicamente la *cantidad* de bandas ni sus límites por señal, sino fijar de antemano una cantidad razonable de bandas con crossover fijo (consistente, además, con que el saturador ya usa 4 bandas fijas — ver [investigación de saturación](saturacion-multibanda.md)), y dejar que lo "automático" sea solo el threshold/ratio por banda, no la cantidad ni los límites. Esto reduce mucho la complejidad sin perder el objetivo de "no tocar nada manualmente".

### Thresholds y ratios automáticos
- Para definir threshold por banda automáticamente, un enfoque común es basarse en estadísticas del propio audio: medir el **nivel RMS y el rango dinámico (crest factor)** de cada banda en una ventana de análisis, y ubicar el threshold relativo a esos valores (ej. unos dB por debajo del RMS de la banda, ajustado según cuán dinámica es esa banda).
- El **ratio** puede definirse en función de cuánto se quiere "controlar" cada banda: bandas con mucha variación de nivel (crest factor alto, picos pronunciados) podrían recibir ratios más altos; bandas más estables, ratios suaves.
- Attack/release automáticos suelen basarse en las características temporales típicas de cada rango de frecuencia (los graves necesitan attack/release más lentos para no distorsionar el ciclo de la onda; los agudos toleran tiempos más rápidos) — esto es una heurística estándar en muchos compresores multibanda, no algo que haya que inventar desde cero.
- Importante: esto es fundamentalmente un problema de **calibración empírica** (probar con muchas señales de referencia y ajustar las heurísticas hasta que el resultado suene bien en la mayoría de los casos), no algo que se resuelva solo con una fórmula matemática cerrada.

### A investigar puntualmente
- Revisar literatura de "loudness range" / "crest factor analysis" aplicada a control automático de dinámica (es un área activa en herramientas de mastering automatizado).
- Decidir si conviene arrancar con bandas fijas (4, alineadas con las del saturador) y solo automatizar threshold/ratio/attack/release, dejando la detección dinámica de cantidad de bandas como una mejora futura más compleja.
- Armar un set de señales de prueba variado (voz, batería, mezcla completa, material muy dinámico vs muy comprimido) para calibrar las heurísticas de threshold/ratio antes de considerar esta etapa "terminada".

## 2. Confirmar la referencia: "MB de pro filters" → ¿FabFilter Pro-MB?

La idea original menciona "como configurar un MB de pro filters, todo automático". La lectura más probable:

- **FabFilter Pro-MB** es un plugin de compresión multibanda real, parte de la línea "Pro" de FabFilter (junto con Pro-Q, Pro-C, Pro-L, etc.). "MB" coincide con la sigla del nombre del producto (Pro-**MB**). Es razonable asumir que "MB de pro filters" se refiere a este plugin, con "pro filters" siendo una forma coloquial de referirse a la línea Pro de FabFilter.
- Pro-MB se caracteriza por: bandas definidas con crossover ajustable libremente (no fijas), cada banda con su propio compresor/expansor configurable (threshold, ratio, knee, attack, release), y la posibilidad de hacerlo "sidechain" por banda. Normalmente se configura **manualmente** banda por banda — no tiene, hasta donde se puede confirmar sin acceso directo al producto actualizado, un modo "todo automático" nativo que decida bandas y parámetros solo.
- Esto sugiere que lo que se quiere imitar **no es necesariamente el modo de uso típico de Pro-MB** (que es manual), sino más bien "el tipo de procesamiento que hace un multibanda como Pro-MB" (separación en bandas + compresión independiente por banda), pero automatizado por nuestro propio análisis de señal (lo desarrollado en el punto 1), no por una función "auto" que exista en Pro-MB.

### A confirmar con el usuario/dueño del proyecto
- ¿La referencia a "pro filters" es efectivamente FabFilter Pro-MB, o se refiere a otra herramienta/plugin con un nombre parecido?
- ¿Lo que se quiere imitar es el *tipo de procesamiento* (separación en bandas + compresión por banda, estilo multibanda profesional) o específicamente algún comportamiento puntual de la interfaz/algoritmo de Pro-MB que haya que escuchar/probar de referencia?
- Si hay acceso a Pro-MB, sería útil probarlo directamente con material de referencia para tener un comportamiento concreto contra el cual comparar nuestra versión automática, en vez de trabajar solo de memoria/descripción.

## Fuentes a revisar (buscar documentación/recursos vigentes, sin asumir URLs específicas)
- Página de producto y manual de FabFilter Pro-MB (para confirmar sus controles reales y si tiene algún modo automático que se nos esté escapando).
- Recursos sobre "crest factor" y "loudness range" aplicados a control dinámico automático.
- Recursos sobre heurísticas de attack/release dependientes de frecuencia en compresión multibanda (es un tema cubierto en bibliografía estándar de procesamiento de dinámica de audio).
