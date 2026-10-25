# Paris Dorado — RmlUi

La nueva base esta en `C:\Users\WOoS\MuMain-Paris-RmlUi`. El cliente anterior
permanece en `C:\Users\WOoS\SrcLadoClienteMuMain-SDL3`; no sustituirlo hasta
comprobar el acceso mediante Jugar.exe y las funciones del servidor.

## Tema

Seleccionar **Paris Dorado** en Opciones → UI → Tema. Tambien se puede probar
durante la sesion con `$theme paris-gold`. La plantilla nueva lo selecciona por
defecto; una configuracion existente conserva su eleccion.

El tema hereda Legacy y utiliza el marco original de Paris con transparencia.
Stats mantiene el nombre debajo de la corona, los puntos separados del nivel
(incluso cuando son cero) y Cerrar, Quest, Mascotas y Opciones visuales.

## Estado de la adaptacion

### Jewel Bank: cantidades de transferencia

El banco mantiene filas de piedra alternadas y cantidades centradas debajo de
Inventario y Banco. Al pasar el cursor sobre una joya aparecen tres columnas:
**x1**, **x10** y **ALL**. Los botones **+** depositan y los botones **−** retiran
la cantidad de su columna directamente, como en OpenGL. ALL deja que el servidor
calcule el total posible segun el saldo, la capacidad del banco y el espacio del
inventario. Los controles esperan la respuesta antes de permitir otra operacion;
una respuesta incompleta requiere actualizar el banco antes de transferir.
La casilla Auto Pick y el boton de cierre permanecen al pie del panel.

Para compilar esta copia en Windows, `build-paris.cmd` fija la salida del
compilador en espanol con codificacion UTF-8. Esto permite que Ninja detecte los cambios en cabeceras y
recompile tambien las ventanas que las usan; una mezcla de objetos anteriores
y actuales puede cerrar el cliente al abrir un sistema custom.

### Estado general

- Base completa de sven-n/MuMain: `16361747a146b430a18a6b86f87409122e2397b1`.
- Tema Paris Dorado, marcos de ocho paneles de inventario/tiendas, Stats y HUD:
  primera adaptacion preparada.
- Helper: configuracion, inicio/parada y coordenadas trasladados al HUD en el tema.
  Las acciones siguen usando el Helper de la base nueva. Configuracion y detalle
  heredan el marco dorado, con titulo y pestanas adaptados.
- El codigo de monstruos custom y la capa pequena compilaron dentro de `MuClient.lib`.
  La nueva configuracion `Data/Custom/Monsters.txt` contiene 104 filas (103 IDs
  unicos) y todos sus modelos referenciados estan presentes.
- Se importaron 1,722 recursos de custom, monstruos, NPC y capa desde el otro
  SDL3. El ejecutable y su carpeta de ejecucion aun no se actualizaron con esos
  recursos.
- No se han borrado ni sustituido las carpetas `World*` y `Object*`; este trabajo
  conserva los mapas actuales.
- Los 11 documentos RmlUi Paris vuelven a pasar la generacion de vistas previas
  a 1024x768 y 800x600; se revisaron visualmente VIP, Reset, Jewel Bank,
  Subastas y Eventos GM. Pasan tambien 30 pruebas de contratos y 20 pruebas de
  previsualizacion. Esto verifica las hojas y estilos, no la entrada de RmlUi,
  los recursos 3D ni las operaciones de servidor.
- Los controles estaticos de RML/RCSS, sus contratos entre temas y la geometria
  pasan: 165 documentos y 271 hojas RCSS revisados; 122 contratos en 488
  variantes de tema.
- `MuClient.lib` recompila con los cambios recientes y el enlace completo de
  Release acaba de pasar con `Main-check.exe`. El build normal de `Main.exe`
  vuelve a detenerse al enlazar porque el proceso PID 22240 mantiene abierto ese
  ejecutable; no se debe cerrar ese proceso sin preservar la sesion del usuario.
- El registro de la sesion anterior confirma que Reset abrio el documento Rml y
  envio su consulta; el servidor no devolvio estado y no hay evidencia de que
  ese proceso se haya cerrado al abrir Reset. Jewel Bank y la respuesta del
  servidor aun requieren prueba con el ejecutable actualizado.
- Los 13 sistemas Paris ya aparecen en CUSTOM y usan la capa de protocolo del
  servidor. Jewel Bank, Market, Ranking User, Subastas e historiales Kill Log /
  Post Items tienen ventanas RmlUi propias; Ranking User recupera las pestañas
  de resets, gremios y quests del ranking Paris nuevo. Reset y VIP tambien
  tienen ventanas doradas propias con datos recientes del servidor y acciones
  confirmadas. Battle Pass tiene temporada/progreso en vivo y reclamos sujetos
  a estado y token actual; Eventos muestra pestañas y horarios con cuenta
  regresiva actualizada. Mentoria, Panel GM y Soul System siguen usando el
  dialogo RmlUi de acciones. Las operaciones con servidor aun requieren prueba
  con la cuenta real.
- Battle Pass permite reclamar recompensas habilitadas y el servidor valida la
  temporada y el nivel. GM ofrece los controles de evento disponibles; Soul
  permite solicitar el reinicio cuando el servidor lo autoriza; Mentor acepta o
  rechaza solicitudes entrantes; Subastas tiene pestañas de publicaciones,
  inventario, ofertas, entregas y subastas propias, con acciones para publicar,
  ofertar, comprar, cancelar o recoger cuando el servidor las admite.
- El menu CUSTOM ahora usa el marco dorado y los iconos originales del menu
  OpenGL, con sus 16 entradas en dos columnas. Falta comprobar su entrada en el
  cliente actualizado. Active Invasion con vista del monstruo y los indicadores
  FPS/ping del HUD siguen pendientes de migrar completamente a RmlUi.
- Jewel Bank abre Inventario y oculta Stats. La busqueda y el estado inferior
  quedan centrados, con una letra mayor. Mantiene las columnas x1/x10/ALL.
- Info PJ abre automaticamente Stats y se acopla a su izquierda. Reutiliza
  el marco Paris, las filas de 15px, las secciones, la barra dorada y los
  totales del OpenGL. Acepta estadisticas v3 (60 campos) y v2 (52 campos);
  los datos nuevos no publicados por un servidor antiguo muestran "--".
- Battle Pass reutiliza su panel, etiquetas Free/Premium, monedas, candados,
  comprobantes y barra de progreso originales. Muestra seis niveles por pagina,
  permite recorrerlos horizontalmente y dibuja los objetos con la camara de items.
  Reclamos y activacion Premium esperan la respuesta del servidor sin repetir
  operaciones de compra. La activacion conserva el dialogo de requisitos original.
  Falta comprobar ambos sistemas dentro de la partida con esta compilacion.
- Inventario, inventario expandido, tiendas, NPC, Chaos, Trainer y Vault:
  pendientes de completar su presentacion dorada y comprobar sus acciones.
- Los mapas actuales se conservan: no se borran ni se sustituyen carpetas de
  terrenos u objetos como parte de esta adaptacion.
- La mini capa usa ahora su BMD y textura propios, y el tejido se ajusta a su
  tamano; falta comprobarla en una partida. Los NPC y modelos custom tambien
  quedan pendientes de comprobacion dentro del juego.

## Comprobacion antes de entregar

Abrir siempre mediante Jugar.exe. Probar conexion, seleccion de personaje,
inventario, equipamiento, tiendas, combinaciones y cada operacion custom contra
el servidor. Verificar el tema en ventana ancha y con escalas de UI distintas.
Los controles estaticos de RML/RCSS y las vistas previas no verifican las
operaciones del servidor ni el resultado del motor Vulkan.

La sintaxis, los contratos de documentos, la geometria y el uso de transformaciones
pasan los controles estaticos. Hay vistas previas de Stats a 800x600 y 1920x1080;
no sustituyen la comprobacion en el juego ni simulan la colocacion del workspace.

Las carpetas antiguas `out/paris-delivery/runtime-*` comparten algunos archivos
Data identicos mediante enlaces duros para ahorrar espacio. Para cambiar un
archivo en una sola copia, escribir uno nuevo y reemplazarlo; no editarlo en el
mismo archivo. Los arboles de codigo fuente no se enlazaron.

## Ajustes de paneles custom (18 de octubre)

- Repartir puntos se abre desde Add Stats y desde el cuadro Puntos del personaje.
  Conserva las cinco filas del original, Cantidades, Porcentajes, Lleno y reparto
  automatico. Comando aparece como Solo DL para las otras clases. Cada asignacion
  espera confirmacion antes de enviar la siguiente; un timeout detiene el reparto.
  Los limites del servidor llegan al controlador activo, sin el receptor antiguo
  que descartaba esas respuestas.
- Stats usa solo los bordes del atlas: los nombres y valores se dibujan una vez.
- Info se coloca junto a Stats. Se amplio el espacio de los scrolls de Info y
  Jewel Bank para que el tirador quede completo. Se corrigieron las zonas de las X.
- Battle Pass usa los archivos de imagen originales para barra, Free/Premium y
  candados; el scroll horizontal muestra la proporcion de niveles visibles.
- Market conserva su composicion original, filtros, cinco publicaciones, vista
  previa del objeto, compra con confirmacion y acceso a publicar desde la tienda.
- Mentoria tiene los fondos originales para Novatos, Buscar, Recompensas e
  Historial; retratos, solicitudes, paginacion y confirmacion de canje. Las acciones
  usan la respuesta y el token del servidor; no se reintentan los canjes.

La compilacion Release y los controles de documentos pasan. Las pruebas puras
suman 11 casos y 335 comprobaciones. Una comprobacion con RmlUi nativo confirma
las cinco filas de reparto, filas de mentores, tarjetas de recompensas y sintaxis
valida de sprites. Las capturas de la partida detectaron y permitieron corregir
las filas superpuestas, los fondos blancos y el texto duplicado; la ultima
revision dentro de la partida sigue pendiente. No se efectuaron compras,
canjes ni asignaciones de puntos para comprobar la presentacion.

### Legibilidad y comparacion con las capturas originales

Repartir puntos utiliza texto rasterizado al tamano final de pantalla, con los
numeros de las casillas en blanco. Los botones + de Stats permanecen visibles
con cero puntos; no gastan puntos hasta que haya saldo disponible. Los nombres
se separan del borde izquierdo. Mentoria centra las etiquetas en cada campo,
columna y boton, incluido el historial.

El tirador de Battle Pass cambia de ancho al recibirse la temporada y conserva
el arrastre; se verificaron cambios del ancho completo al 24% y al 50%, avance
con teclado y arrastre hasta el ultimo nivel. Los controles inferiores recuperan
el degradado y las letras blancas de OpenGL.

Market utiliza sus controles y marcos originales, precios con separadores,
detalle de categoria, inicial del vendedor y los controles inferiores dentro del
panel. Las vistas 3D encajan el objeto completo y centran su silueta dentro de
cada marco. Estos ultimos cambios requieren una revision visual en la partida;
la vista previa estatica no dibuja los modelos 3D.


### Market, venta, registros y Soul System

Market muestra la ficha completa del objeto al pasar el puntero por una
publicacion o por su vista previa. Categoria, moneda y orden actualizan la
busqueda al seleccionarlos. La busqueda por nombre mantiene su boton Buscar.
Los nombres de las monedas corresponden a sus codigos: Life usa 4 y Chaos 9.

Al poner precio a un objeto aparece Vender articulo, con vista previa y campos
para Zen, Paris Coins, Bless, Soul, Life, Creation y Chaos. Se admite una sola
moneda con cantidad positiva; letras, cantidades negativas, cantidades mayores
que 2.147.483.647 y varias monedas simultaneas se rechazan. Las monedas adicionales
solo se habilitan cuando el servidor anuncia que las acepta. Cancelar o pulsar
Escape devuelve el objeto al inventario. El boton no abre por si solo la tienda:
continua el mismo proceso de publicacion del cliente original.

Kill Log y Post Item Log conservan cuatro registros visibles, busqueda inmediata,
contador, paginas y desplazamiento con rueda, teclado y arrastre. Derrotas excluye
las victorias; Post muestra los objetos publicados en esta sesion y permite
consultar su ficha al pasar el puntero. Se conservan como maximo 128 registros.

Soul System abre sus cuatro categorias, con cuatro habilidades por categoria,
controles -10/-1/+1/+10, puntos disponibles, progreso e hitos bloqueados o
habilitados. Aceptar envia el borrador; Cancelar restaura los rangos recibidos.
Reiniciar exige confirmar el coste informado por el servidor. Los aumentos usan
el coste actual; al reducir un rango se devuelve la proporcion del precio que
se pago originalmente: pagado - pagado * rangos_devuelto / rango_original.
Los hitos se habilitan con rangos confirmados por el servidor, no con el borrador.
Si falta una respuesta, se consulta el estado una vez y no se reenvia la operacion.

Estas ventanas se centran sobre el juego, conservan la proporcion del arte
original y utilizan la misma escala de interfaz. Se verificaron sus modelos y
controles en RmlUi; la comparacion final dentro de la partida sigue pendiente.

Las miniaturas de Post Item mantienen su actualizacion aunque su textura todavia
no este lista, de modo que vuelven a mostrarse al completarse. El puntero sobre
la fila abre la ficha del objeto con los datos conservados en el post. Los
controles Anterior, Siguiente y Cerrar conservan etiquetas visibles centradas.


### Rank User y Eventos

Rank User presenta RESET, CLANES y MISIONES dentro del mismo marco de OpenGL.
La lista de jugadores y clanes permite buscar por nombre, cambiar de pagina y
seleccionar una ficha. RESET incluye el personaje, las doce casillas de equipo
y la ficha completa del objeto al pasar el puntero. MI EQUIPO permite cambiar
la visibilidad usando la preferencia confirmada por el servidor.

CLANES muestra el emblema, master, puntuacion, miembros y seis miembros por
pagina. Seleccionar un miembro actualiza su retrato, nivel, resets, atributos
y estado. Los atributos solo aparecen cuando el servidor permite compartirlos.

MISIONES usa el catalogo del servidor, sus objetivos, premios, miniaturas de
monstruos y avance actual. Se puede buscar y desplazar la lista; seleccionar
una mision consulta su ranking. No se aceptan ni se cobran misiones al abrir
este panel. Los relojes de Eventos mantienen cinco paginas y las columnas
Evento, Restante y Estado, con filas alternadas y cierre mediante H o ESC.

La interfaz se comprobo con listas, equipo, clanes y misiones de prueba en el
motor RmlUi. Las respuestas de equipo, miembros y misiones se validaron con
paquetes correctos, incompletos y de otra seleccion. Queda pendiente comparar
los modelos 3D y los datos reales dentro de la partida.

Soul muestra el progreso del borrador al ajustar los puntos y anima las almas de las esferas. Eventos abre en el borde derecho y permite arrastrar la cabecera. El boton MENU abre directamente Sistemas Paris.

Las esferas de Soul tienen particulas orbitales con estelas: azul helado, luz dorada, remolino plateado y brasas ascendentes segun la ilustracion. La intensidad aumenta al asignar rangos.

Rank User presenta el personaje de frente. Las fichas de clan y las misiones mantienen textos y premios dentro de sus casillas, sin repetir la cantidad del premio.

Las miniaturas de equipo del Ranking conservan su imagen hasta cambiar de personaje, recibir equipo nuevo o cambiar de tamano. La vista del personaje mantiene su animacion continua.

### Subastas y Panel GM
Subastas utiliza el menu lateral, buscador, filtro de categoria y tabla del cliente OpenGL. Sus cinco secciones conservan ofertas, historial, entregas y cancelacion, con confirmacion antes de publicar, comprar u ofertar. Las miniaturas permiten consultar el objeto al pasar el puntero. El precio minimo y las monedas disponibles proceden del servidor.
Panel GM conserva el arte original y los diez accesos. Los siete eventos conectados muestran estado, inscritos, tiempo, mapa y requisito recibidos del servidor; los otros tres se muestran pendientes y no ejecutan acciones. El anuncio se revisa antes de enviarse. Buscar filtra sin cambiar la posicion de los iconos originales.
Las vistas de personajes y monstruos avanzan con un reloj propio a 25 cuadros de animacion de referencia por segundo real, sin alterar la animacion del mundo. Al reabrirse una vista tras una pausa larga no acumula el tiempo oculta. Los monstruos mantienen su propia accion y repiten la animacion de reposo.

Repetir la pestana activa de Mentoria conserva los datos visibles sin iniciar otra carga. CUSTOM deja espacio bajo el adorno superior antes del titulo y los botones.

El HUD Paris muestra las coordenadas actuales en el recuadro inferior izquierdo. MENU tiene fondo oscuro y borde dorado y abre Sistemas Paris.

Los recursos de mapas World/Object se sincronizaron con el cliente OpenGL de referencia, incluidos World85/Object85 y World95/Object95. El servidor conserva el control de accesos y destinos de movimiento.

Las coordenadas se dibujan sobre el fondo del HUD. Al cambiar de pestaña, Mentoria conserva su marco y muestra la espera en el pie sin tapar la ventana con un panel negro.

Add Stats abre la ficha de personaje con Repartir puntos a su izquierda. Al solicitar una ventana nativa, los paneles custom abiertos se cierran para dejar visible la ventana solicitada. El menu CUSTOM deja separados el titulo, subtitulo y su cuadricula completa.

En Opciones, Clima permite activar lluvia, tormenta, nieve, ventisca, niebla y el ciclo de dia y noche, con duraciones independientes. La nieve se acumula gradualmente sobre el terreno, deja huellas al caminar y se desvanece al terminar el clima. Shaders ofrece Clasico, Moderno y Ultra HD: los perfiles cambian nitidez, color, brillo y cantidad de particulas, conservando las otras preferencias graficas. La eleccion del posprocesado se guarda para la siguiente apertura del cliente.

La nieve de Vulkan cubre progresivamente el suelo alrededor del jugador como en OpenGL, conservando el relieve y las zonas protegidas por techos. Las huellas alternan los pies, aparecen al caminar sobre la nieve y se rellenan mas rapido mientras sigue nevando. La cobertura se derrite gradualmente al terminar la nevada.

Los copos de nieve usan la textura suave y las trayectorias del cliente OpenGL: oscilan, giran y cambian de tamano y opacidad. La ventisca aumenta la velocidad y el viento; los mapas de nieve tienen mayor densidad. La cobertura del terreno y las huellas se conservan.

La cobertura de nieve es mas blanca, manteniendo visible el relieve. Los copos se orientan hacia la camara activa para conservar su forma suave al girar o mover la vista.

La lluvia y las tormentas usan gotas mas visibles, orientadas hacia la camara, con inclinacion y velocidad del cliente OpenGL. Tormenta reproduce lluvia y truenos; la pista de viento de nieve queda reservada para nieve y ventisca. Todos respetan el volumen de Sonido y dejan de sonar al entrar en una zona protegida o desactivar el clima.

La niebla de distancia y de tormenta conserva la forma de luces, alas, antorchas y efectos del suelo; los brillos no se rellenan con el color de niebla ni forman cuadros al activar los climas.

Shaders incorpora Cielo Paris Project, Color Paris Project y Bloom con interruptores independientes y guardado automatico. Clasico los desactiva; Moderno usa una correccion suave y Ultra HD usa exposicion 1.10, contraste 1.18, saturacion 1.10 y mezcla de la tabla Pegasus al 20%. Los cambios afectan al mundo y preservan el HUD. El cielo usa las seis caras originales en Lorencia, Dungeon, Devias, Noria, Lost Tower y Tarkan; otros mapas exteriores usan un fondo con colores de cielo. El ciclo dia/noche modifica cielo y estrellas; lluvia, nieve, niebla y relampagos modifican su color. Estrellas conserva su interruptor en Clima. La atmosfera por profundidad, las sombras horarias y la nieve en modelos de Pegasus no forman parte de esta primera adaptacion.

Los nombres visibles de los efectos adaptados son Paris Project. La procedencia tecnica de los recursos queda documentada en la revision de fuentes.

El inventario expandido Paris Project usa el fondo de piedra y la cuadricula del OpenGL, con paginas bloqueadas numeradas. Las dimensiones de los items se sincronizan con el catalogo ingles del cliente OpenGL y su lista explicita de pergaminos 1x1; solo cambian ancho y alto, sin modificar estadisticas ni habilidades. Los 1275 recursos de Data/Item ya coinciden por hash con el cliente OpenGL.

### Login Paris Project y cuentas guardadas (2026-10-19)

El acceso conserva el escenario animado de selección de servidor. El emblema superior usa la Torre Eiffel de Paris Project y el formulario usa el marco negro y dorado con dragones del diseño aprobado. El formulario se centra en la zona inferior y distribuye las credenciales y las cuentas guardadas en dos columnas.

Hay cinco espacios locales. Seleccionar uno ocupado rellena los datos; seleccionar uno vacío permite guardarlos allí. GUARDAR CUENTA actualiza un nombre ya guardado, usa el espacio seleccionado o el primer espacio libre. Una lista llena requiere seleccionar explícitamente un espacio para reemplazarlo. La contraseña sólo se guarda si se activa Guardar contraseña y se acepta la confirmación existente. Las credenciales usan el cifrado del cliente; la lista no publica contraseñas. La X de cada fila elimina esa entrada local.

CANCELAR oculta el acceso, mantiene el mundo animado y vuelve a solicitar la lista de servidores; el selector reaparece al recibir una lista nueva. No cambia el mapa ni la cámara de selección.

Arte preparado con la herramienta integrada de imágenes a partir del diseño aprobado: extracción del emblema Eiffel PARIS PROJECT sobre transparencia; extracción del marco con dragones, conservando el título y retirando los controles dibujados para sustituirlos por controles reales. Los PNG fuente y las versiones TGA/OZT para el cliente están en Data/Interface/ParisLogin. El empaquetado conserva los píxeles y el canal alfa y marca los recursos como HD.

Ajuste visual posterior: el panel-v2 conserva los marcos de entrada, cinco filas, casillas y botones ornamentados como parte de la textura HD, con controles reales transparentes encima. Los cinco espacios tienen posiciones independientes para que no se superpongan en RmlUi. La presentación del acceso está en español y usa tipografía con remates; el ojo permite mostrar u ocultar la contraseña y vuelve a ocultarla al cerrar el formulario. La imagen del emblema superior no cambia. La vista previa se comprueba por separado del cliente, ya que los valores de display por defecto no son iguales entre HTML y RmlUi.

### Correcciones de acceso y guardado automático (2026-10-19)

Las etiquetas Cuenta y Contraseña quedan separadas de sus bordes y el contenido se centra en los campos. El ojo conserva el valor real, el límite de caracteres y la selección al alternar entre texto visible y oculto; al cerrar el formulario la contraseña vuelve a ocultarse. El aviso inferior lleva una superficie de cuero que evita que los adornos crucen el texto. Menú y Créditos usan el marco negro y dorado del formulario y se retira la decoración plateada anterior.

Al pulsar ENTRAR con Recordar cuenta activado se añade o actualiza automáticamente una entrada de Cuentas guardadas. Guardar contraseña añade la contraseña sólo con la autorización existente; si se deja desactivado se guarda únicamente el nombre. El acceso automático usa un espacio libre y no reemplaza una cuenta diferente por estar seleccionada. Si los cinco espacios están ocupados, GUARDAR CUENTA conserva la opción manual de reemplazar expresamente el seleccionado. Editar los datos elimina las credenciales obsoletas del último acceso, pero mantiene la elección de las casillas para el siguiente ENTRAR.

Verificación: dos pruebas específicas, 23 comprobaciones, cubren la visualización real/oculta de la contraseña, la escritura con el límite de caracteres tras usar el ojo y la elección de espacios sin sobrescribir otras cuentas. Las comprobaciones de sintaxis, contratos de temas y distribución también pasan.

Comprobación visual en el cliente a 1024×768: formulario con Cuenta y Contraseña separados del borde, contenido centrado, cinco filas visibles, mensaje inferior sin adorno cruzando el texto y Menú/Créditos con marco dorado. Jugar queda abierto en el formulario. El ojo y la selección del espacio se verificaron con datos ficticios en las pruebas, sin mostrar contraseñas reales ni enviar un acceso de prueba al servidor. La vista previa también respeta ahora el tamaño lógico de los recursos HD; pasan sus 21 pruebas.

### Estilo de selección, menú y desconexión; entrada de contraseña (2026-10-19)

La entrada de Cuenta y Contraseña empieza después del icono y conserva el centrado vertical. El ojo alterna entre dos controles enlazados al mismo valor: uno siempre oculto con asteriscos y otro siempre de texto normal. Así no se recrea el control al mostrar la contraseña y la escritura posterior conserva el valor y el límite; cerrar el acceso vuelve al modo oculto. La prueba del formulario simula clics en el ojo y escritura posterior; junto con los casos de espacios guardados pasan 31 comprobaciones.

El menú del acceso y de selección de personaje, los botones de grupo/servidor MuParis y los mensajes de aviso usan cuero negro, bordes dorados y tipografía Paris Project. Esto incluye el mensaje de desconexión del servidor. Se conservan sus botones, acciones, estados de disponibilidad y el escenario animado.

### Botones sutiles y aviso sin placa (2026-10-19)

MuParis, el acceso al servidor, Menú y Créditos usan el diseño compacto aprobado: cuero negro, borde dorado fino y grabados pequeños en las esquinas. Comparten la misma textura y conservan las acciones y los estados del selector. El aviso inferior del login queda únicamente como texto, sin fondo ni marco, colocado en el espacio libre bajo el panel para evitar líneas o adornos detrás de las letras.

El recurso de proyecto Data/Interface/ParisLogin/subtle-button.png se preparó con la herramienta integrada de imágenes a partir de la maqueta aprobada: extraer un botón pequeño vacío, cuero negro, doble borde dorado discreto, esquinas cortadas con filigrana mínima, sin texto ni iconos y con exterior transparente. Las versiones TGA/OZT conservan los píxeles y el alfa; el OZT está marcado 4x y las coordenadas del recorte se expresan en el tamaño lógico del cliente. La textura permanece en el proyecto, sin depender del directorio de imágenes generadas.

### Compact Paris dialogs (2026-10-19)

System menu and generic notices now use the approved fine gold frame with small matching buttons, without the large logo. All three themes retain the menu callbacks and all five message modes, including resident-password input. Spanish menu and disconnect captions match the approved mockup.

The shared skin is `src/bin/Data/Interface/ParisLogin/dialog-frame.png` (TGA/OZT MUX2 runtime encoding). Built-in image generation prompt: “Blank compact game dialog frame, black fine leather, very thin gold double outline, clipped corners with tiny gold engraving, no text, no logos, transparent surroundings.” The spritesheet resolution keeps the corner details small while the center stretches.

Active Invasion now hides with the game interface when returning to character or server selection, including its active-event button and expanded details.

Paris Dorado character selection uses compact gold/black CREAR and MENÚ buttons at the lower left, and ENTRAR and ELIMINAR at the lower right. The silver strip and flourish are removed; character availability and confirmation actions remain unchanged.

The unified Windows installation and Paris Project launcher are documented in [Paris Launcher](paris-launcher.md), including R2 updates, the desktop shortcut and how to publish releases.

Paris Dorado gives the in-game menu and Options a compact black and gold frame without a logo. Legacy and Legacy inheritance test retain their original frames. Clima lists the six timed weather settings vertically, with the duration and audio explanation at the end; only the Clásico/Moderno/Ultra HD preset selector is removed from Clima. Its natural colors, ambient light, stars, distance fog, sharpness, fog density, bloom and sky/atmosphere controls remain below the six timed settings. Paris Options uses the same gold scrollbar as Info.

Hovering a monster row in Active Invasion displays its model and remaining/total count. The event footer shows the combined remaining/total count.

Modern item tooltips read the same Paris rarity configuration as OpenGL, including server-provided text colors, rarity labels and frames, lore, and category bonuses. Locked bonuses show their required item level. The configuration changes presentation only; the server still determines the actual bonuses.

The modern window title is MU PARIS CLIENTE VULKAN. Paris HUD potion counts and orb values use smaller text. HP and mana use the server's full 32-bit character values; SD and AG retain actual zero values and use the available server maximum, so their fill follows current / maximum without inventing a full bar. Hovering each resource shows current and maximum values. The two slim performance indicators update every second: FPS and TCP round-trip latency, with their numerical values on hover. Gray ping means no measurement is available.

Windowed mode supports resizing by dragging its borders and maximizing, with a minimum of 640 × 480. The camera, HUD and mouse coordinates follow the actual client area. In a game session, the window title adds character name, level, Master level, resets and the local clock; selecting a server returns to the base title. Paris HUD centers potion quantities below each socket, uses a square mana fill to preserve its round outline, hides the SD bar in this theme, moves AG down and uses the two adjacent narrow wells for FPS and ping. Monster previews frame their sampled idle-animation bounds with a margin, including large models and wings.

The AG readout is centered below its full-height fill. FPS and ping each occupy their own narrow frame with an inset, without using or covering the former SD fill.

Monster previews use visible posed mesh vertices rather than loose collision bounds, occupy 90% of the available image limit and turn around the vertical axis once every 12 seconds. A six-pixel grip inside each window edge and corner supports mouse resizing from all sides; fullscreen and maximized windows do not capture these grips.


### Server Custom Notice parity

Custom Notice announcements now use the same server packet metadata as the classic client: text RGBA, RGB565 background, background opacity and blink flag for the five notice categories. Guild notices retain their guild-history entry. Wrapped lines keep their own received style. Standard announcements and slide notices keep their existing behavior. This works in Legacy and Modern themes.

Paris chat recognizes [POST] messages and uses the same cyan background / black text as OpenGL. Ordinary GM messages retain their GM style. Server-configured Custom Notice colors apply to notices, not ordinary chat packets, which do not contain a color field.
