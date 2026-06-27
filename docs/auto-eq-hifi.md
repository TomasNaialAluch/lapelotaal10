# Auto-EQ "Hi-Fi" — investigación

Notas de investigación sobre qué curva objetivo usar como referencia "Hi-Fi" y cómo detectar enmascaramiento entre frecuencias para decidir dónde aplicar boost.

## 1. Definir la curva "Hi-Fi" objetivo

"Hi-Fi" no es un estándar técnico único — hay varias curvas de referencia conocidas en audio, cada una pensada para un contexto distinto. Hay que elegir (o construir) una concreta antes de implementar nada.

### Curvas de referencia existentes
- **Curva Harman:** desarrollada por Harman International a partir de estudios de preferencia de escucha (qué balance tonal prefiere la mayoría de los oyentes en pruebas ciegas). Tiene un leve realce en graves y una ligera caída suave hacia agudos. Es la referencia más citada en el mundo de auriculares/monitores de consumo orientados a "lo que la gente prefiere escuchar", no necesariamente a "lo que es plano/neutro".
- **X-Curve:** estándar usado en salas de cine (calibración de salas según normas SMPTE/ISO), pensada para grandes salas y un tipo de contenido específico (cine). Menos relevante como referencia para un plugin de mezcla/mastering orientado a música.
- **Curva plana/neutra (referencia de monitoreo profesional):** lo que se busca en monitores de estudio calibrados — sin énfasis ni caída deliberada. Es la base "neutra" contra la cual después se podría aplicar un leve perfil de preferencia (como el de Harman) si se quiere un resultado más "agradable" y no estrictamente neutro.
- **Curva propia ("house curve"):** definida por nosotros mismos, ajustando de oído sobre el resultado del resto de la cadena (saturación + EQ dinámico + multibanda), en vez de adoptar una curva externa pensada para otro contexto (auriculares de consumo, salas de cine, etc.).

### Qué tiene más sentido para este proyecto
Dado que el plugin ya satura y reacomoda dinámicamente el espectro en etapas anteriores, la curva "Hi-Fi" objetivo de esta etapa probablemente no deba copiarse literalmente de un estándar externo (Harman está pensada para el diseño de transductores de auriculares, no para procesar una señal ya saturada), sino:
- Partir de una curva neutra/plana como base teórica.
- Ajustar esa base con una inclinación suave tipo "house curve" (algo de realce en graves bajos y algo de aire en agudos altos, en la línea de lo que sugiere el perfil Harman como preferencia general) calibrada de oído sobre el resultado real de las etapas anteriores del plugin, no tomada prestada sin ajuste.

### A investigar/decidir puntualmente
- Conseguir los valores numéricos publicados de la curva Harman (la curva objetivo, no solo la descripción cualitativa) como punto de partida de referencia, aunque se termine ajustando.
- Definir la curva propia comparando el resultado del plugin (después de saturación + EQ dinámico + multibanda) contra mezclas/masters de referencia que se consideren "buen sonido Hi-Fi", y derivar de ahí los ajustes finos en vez de aplicar una curva externa a ciegas.

## 2. Detección de enmascaramiento entre frecuencias (referencia: Soundtheory Gullfoss)

### Qué hace conceptualmente Gullfoss
Gullfoss (Soundtheory) hace dos cosas relacionadas, ambas automáticas y sin bandas definidas por el usuario:
- **Realza contenido que está siendo enmascarado** por otras frecuencias (sonidos que "deberían" escucharse pero quedan tapados por energía cercana más fuerte).
- **Atenúa resonancias/acumulaciones** (de forma similar, conceptualmente, a lo que se busca en la etapa de control de resonancias tipo Soothe2 del propio proyecto — ver que hay solapamiento de objetivo entre esta etapa y esa).

Igual que con Bloom, el algoritmo interno exacto de Gullfoss no está publicado (es propiedad de Soundtheory) — lo que se puede investigar es el concepto de enmascaramiento auditivo en general y aproximarlo con técnicas conocidas.

### Cómo se modela el enmascaramiento auditivo (base teórica, no específica de Gullfoss)
- El enmascaramiento es un fenómeno psicoacústico bien documentado: un sonido más fuerte en una frecuencia (o banda) reduce la audibilidad percibida de sonidos más débiles en frecuencias cercanas. Es la misma base teórica que usan los códecs de compresión con pérdida (MP3, AAC) para decidir qué información "no se nota" si se descarta.
- Existen modelos psicoacústicos clásicos para esto, típicamente trabajando sobre bandas críticas del oído (escalas **Bark** o **ERB**, que dividen el espectro de forma similar a cómo lo resuelve el sistema auditivo humano, no de forma lineal en Hz) y curvas de enmascaramiento que describen cuánto "tapa" una banda fuerte a las bandas vecinas según la diferencia de nivel y la distancia en frecuencia.
- Para detectar enmascaramiento en una señal real en tiempo real, un enfoque conceptual razonable:
  1. Analizar el espectro en bandas perceptuales (Bark/ERB) en vez de bandas lineales.
  2. Por cada banda, estimar cuánta energía de bandas vecinas más fuertes está "tapando" esa banda, usando una función de propagación de enmascaramiento (modelo psicoacústico estándar, similar a los usados en codecs perceptuales).
  3. Si una banda está significativamente enmascarada por sus vecinas (su contenido "debería" ser audible pero queda tapado), es candidata a recibir boost; si una banda está generando enmascaramiento fuerte sobre sus vecinas sin aportar contenido relevante, es candidata a atenuación.

### Relación con las otras etapas del plugin
Vale la pena notar que esta etapa (detección de enmascaramiento + boost) y la etapa de control de resonancias (tipo Soothe2, al final de la cadena) atacan problemas relacionados desde ángulos opuestos: una busca realzar lo que está tapado, la otra busca atenuar lo que sobresale de forma resonante. Conviene diseñarlas teniendo en cuenta que comparten la misma base de análisis espectral perceptual, para no duplicar trabajo de DSP ni generar correcciones que se contradigan entre sí.

### A investigar puntualmente
- Revisar modelos psicoacústicos de enmascaramiento usados en codecs de audio perceptual (MPEG/AAC) como base teórica concreta y ya validada, en vez de partir de cero.
- Investigar escalas Bark vs ERB para decidir cuál usar como resolución de bandas perceptuales en el análisis.
- Definir cómo coordinar esta etapa con la de control de resonancias (¿comparten el mismo análisis espectral de base? ¿se calculan por separado?) antes de implementar ambas.

## Fuentes a revisar (buscar documentación/recursos vigentes, sin asumir URLs específicas)
- Publicaciones de Harman International sobre la curva de preferencia Harman (estudios de Sean Olive y equipo).
- Normas SMPTE/ISO sobre X-Curve para calibración de salas de cine (como referencia conceptual, no necesariamente aplicable a este proyecto).
- Página de producto de Soundtheory Gullfoss (para el comportamiento descripto por el fabricante).
- Literatura estándar de psicoacústica sobre enmascaramiento auditivo y bandas críticas (Bark, ERB) — es un tema clásico y bien documentado en libros de procesamiento de audio/psicoacústica.
- Documentación de modelos psicoacústicos usados en codecs perceptuales (MPEG/AAC) como referencia de implementación de detección de enmascaramiento.
