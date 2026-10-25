# Charcos de lluvia

Vulkan muestra manchas de agua de bordes irregulares en suelo exterior casi plano cuando se acumula humedad. Su posicion queda fija por mapa y celda; no siguen al personaje. Se atenúan al alejarse y desaparecen gradualmente al secarse.

La lluvia, sus gotas, salpicaduras y audio siguen usando el sistema existente. Los charcos no se dibujan en huecos del terreno ni en las zonas cubiertas que conoce el sistema de clima. Las laderas pronunciadas se omiten para evitar agua sobre paredes.

Esta primera version utiliza color de cielo y suavizado de bordes; no refleja personajes ni edificios como un espejo. La comprobacion visual final se hace durante lluvia dentro del juego.
