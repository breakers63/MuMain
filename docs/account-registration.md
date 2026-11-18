# Registro y confirmacion de venta MU Paris

Crear cuenta usa el servicio HTTPS compartido `https://ws.hispanogaming.com/launcher-api/register`. El servidor valida y guarda la cuenta; el cliente nunca accede directamente a PostgreSQL. Usuario de 4 a 10 letras o numeros, clave de 4 a 20 caracteres ASCII, repeticion y correo. Una solicitud por formulario a la vez.

CONTINUAR despues de un alta 201 guarda la cuenta con el sistema existente protegido por Windows y conecta al juego sin volver a escribirla. CANCELAR o Escape limpia la clave temporal y restaura el login si el socket del juego sigue listo; en caso contrario conecta al ConnectServer y vuelve al selector. Salir de la escena no restaura ventanas. La ruta de CONTINUAR no activa la restauracion de CANCELAR.

La confirmacion de venta conserva los callbacks originales y la validacion de slot. Solo cambia la presentacion de la confirmacion ya existente para articulos valiosos vendidos a NPC. Presenta el item real, marco negro y dorado, VENDER y CANCELAR. Cancelar conserva la devolucion del item al inventario. No incluye ventas de prueba automaticas.

Los textos de inicio y registro son ASCII. OpenGL conserva controles nativos y Vulkan usa RmlUi con geometria explicita para impedir campos comprimidos. No modifica tiers, Item Trace, RGB ni estadisticas.

## Recursos y verificacion

Los recursos del cliente se distribuyen en R2 como el resto de Data. La version `2026.11.17.3` contiene los nuevos recursos de ParisLogin y los documentos RmlUi. Ver `docs/registration-assets.json` y ejecutar `python tools/install_registration_assets.py` para obtener los recursos nuevos con SHA256 verificado en una copia de las fuentes que ya tenga el Data base.

Ambos clientes compilaron en Windows y el usuario confirmo el alta, CONTINUAR, cierre del formulario y la nueva presentacion. Las comprobaciones RML de sintaxis, geometria y contratos de temas pasaron. R2 se publica solo despues de verificar los ZIP y el launcher.
