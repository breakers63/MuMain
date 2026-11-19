# Revisión gráfica de Pegasus 2.0

Revisión estática del 19 de octubre de 2026. Carpeta examinada: `C:/Users/WOoS/Downloads/Source Pegasus 2.0`. Se revisaron el proyecto compilable de SourceCliente, sus shaders y configuración, la copia ZIP de las fuentes y los componentes equivalentes del cliente SDL3. No se ejecutaron las DLL de Pegasus ni se cambiaron las opciones del cliente durante esta revisión.

## Resultado

Pegasus utiliza OpenGL: solicita un contexto **3.3 Compatibility** y conserva la ruta antigua si la actualización del contexto falla. Sus shaders son GLSL 330. No se identificó un backend Vulkan en el código del cliente revisado. La carpeta `OpenGL3/Shader` no es el sistema gráfico principal: el proyecto incluye explícitamente los módulos de `Graphics/OGL330`.

Sí se pueden aprovechar sus efectos para Vulkan, portando las operaciones gráficas a los shaders HLSL y a los pases SDL_gpu existentes. Copiar el `opengl32.dll`, los archivos GLSL o toda la interfaz no realiza esa adaptación.

**Hallazgo verificado:** `post.fs` y `world_lighting.fs` coinciden byte por byte con los archivos de `C:/Users/WOoS/Downloads/MuParisClient/MuParis/Data/Shaders/PegasusPost`. Por tanto, esta carpeta contiene una referencia directa del tratamiento de imagen del cliente OpenGL del usuario. La copia ZIP y los archivos extraídos coinciden también en PostProcess.cpp, WeatherSystem.cpp y NewUIOptionWindow.cpp.

## Qué aporta cada componente

| Componente | Comportamiento encontrado | Aprovechamiento en Vulkan |
|---|---|---|
| `post.fs` | Nitidez limitada al rango de los píxeles vecinos, exposición en espacio lineal, curva de contraste, saturación, gamma y tabla de color | Portar la cadena completa y sus valores, en lugar de aumentar únicamente nitidez y bloom |
| `look2.cube` | Tabla de color 33×33×33, titulada Exile2 Look | Convertir o remuestrear para el formato de tabla del cliente actual, que utiliza 32×32×32 |
| `bloom.fs` | Extracción por luminancia con umbral suave, reducción de resolución y desenfoque separable | Calibrar el sistema de bloom existente; no añadir otro brillo encima sin comparar el resultado |
| `world_lighting.fs` | Niebla y desenfoque distante basados en profundidad; aproximación de normales desde profundidad; color y brillo del mundo | Requiere un pase de mundo que lea profundidad, con la conversión de profundidad propia de Vulkan |
| `PostProcess.cpp` | Combina clima, brillo, luz direccional, color y niebla; fases noche/amanecer/mediodía/atardecer | Portar las reglas y conectar sus parámetros a las opciones actuales |
| `NCEmTOD` | Un perfil general y 26 perfiles de mapa | Reutilizar los datos tras comprobar la correspondencia de mapas |
| `SkyBox.cpp`, `SkySystem.cpp` | Cielo con seis imágenes, estrellas y cambios de color por hora y clima | Aprovechar imágenes y reglas; adaptar geometría, profundidad y recursos |
| `SurfaceWeather.cpp` | Acumulación, deshielo, huellas, humedad y protección de techos en Devias | Referencia útil, aunque el OpenGL del usuario tiene una versión de clima más reciente |
| `ZzzBMD.cpp` | Nieve sobre polígonos opacos orientados hacia arriba; excluye brillos y transparencias; sombras que siguen la luz del día | Portar de manera selectiva al render de modelos, sin blanquear alas ni efectos transparentes |
| `ZzzObject.cpp` | Sombras de objetos estáticos con límite de distancia y cantidad de emisores | Evaluar junto al sistema de sombras actual para evitar dibujar sombras duplicadas |

Fuentes principales:

- [Contexto OpenGL 3.3](<C:/Users/WOoS/Downloads/Source Pegasus 2.0/SourceCliente/source/Graphics/OGL330/OGL330Context.cpp:173>)
- [Proyecto y módulos realmente incluidos](<C:/Users/WOoS/Downloads/Source Pegasus 2.0/SourceCliente/Main.vcxproj:329>)
- [Corrección final de imagen](<C:/Users/WOoS/Downloads/Source Pegasus 2.0/Client/Data/Shaders/PegasusPost/post.fs:1>)
- [Niebla, iluminación y profundidad](<C:/Users/WOoS/Downloads/Source Pegasus 2.0/Client/Data/Shaders/PegasusPost/world_lighting.fs:1>)
- [Respuesta visual al clima](<C:/Users/WOoS/Downloads/Source Pegasus 2.0/SourceCliente/source/Graphics/OGL330/PostProcess.cpp:427>)
- [Nieve en modelos](<C:/Users/WOoS/Downloads/Source Pegasus 2.0/SourceCliente/source/ZzzBMD.cpp:1631>)

## Climas y audio

Pegasus contiene despejado, lluvia, tormenta, nieve, ventisca y niebla. Su configuración permite densidad, viento, intensidad, transición, relámpagos, cambio automático y sustitución de lluvia por nieve en mapas fríos.

La implementación examinada de precipitación es anterior a la del OpenGL del usuario: dibuja lluvia con líneas y nieve con puntos suavizados; no usa la versión de copos texturizados y salpicaduras examinada anteriormente en `MU-Paris/client-publish`. Por ello no conviene sustituir todo el sistema Vulkan por WeatherSystem.cpp de Pegasus.

La combinación de cielo, luz, color y visibilidad es más completa que la adaptación meteorológica actual de Vulkan. Esa combinación explica parte de la diferencia visual incluso cuando las partículas tienen trayectorias parecidas.

No se encontró en estos dos módulos meteorológicos la reproducción de las pistas lluvia.mp3/tormenta.mp3/trueno.mp3. Esos archivos tampoco están en `Client/Data/Music` de esta copia. `SurfaceAudioProfiles.txt` describe materiales para pisadas y empieza sin regiones definidas; no es un mezclador de sonidos de clima. Para esos sonidos la referencia útil sigue siendo el OpenGL del usuario, cuyos archivos ya se verificaron por hash.

## Opciones, HD y Ultra HD

El menú de esta copia declara tres pestañas: **Común, Anti-Lag y Opciones**. Incluye niebla, anisotropía, filtro lineal, resolución y FPS. No se encontraron en ese código los botones Clásico/Moderno/Ultra HD ni una pestaña Shaders equivalente a la que tiene el cliente actual. Los parámetros de los efectos se leen principalmente desde config.ini.

La lista de resoluciones del menú incluye hasta 1920×1080. El nombre Ultra HD no demuestra por sí solo resolución 4K ni texturas de mayor resolución. En este proyecto la mejora visual proviene sobre todo del tratamiento de imagen, iluminación, cielo y filtrado.

El render principal del postprocesado usa RGBA8 y profundidad de 24 bits; el bloom usa un destino RGBA16F. Esto no constituye una cadena de escena HDR completa.

Además se incluye `opengl32.dll` de QeffectsGL con su configuración. Esta solicita anisotropía y multisample 16×, corrección de color y Emboss. Bloom, SSAO, DOF y MotionBlur están desactivados en ese INI. El registro histórico de QeffectsGL confirma corrección de color y Emboss activos, pero no demuestra cómo se ve la carpeta hoy en esta PC. No se identificó la fuente de ese wrapper entre los módulos examinados. No es una DLL Vulkan reutilizable.

No deben copiarse los valores de config.ini a ciegas: las cuatro fases de brillo del día están configuradas en cero y el código las limita a mínimos distintos. Los registros incluidos muestran brillo diurno reducido a 0.30. Copiar esos valores podría oscurecer demasiado nuestro cliente.

## Diferencias verificadas respecto al SDL3 actual

- El shader final de Vulkan tiene nitidez, saturación, bloom y una tabla de color opcional, pero no reproduce la cadena completa de exposición/contraste/gamma de Pegasus.
- En `src/bin/Data/Grading` solo se encontró `World2.png`. Los mapas sin tabla no reciben esa corrección de color. Por tanto, seleccionar Ultra HD no puede reproducir automáticamente `look2.cube` en todos los mapas.
- El selector actual de calidad cambia porcentajes de nitidez, niebla, bloom y atmósfera y habilita posprocesado. No carga los perfiles horarios y de mapa de Pegasus.
- La niebla actual se aplica durante el dibujo de superficies; Pegasus también tiene una composición del mundo que usa profundidad. Adaptar esa composición requiere respetar transparencias, brillos y las convenciones de profundidad de Vulkan.
- El bloom del cliente SDL3 ya distingue efectos y conserva el HUD fuera del posprocesado. Pegasus termina con un pase de corrección que también alcanza al HUD. Es preferible preservar la separación actual para no alterar textos e interfaz.

## Orden de adaptación propuesto

1. Reproducir exposición, contraste, nitidez limitada y tabla de color, con comparación en el mismo mapa, cámara y resolución.
2. Calibrar bloom y las cuatro fases de luz con los perfiles de Pegasus.
3. Adaptar niebla y atmósfera por profundidad, validando luces, alas, partículas y efectos del suelo antes de añadir desenfoque distante.
4. Incorporar cielo por mapa y, después, nieve sobre materiales opacos y sombras dependientes de la hora.
5. Conectar cada mejora a **las opciones actuales** de Clásico/Moderno/Ultra HD, conservando controles, volumen, duración de climas y posibilidad de reducir efectos.

El primer objetivo verificable debe ser igualdad visual con el OpenGL del usuario en una escena fija. El nombre del preset y el número de efectos habilitados no sustituyen esa comprobación.

Esta revisión identifica componentes reutilizables y trabajo pendiente; no afirma que ya estén portados ni validados visualmente en Vulkan.
