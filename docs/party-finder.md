# Party Finder y ajustes de Party

P conserva el ancho y alto de la ventana original y los marcos de inventario/estadísticas. Tiene las pestañas Miembros, Buscar y Ajustes. Buscar permite consultar las parties públicas aunque ya pertenezcas a una; para unirte a otra debes salir de la actual. Las privadas no aparecen en la búsqueda ni en la unión aleatoria. Las invitaciones normales siguen disponibles.

Cada integrante tiene una bandera. La del líder está activa; el líder puede pulsar la bandera apagada de otro integrante conectado para cederle el mando. Los demás no pueden transferirlo. El nuevo líder pasa a la primera posición de la lista. Una transferencia explícita conserva la privacidad y el permiso de seguimiento elegidos, y cambia el identificador público para invalidar solicitudes anteriores. Si el líder abandona la party, la publicación se revoca hasta que el siguiente líder decida activarla.

En Ajustes se muestran la visibilidad y el permiso de seguimiento. Solo el líder puede activar o desactivar la vista pública y permitir o deshabilitar el seguimiento. Deshabilitarlo impide nuevas solicitudes y detiene los seguimientos activos al recibirse el estado del servidor. Cambiar este permiso no convierte una party privada en pública.

Seguir aparece solo junto a las barras de party. Usa pathfinding y movimiento normales. Cuando el compañero cambia de mapa, se usa la misma acción del menú M, con sus requisitos y coste en Zen, y se continúa caminando desde la entrada hasta el compañero. Seguir no invoca al personaje junto a su objetivo. Ir al líder es una acción independiente de viaje directo validado.

El seguimiento se cancela al pulsar de nuevo su botón, caminar manualmente, pulsar botón derecho en el mundo o Esc, morir, salir de la party, perder al compañero, no encontrar ruta o recibir una denegación. Al llegar a dos casillas se detiene la marcha, manteniendo el seguimiento activo.

Los recursos visuales, textos y controles están separados. Los 40 textos de Party están traducidos a los 11 idiomas disponibles: alemán, inglés, español, indonesio, japonés, polaco, portugués, ruso, tagalo, ucraniano y chino tradicional. Vulkan usa RmlUi y OpenGL conserva su presentación clásica con el mismo protocolo y reglas del servidor.

## Validación

Se compilaron ambos clientes y el servidor. Las pruebas cubren privacidad, admisión concurrente, capacidad, liderazgo, ajustes de seguimiento, costes y entrada de Move, requisitos de viaje, cancelación de movimiento y paquetes inválidos. El layout de RmlUi conserva 190 × 429, tres pestañas, cuatro filas y controles separados en distintos temas y escalas. Las pruebas automáticas y las vistas previas no sustituyen la comprobación interactiva con dos personajes conectados.

Si el líder se desconecta, se elimina su plaza y se elige como líder a un integrante conectado, saltando las plazas reservadas de jugadores ausentes. Una party de dos conserva al integrante restante como líder. Si no queda nadie conectado, se cierra la party.

Auto Aceptar Party está desactivado inicialmente. Solo el líder puede activarlo para aceptar solicitudes directas de jugadores sin party en el mismo mapa; conserva los requisitos y cupos. No publica la party: si es privada seguirá oculta de la búsqueda y de la unión aleatoria. No afecta a las opciones existentes de MU Helper.

Buscar usa cuatro filas por página, con el arte rojo/dorado y adornos separados de los textos. Crear party pública está en Ajustes cuando no perteneces a ningún grupo. El botón Seguir de P se retiró: la flecha lateral del HUD conserva esa función.

En Ajustes, los integrantes ven las opciones de privacidad, seguimiento y Auto Aceptar Party deshabilitadas; solo el líder puede cambiarlas. Cada party tiene su propio marco dorado; Refrescar y la paginación usan adornos separados de sus textos y controles. Sin resultados se muestra el aviso sin un recuadro vacío.

Buscar conserva los marcos dorados sobre fondo negro, sin ilustraciones interiores. El aviso de salir de la party queda separado del adorno y de la lista.

Ajustes presenta cinco casillas verticales: party pública, permitir seguimiento, Auto Aceptar Party, mostrar buffs y bloquear invocación de Dark Lord. Las tres primeras las controla el líder. Buffs y bloqueo son personales para la sesión conectada; al reconectar vuelven a mostrar buffs y permitir invocación. El bloqueo se comprueba durante la cuenta atrás y antes del traslado. Miembros muestra nivel, resets, vida roja y maná azul con valores reales enviados solo a integrantes de la misma party.

Los buffs pequeños de cada integrante aparecen también encima de su barra del HUD. Mostrar buffs controla los iconos tanto en P como en el HUD. En Vulkan cada casilla usa un control estable de fila completa; los cambios rápidos esperan el intervalo del servidor sin perder el clic.
