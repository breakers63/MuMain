# NPC Dressed, joyería, BotBuffer e intercambio Crest para Paris

Implementado en las fuentes locales de OpenMU Paris y ambos clientes MuMain (Vulkan y OpenGL). Los ejecutables del paquete fueron compilados localmente. No se ha instalado ni reiniciado el servidor en el VPS y todavía no se ha validado la apariencia dentro del juego.

## NPC configurados

| Servicio | NPC | Mapa | X | Y | Costo |
|---|---:|---:|---:|---:|---|
| Anillos SM | 638 | 8 | 195 | 73 | 50.000.000 Zen y 15 de cada joya |
| Anillos BK | 639 | 8 | 197 | 73 | Igual |
| Anillos ELF | 640 | 8 | 199 | 73 | Igual |
| Anillos MG | 641 | 8 | 201 | 73 | Igual |
| Anillos DL | 642 | 8 | 203 | 73 | Igual |
| EXCHANGE CREST | 804 | 2 | 213 | 60 | Tres ítems 13/14 +1 |
| BufferElfa | 643 | 6 | 61 | 122 | 250.000 Zen |
| BufferSoul | 644 | 6 | 61 | 124 | 250.000 Zen |
| BufferBlad | 645 | 6 | 61 | 126 | 250.000 Zen |
| BufferLord | 646 | 6 | 61 | 128 | 250.000 Zen |

Las cuatro joyas de cada intercambio de joyería son Chaos (12/15), Bless (14/13), Soul (14/14) y Creation (14/22). Los requisitos originales aceptan cualquier nivel/opción; se conservan esos comodines. Solo se consumen objetos del inventario, fuera del equipo y de la tienda personal. Cada NPC conserva la restricción de familia de clase y nivel base 0–400, y se puede repetir sin límite.

Premios, todos de grupo 13, nivel 0 y sin opciones añadidas:

| Clase | Números de los tres premios |
|---|---|
| SM | 8, 9, 12 |
| BK | 8, 23, 26 |
| ELF | 8, 23, 26 |
| MG | 8, 12, 21 |
| DL | 8, 23, 13 |

Se siguieron los IDs de los archivos; varios comentarios originales tienen nombres incorrectos. Durabilidad de premio 0 o `*` se interpreta como durabilidad máxima nativa, para entregar objetos utilizables.

Crest entrega un ítem 13/14 +0 a cambio de tres 13/14 +1, sin skill, luck, opción, excellent ni ancient. Conserva el modelo 804 ya existente en `Data/Custom/Monsters.txt`.

## Uso

Pulsa el NPC para abrir la ventana dorada con requisitos, costo y premios. Confirma con Intercambiar o cancela con Cerrar/Esc. La confirmación dura cinco minutos y queda vinculada al NPC abierto. Los datos se envían desde el catálogo del servidor usando avisos delimitados que ambos clientes consumen antes de mostrarlos en el chat; no se usan valores de premios escritos en el cliente. Se comprueban distancia, clase, nivel, objetos, dinero y espacio. Si los premios no caben, se restauran los objetos a sus posiciones originales y no se cobra. El pago y los premios se guardan en la misma transacción del sistema Paris. Hay una espera de dos segundos entre intercambios.

## Buffs y diferencias de adaptación

Los cuatro bots estaban comentados en el respaldo. Se activaron por solicitud del usuario y se asignaron nuevos IDs 643–646 para representarlos como Dressed.

| Bot | Skill OpenMU | Duración | Potencia |
|---|---:|---:|---:|
| Elfa: Greater Damage | 28 | 10 minutos | 80 % |
| Soul: Expansion of Wizardry | 233 | 10 minutos | 80 % |
| Blad: Swell Life | 48 | 10 minutos | 80 % |
| Lord: Critical Damage Increase | 64 | 10 minutos | 80 % |

El skill 82 «Magic Circle» del respaldo se adaptó al equivalente 233 de OpenMU. La potencia es 80 % del resultado de la fórmula nativa calculada con los atributos del receptor al comprar; no se puede asegurar que coincida numéricamente con la fórmula cerrada del antiguo MSPro. Se conserva el ID de efecto nativo para que aparezcan sus iconos y se reemplace el mismo buff en vez de acumularlo. La duración queda en diez minutos y el comportamiento ante muerte/reemplazo sigue el sistema nativo.

La línea de alas del bot Lord decía grupo 14, número 40. Se corrigió al grupo 12, número 40, correspondiente a sus alas de tercera evolución en el cliente. Las opciones de vestimenta de los bots conservan el valor 15. `Dressed.txt` admite opcionalmente una columna final de máscara excellent para esas opciones.

## Configuración e instalación

Servidor: `Configuration/ParisNpcs.json`, junto a los archivos del servidor. Alternativamente define `PARIS_NPCS_FILE` con la ruta absoluta. Si no existe archivo, se usan los mismos valores incorporados en el ensamblado. Se requiere reiniciar tras editarlo.

Cliente: `Data/Custom/Dressed.txt`. Conserva las dos secciones del formato anterior: definición y vestimenta. Los IDs de misión y bots deben coincidir con el servidor. Cambiar la ropa requiere reiniciar el cliente. Las armas y partes usan los IDs del catálogo de Paris; los modelos de mascotas/alas deben existir en sus recursos.

Las fuentes ya están modificadas en:

- Servidor: `C:/Users/WOoS/MU-Paris/server-android-custom`.
- Cliente Vulkan: `C:/Users/WOoS/MuMain-Paris-RmlUi`.
- Cliente OpenGL: `C:/Users/WOoS/MU-Paris/client-publish`.

Compila y despliega el servidor completo con su procedimiento habitual. En una base existente aplica la actualización opcional «Servicios NPC configurables de Paris» en las actualizaciones de configuración del panel. Su identificador es `83C053C8-5A4B-42E6-A98D-C45315A4C7B8`. En una base nueva el plugin se registra activo automáticamente. No es necesario borrar ni reinicializar la base.

Los ejecutables de `Clientes` corresponden a las instalaciones locales actuales y necesitan sus recursos y dependencias habituales. Respaldar el Main.exe de destino antes de reemplazarlo. Copia además `Dressed.txt` a `Data/Custom/` de cada instalación del cliente.

## Alcance de NPCManager

Se portó el intercambio activo del archivo 017 (Crest). Los archivos 003–006 tienen el NPC y las posiciones comentados, por lo que no se activaron intercambios nuevos a partir de esos ejemplos. Sus reglas originales y las de los otros NPC se conservan en `source-audit.json`; teleport, GuildBattle, SkyEvent, monedas y permisos especiales no forman parte de este nuevo servicio de objetos.

## Verificación

- GameLogic y servidor de inicialización compilan.
- OpenGL y Vulkan compilan; los controles de recursos RmlUi de Vulkan pasan.
- 16 pruebas: niveles exactos de Crest, exclusión de equipo, selección sin reutilizar objetos, rechazo de costo negativo, familias de clase y evoluciones, y restauración de pago si el inventario no admite todos los premios.
- Catálogo Season Six: existen todos los objetos y skills; el plugin se registra al inicializar y su actualización repara una base existente sin duplicarse al aplicarse dos veces.
- Pendiente de verificación en juego: apariencia de cada Dressed y mascota, posiciones libres en los mapas reales del VPS, iconos/duración/potencia de buffs, y un intercambio real con reconexión para confirmar los datos persistidos.


Actualización instalada: los cinco NPC de joyería miran en dirección 0 (giro de 180 grados respecto a la configuración inicial). La ventana usa el mismo marco de Golden Archer/PvP en Vulkan y el marco Golden Archer de OpenGL. El componente instalado se integra sobre el ensamblado original del VPS, conservando Party Finder y las demás funciones de esa versión.


La ventana de intercambio muestra casillas con modelos 3D para requisitos y premios. Las joyas requeridas indican la cantidad disponible y la necesaria; el tooltip muestra nombre, nivel y cantidad. Vulkan usa los atlas de estado de Exodus en `Data/Custom/Buffs`, y los pergaminos muestran descripción y duración restante mediante el tooltip compartido. Los NPC de joyería usan Direction.South (3) para mirar al frente.
