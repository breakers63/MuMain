# Estelas de sets completos

Los sets completos +13, +14 y +15 muestran dos estelas en sentidos opuestos con el blanco original de OpenGL. No se tiñen con las botas. La frecuencia y velocidad se normalizan por el tiempo de animación.

Data/Custom/ParisSetAppearance.json permite ajustar minimumLevel (13), trailPairsPerSecond (1.25), trailWidth (23) y trailSpeed (1.35). Se carga al iniciar el cliente. Las opciones de calidad y ocultación de efectos se respetan.

## Ajustes de mascotas de comparación

Rudolf rodea al NPC y orienta su cuerpo hacia el recorrido. La dispersión inicial de sus estrellas conserva la misma distancia a cualquier FPS. El ángel de SUMN emite plumas pequeñas descendentes, con emisión limitada por tiempo de animación para evitar acumulación a FPS altos.

La fila de Lorencia comienza con Elfa en 162,130 y continúa cada dos casillas hasta Rage en 172,130. Shadow Phantom Soldier queda en 174,130.

## Reflejo claro de las piezas

Las piezas de armadura +13 a +15 añaden un reflejo blanco suave que conserva su color base. `neutralHighlightStrength` permite reducirlo o desactivarlo (0), con un máximo de 0,3. Respeta la calidad máxima de efectos y no se añade sobre piezas teñidas. Funciona en el mundo y en el inventario. La luz de Lorencia usa la textura del cliente MSPro de referencia; el ciclo de clima sigue siendo independiente.
