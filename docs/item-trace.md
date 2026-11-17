# Item Trace

Al pasar el cursor sobre un ítem del inventario o equipo propios, aparece su
procedencia debajo de SPECIAL OPTIONS, si el servidor tiene Item Trace activado.
Se conserva la rareza, el tier y las estadísticas originales del tooltip.

El bloque muestra el monstruo o la caja, el mapa y las coordenadas de la caída,
el destinatario original y la fecha en UTC. Make muestra al administrador que
creó el ítem. Editor muestra al operador que lo guardó; no inventa una caída.

La procedencia se conserva al comerciar y mejorar el ítem. Los ítems sin registro
figuran con origen desconocido. El servidor decide qué campos mostrar y puede
desactivar el bloque desde Custom → Item Trace.

Esta integración consulta inventario y equipo propios. Las vistas de banco,
oferta de trade, Market y NPC todavía no muestran esta información. Cuando el
ítem entra al inventario del nuevo dueño, se puede consultar allí.


## Compras en NPC y Máquina Chaos

Las compras nuevas registran el nombre y las coordenadas del vendedor, el mapa, el comprador y la fecha UTC. Los objetos nuevos de una combinación exitosa registran Máquina Chaos (o el NPC de la combinación), el creador, la ubicación y la fecha UTC. Incluye las alas Evo 1, 2 y 3 y las mezclas custom que usan el sistema de combinaciones del servidor. Una mezcla fallida no crea historial. Mejorar el mismo objeto conserva su origen; las pilas conservan el historial de la pila de destino. Las compras y combinaciones antiguas sin registro siguen como desconocidas.

Los clientes Windows deben actualizarse para reconocer estos orígenes.
