# PARIS SDL3: interfaz dorada y mapas OpenGL

El cliente usa SDL3 GPU con los paneles y recursos de PARIS. Stats, Inventario, barra inferior, Mascotas, ampliaciones, tienda personal, baúl, Chaos, Trainer y Charon usan los recursos dorados adaptados del cliente OpenGL.

## Abrir la compilación de prueba

Inicia `out/paris-delivery/runtime-gold-final/Jugar.exe`. Usa Jugar.exe para conservar el arranque y la conexión habituales. La carpeta de prueba es independiente de los clientes anteriores.

## Mapas y entorno

Se sincronizaron los recursos World y Object del cliente OpenGL distribuido: pisos, alturas, luces, colocación de objetos, modelos, texturas y minimapas. También se incluye su música. La selección de servidores utiliza el puerto con barcos de World95 y su cámara fija; la selección de personajes conserva World75.

Los niveles de Blood Castle, Chaos Castle, Kalima e Illusion Temple conservan sus carpetas compartidas. Los recursos adicionales del paquete no crean mapas nuevos en el servidor: siguen disponibles los destinos configurados por el juego.

Pantalla completa utiliza el tamaño del escritorio para llenar monitores anchos sin cambiar a un modo 4:3. En modo ventana se conserva la resolución seleccionada.

## Menú CUSTOM

El panel grande CUSTOM muestra los dieciséis sistemas de OpenGL en una sola página: Add Stats, Jewel Bank, Reset, Eventos, Info Character, Market, Rank User, VIP, Battle Pass, Guias, Kill Log, Post Item Log, Mentoria, Panel GM, Soul System y Subastas. Inventario, Mascotas, Tienda personal, Chaos y Quest conservan sus accesos habituales fuera de este menú. Las operaciones dependen de los datos, permisos y funciones disponibles en el servidor; el panel GM no concede permisos adicionales.

Rank User utiliza la última interfaz de OpenGL: pestañas Reset, Clanes y Misiones, búsqueda, fichas de miembros y vista del equipo. Mi equipo permite solicitar al servidor el cambio entre público y privado. Ctrl sobre un jugador muestra su ficha si el servidor publica sus datos.

Battle Pass muestra seis columnas de niveles con desplazamiento horizontal y rueda, recompensas gratuitas y premium, estados de bloqueo y reclamación. Activar Premium abre la confirmación con los requisitos y la fecha publicados por el servidor. El cliente espera la respuesta del servidor al reclamar o activar.

Kill Log y Post Item Log conservan los eventos recibidos durante la sesión. El registro de artículos reconoce los mensajes con metadatos de item; no reconstruye mensajes ni muertes ocurridos antes de abrir el cliente.

- Jewel Bank permite búsqueda, desplazamiento, ingreso/retiro y Auto Pick; espera los saldos del servidor antes de operar.
- VIP y Reset muestran las condiciones y cotizaciones recibidas del servidor.
- L abre Add Stats; H abre Eventos; F1 abre las guías en el navegador.
- Chaos Mix requiere hablar con el Chaos Goblin. Trainer, Charon y Baúl se abren mediante sus NPC habituales.

Inventario expandido muestra las ampliaciones habilitadas por el servidor. Las secciones restantes aparecen bloqueadas, sin fragmentos de marco superpuestos.

## Opciones y shaders HD

Opciones usa el panel dorado ampliado. Conserva sonido, música, ayuda, límite de efectos, fuente, idioma, resolución y modo ventana. Rendimiento permite reducir partículas, ambiente, aves/peces, sombras y brillo del equipo por separado. La sincronización vertical se aplica al instante.

Shaders HD ofrece perfiles Clásico (sin postprocesado), Moderno y HD, además de controles de color/contraste, nitidez, bloom y exposición. Su estado se conserva en config.ini. Estos efectos son postprocesado de SDL3 GPU; no sustituyen texturas ni añaden mapas. Si el equipo no puede crear el efecto, el cliente conserva la imagen normal y muestra que HD no está disponible.

Los skills desplegados tienen borde dorado. Su botón usa el mismo espacio de pantalla para dibujar y recibir el clic; el área de tienda X ya no se superpone al botón de poderes.

## Tienda personal

El campo Nombre queda dentro del panel. Al poner precio a un objeto aparece la ventana de venta con vista del artículo y filas para Zen, Paris Coins, Bless, Soul, Life, Creation y Chaos. Introduce un importe positivo en una sola moneda; se rechazan cantidades negativas, varias monedas a la vez y números mayores de 2.147.483.647. Las monedas adicionales necesitan la capacidad compatible anunciada por el servidor; en caso contrario sólo se acepta Zen.

Los precios y sus monedas se conservan al mover objetos dentro de la tienda y en la información del artículo. Los botones inferiores permiten cerrar el panel, abrir/cerrar la tienda y solicitar tienda offline. La tienda offline exige una tienda abierta y una confirmación antes de enviar la solicitud al servidor.

## Baúl y otros paneles

El baúl y su ampliación tienen cuadrícula y controles dorados. Se conservan guardar/retirar Zen, contraseña, bloqueo y movimientos de objetos. La ampliación se muestra cuando la cuenta la tiene habilitada. Chaos y Trainer conservan las recetas del juego; la rueda permite recorrer sus detalles. Mascotas conserva la información de Dark Horse y Dark Raven.

## Instalar la actualización

Cierra el cliente SDL3, conserva una copia de Main.exe y de los recursos que vas a reemplazar y extrae el ZIP en su carpeta raíz. El paquete actualiza el ejecutable, bibliotecas x64, shaders, fuentes, recursos dorados y archivos de mapas nuevos o distintos. Requiere los demás datos de tu cliente actual.

Conserva tu Jugar.exe y config.ini. El paquete no incluye datos de cuenta ni cambia la configuración del servidor.

## Validación

La compilación Release x64 y la protección del motor SDL3 pasaron. Las pruebas de precios, banco, metadatos, Battle Pass y atributos suman 25 casos y 364 comprobaciones correctas. Se revisaron los contratos de copia de recursos y opciones de pantalla.

Los archivos de mapas y música se comparan por SHA-256 con los del cliente OpenGL. Las texturas de mapas revisadas caben en los límites admitidos. La compilación y la coincidencia de archivos no sustituyen recorrer cada mapa ni probar cada operación en una partida.

Para volver a sincronizar recursos, usa `tools/sync_paris_maps.py --source <cliente-OpenGL/Data> --apply`. El comando respalda los archivos distintos antes de reemplazarlos. Sin --apply sólo informa las diferencias.

Para probar Vulkan en Windows abre Jugar-Vulkan.cmd, junto a tu Jugar.exe habitual. Este acceso selecciona Vulkan solo para esa ejecución. En Opciones > Shaders HD puedes elegir HD y ajustar nitidez, bloom y exposición. La prueba de arranque confirmó Vulkan y los shaders disponibles; la calidad y los FPS deben comprobarse dentro de los mapas. El postprocesado no convierte los recursos originales en texturas HD ni implementa HDR de pantalla.

## Capa, modelos custom y NPCs

El menú CUSTOM usa botones de borde fino, iconos sin textos incrustados y una sola etiqueta por botón. Info PJ abre la información del personaje; Post Items abre su historial.

La capa pequeña de Dark Lord usa Wing50 y la textura NBCcape de OpenGL, con su tamaño y física adaptados. Los NPCs y monstruos de Data/Custom/Monsters.txt conservan nombre, escala y tipo; los IDs configurados mayores de 1023 se reciben completos. Las configuraciones y recursos custom se incluyen junto con los modelos y texturas de Item, Player, Monster y NPC de la base OpenGL. No basta con copiar Main.exe: hay que aplicar también la carpeta Data del paquete.

La comprobación de archivos encontró los 104 modelos custom configurados y las 779 referencias de modelos de ítems. Esto comprueba los archivos, no su apariencia en cada mapa. La prueba en partida de capa, NPCs y efectos sigue siendo necesaria.

## Ajustes de NPCs y HUD

Chaos y las dos ventanas de Trainer usan el marco y botones dorados. Charon y Titus ajustan sus explicaciones al ancho interior; Merchant mantiene su cuadrícula y usa el mismo marco dorado. Los importes y la comisión del baúl quedan dentro de sus casillas. El marco CUSTOM conserva la transparencia exterior.

El GameMaster (NPC 378) se clasifica como NPC y utiliza su apariencia de GM. Los iconos de poderes mantienen el recorte original de su textura al dibujarse a otro tamaño. El selector también distingue entre posiciones de skills y comandos de mascotas. Con la lista de poderes abierta, pasa sobre un poder y usa Ctrl+número para asignarlo.

## Menú, métricas, Helper e invasiones

MENU abre CUSTOM desde el recuadro izquierdo del HUD; debajo se muestran las coordenadas actuales. H abre la configuración de MU Helper y el botón inferior inicia o detiene su funcionamiento. Las dos barras junto al maná indican calidad de FPS y ping: verde, naranja o rojo; al pasar el cursor aparece el valor. El ping usa la conexión TCP activa en Windows; cuando no puede medirse se indica -- y no se inventa un valor.

ACTIVE INVASION recibe los avisos reales del servidor. Permite cambiar entre invasiones, ver el tiempo y las cantidades, desplazar la lista con la rueda y pasar sobre un monstruo para ver su modelo. Si un modelo no tiene una vista previa disponible, se indica en el panel. La X cierra la lista. Los mensajes técnicos AINV ya no se imprimen en el chat.

Stats separa el nombre del adorno y muestra los puntos en una casilla propia, sin montarlos sobre Resets. Sus iconos inferiores son Cerrar, Quest, Mascotas y Opciones visuales. El árbol de habilidades ya no se abre desde ese botón.
