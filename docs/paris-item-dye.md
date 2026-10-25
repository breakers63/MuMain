# Color de equipo (RGB) en Vulkan

Abre Custom > RGB. El inventario permanece disponible a la derecha.

- Clic derecho sobre un arma, escudo o pieza de armadura desequipada para seleccionarla.
- Arrastra una pieza al recuadro de su categoria si prefieres ese gesto. La pieza se conserva en el inventario: el panel solo selecciona su identidad.
- Una pieza aplica a un objeto. Varias piezas permite casco, pechera, pantalon, guantes y botas, con un objeto por categoria.
- Las piezas permanecen arriba; Previo y Siguiente recorren su vista previa en orden de categoria.
- Elige el color con el cuadro, la barra de tono o la paleta. Original muestra la apariencia sin tinte, Nuevo muestra la propuesta.
- Aplicar color solicita confirmacion y guarda cada pieza en el servidor. Si una falla, la secuencia se detiene y muestra el error; no se presenta como una transaccion conjunta del set.
- Restaurar original elimina el tinte de las piezas seleccionadas sin consumir materiales.

Custom > RGB del panel administrativo configura activacion, requisito de pociones y cantidad por color/pieza. El cliente usa esa politica del servidor; el modo de pruebas permite aplicar sin pociones. No cambia atributos ni opciones.

El tinte multiplica la iluminacion de las superficies existentes. No sustituye las texturas ni todos los efectos especiales; la cobertura visual completa del set queda pendiente de ajuste.

Validado: compilacion Release Vulkan, controles de arquitectura RmlUi y seis pruebas de estilos (660 comprobaciones). Comprobacion final con objetos y cuenta del jugador pendiente.

Corregido el desbordamiento del selector HSV: las clases de geometria fijan su contenedor, el degradado se recorta dentro de el y los titulos declaran su disposicion de bloque. Prueba real RmlUi: 681 comprobaciones correctas.

Las casillas vacias muestran las siluetas nativas de equipo. La paleta usa nueve filas de catorce colores con ancho flexible por celda, sin saltos de fila por redondeo. Los indicadores usan botellas nativas de curacion, antidoto y mana. Validacion de estilos: seis pruebas, 1920 comprobaciones.

Ajuste de interaccion: el destino del arrastre es la casilla padre, que sigue visible cuando su imagen 3D esta vacia. El clic derecho se comprueba tambien en la actualizacion del panel con las coordenadas de ventana del inventario. Original/Nuevo tienen marcos independientes de la imagen; Hex/R/G/B tienen columnas separadas. Validacion: 1965 comprobaciones de interfaz correctas. La seleccion final con cuenta del jugador sigue pendiente de comprobacion.

El tinte guardado usa una pasada de color propia para que los brillos de nivel/excellent no lo reemplacen. Al restaurar el original vuelven las pasadas de brillo nativas. La apariencia del jugador local se sincroniza tambien desde su inventario equipado. Los botones RGB/Asesinatos mantienen texto e icono dentro de su altura.

El selector de habilidades coloca los comandos de Dark Spirit al final de la lista general, sin fila independiente. En el tema Paris Dorado las filas se centran aunque haya pocas habilidades; las filas adicionales crecen hacia arriba.
