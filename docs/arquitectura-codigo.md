# Arquitectura de código — cómo encarar esto de forma incremental

Notas sobre cómo estructurar el código del plugin para poder construirlo de a partes, empezando mínimo y escalando en complejidad sin tener que reescribir lo ya hecho.

## Punto de partida: se puede, Saturn (FabFilter) lo prueba

Saturn 2 (FabFilter) es un saturador multibanda en producción real, lo cual confirma que el problema de fondo (saturación distinta por banda, sumando bien) es resoluble. Eso sirve como **prueba de que el objetivo es alcanzable**, no como referencia de implementación — no hay acceso a su código ni a su algoritmo interno. La conclusión práctica es: el riesgo no es "si se puede", es "cómo llegar ahí sin romper todo en el camino" — que es justamente lo que busca resolver esta arquitectura.

## Principio rector: construir de a una etapa, nunca todas juntas

Cada etapa de la cadena cambia lo que la siguiente recibe. Si se construyen las 6 etapas a la vez, un problema de fase del saturador se puede confundir con un problema del EQ dinámico, porque todo se escucha junto desde el primer día — muy difícil de diagnosticar qué falla y por qué.

Construyendo de a una etapa, y validando cada una antes de sumar la próxima, cualquier problema nuevo que aparezca se puede atribuir con confianza a lo que se acaba de agregar, porque todo lo anterior ya se escuchó y midió por separado.

## Arquitectura base: pipeline de etapas intercambiables

- Cada etapa (saturador, EQ dinámico, multibanda automático, auto-EQ, resonancias, loudness matching) se implementa como un módulo de procesamiento con una **interfaz común** (algo del estilo de una clase base `AudioStage` con métodos `prepare(sampleRate, blockSize)`, `process(buffer)`, `reset()`).
- El plugin completo es, en el fondo, una **lista ordenada de estas etapas** ejecutadas en secuencia. Agregar una etapa nueva no debería requerir modificar las etapas anteriores, solo insertarla en la lista.
- Cada etapa tiene un **bypass individual** controlable (al menos en builds de desarrollo) para poder aislar su efecto sin comentar código ni recompilar distinto.

## Cómo escalar el saturador (caso concreto: mono-banda → multibanda)

1. **V0 — un solo saturador, full-band.** Una clase `Saturator` con una interfaz tipo `process(buffer)`. Un solo tipo de curva (por ejemplo, una genérica tipo tanh o la curva tipo tubo) aplicada a toda la señal, sin ningún split de bandas. Esto ya es un plugin "completo" en el sentido de que carga, procesa y suena — solo que simple.
2. **V1 — parametrizar el tipo de curva.** `Saturator` deja de tener una sola función de saturación fija y pasa a recibir una estrategia/tipo (`SaturationType: Warm | Tube | Diode | Tape`), delegando a una implementación distinta según el tipo seleccionado. Todavía full-band — esto solo separa "el carácter" como algo intercambiable, sin tocar el tema de bandas.
3. **V2 — multibanda real.** Se agrega un `BandSplitter` (el crossover, ver investigación de [saturación multibanda](saturacion-multibanda.md)) que separa la señal en N bandas — conviene arrancar con 2 o 3 bandas antes de ir directo a las 4 finales —, y se instancia un `Saturator` (la misma clase de V1, sin cambios) por banda, cada uno con su propio `SaturationType`. Se suman las bandas procesadas al final. **La clase `Saturator` no cambia entre V1 y V2** — lo que cambia es cuántas instancias hay y cómo se conectan alrededor (el splitter + la suma). Este es el punto donde se valida en la práctica el problema de fase/aliasing investigado aparte, con saturación real puesta, no en teoría.
4. **V3 — perilla de Énfasis.** Se agrega una capa de control que, a partir de un único parámetro (0 a 1, izquierda-derecha), ajusta los pesos/ganancias relativas de las N instancias de `Saturator` ya existentes desde V2 (y el balance de EQ asociado). No es una etapa nueva de procesamiento de audio — es lógica de control sobre instancias que ya existían.

La idea de fondo: **nunca se reescribe el saturador "para hacerlo multibanda"** — se reusa la misma pieza y se la instancia varias veces. Si la interfaz está bien pensada desde V0 (sin estado oculto que asuma "soy full-band"), escalar a N bandas es agregar un splitter alrededor, no reescribir el corazón del saturador.

## Mismo patrón para las demás etapas

Para EQ dinámico, multibanda automático, auto-EQ Hi-Fi, control de resonancias y loudness matching, el criterio es análogo:

- **V0 de cada etapa:** la versión más simple posible que "hace algo" — puede ser incluso una versión con parámetros fijos/manuales en vez de automáticos —, conectada al final del pipeline existente, con bypass.
- **Versión siguiente:** automatizar lo que en V0 era fijo, recién una vez que ya se escuchó y validó que lo que esa etapa recibe de entrada (el resultado de las etapas anteriores) es estable.
- **No conviene sumar dos etapas nuevas en la misma iteración**: cada etapa que se agrega cambia la señal que recibe la siguiente, así que hay que escuchar el resultado antes de seguir sumando.

## Orden de incorporación sugerido

1. Esqueleto de plugin VST3 (JUCE) en passthrough total (no toca el audio) + UI mínima de debug (ni siquiera circular todavía) — solo para confirmar que el build carga correctamente en un DAW real, en Windows y en Mac.
2. `Saturator` V0 (full-band, un solo tipo de curva) con un parámetro de cantidad/drive.
3. `Saturator` V1 (tipos de curva seleccionables) — todavía full-band.
4. `BandSplitter` + multibanda real, V2, arrancando con 2-3 bandas — acá se valida fase/aliasing real antes de ir a las 4 bandas finales.
5. Perilla de Énfasis, V3, sobre la estructura multibanda ya validada.
6. EQ dinámico tipo Bloom (etapa nueva, bypasseable, después del saturador ya estable).
7. Multibanda automático.
8. Auto-EQ Hi-Fi.
9. Control de resonancias tipo Soothe2.
10. Loudness matching (última etapa — depende de que todo lo anterior ya esté relativamente estable en nivel de salida para que el matching tenga sentido).
11. UI circular real (recién en este punto, una vez que el procesamiento interno ya funciona de punta a punta con una UI mínima de debug).

## Sistema de parámetros (para no romper todo al escalar)

- Usar **IDs de parámetro estables** desde el principio (en VST3/JUCE los parámetros se identifican por ID, no por posición en una lista) — aunque al principio haya pocos parámetros, conviene reservar rangos de IDs por etapa desde ahora (ej. 0–9 para el saturador, 10–19 para el EQ dinámico, etc.), para que agregar parámetros nuevos en el futuro no rompa presets ni automatización ya guardada en sesiones de prueba previas.
- Cada etapa expone sus propios parámetros de forma independiente, no como un único bloque global de parámetros, para que agregar una etapa nueva no requiera tocar el código de las etapas existentes.

## Testing por etapa

- Cada etapa nueva se valida aislada antes de sumar la siguiente: señales de prueba simples (seno, sweep, impulso) más material real, comparando con la etapa en bypass vs activa, escuchando y revisando espectro/fase.
- Mantener una build de desarrollo con bypass individual por etapa expuesto en una UI de debug (no necesariamente la UI final), para poder prender/apagar cada una sobre la marcha mientras se prueba dentro de un DAW real.

## Resumen del criterio general

No se construyen las 6 etapas a la vez ni se intenta tener las 4 bandas de saturación funcionando de entrada. Se arranca con la pieza mínima que ya es un plugin funcional (un saturador full-band), se valida, y se la escala (más tipos de curva, más bandas, control de énfasis) antes de recién ahí sumar la siguiente etapa de la cadena. Cada paso debe poder probarse de oído por separado antes de seguir.
