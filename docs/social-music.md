# Emojis e instrumentos

La tecla encima de Tab abre cuatro anillos: gestos, emojis e instrumentos.
Los siete emojis se envían por el chat y se dibujan sobre el personaje; web y
Windows reconocen los mismos tokens.

Elegir guitarra, flauta, ocarina, arpa o batería abre la ventana del instrumento.
Cargar MIDI permite seleccionar `.mid` o `.midi`, Tocar, Pausa, Detener y Repetir
controlan la canción. Guardar retira el instrumento. Moverse, morir o cambiar de
mapa detiene la reproducción. ESC cierra la ventana.

El audio es LOCAL y usa el sintetizador MIDI de Windows. Las bandas y la escucha
compartida con la web todavía no están conectadas. La interfaz lo indica.
Límites: 2 MB, 30 minutos, formato MIDI 0/1 con tiempo por pulsos.
Batería reproduce el canal 10; los demás instrumentos los canales melódicos.

Los modelos son recursos originales de la web convertidos al formato nativo.
No cambian el equipo ni las estadísticas del personaje.

Los instrumentos usan una postura estable y ocultan visualmente las armas mientras se toca, sin desequiparlas. Los emojis aparecen sobre el personaje a tamaño reducido.

El renderizado de instrumentos prepara una sola transformacion por pieza y fotograma. La reproduccion MIDI respeta la duracion final del archivo aunque ya no queden eventos.

En Vulkan, el nombre del personaje local permanece visible sin senalarlo. Se conserva la ocultacion de identidad de Chaos Castle y el titulo de la tienda personal.

Opciones: �Ocultar nombres� oculta en tu pantalla los nombres y clanes de todos los jugadores, incluido el propio. No oculta los mensajes, los NPC ni las barras de vida. La preferencia se guarda en esta instalacion.

Skill lists scan all 650 character slots across classes. Passive/internal skills remain excluded. Burning now probes both terrain layers and emits bounded embers along the front.
