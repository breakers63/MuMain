# Paris Project para Windows



## Launcher 2.0: cuenta, bienvenida y rendimiento



La versión 2.0 añade acceso con la misma cuenta de MU Paris y registro desde el

launcher. El registro muestra una bienvenida de tres pasos que explica OpenGL,

Vulkan y DirectX 12. También se puede volver a abrir desde Opciones. Las ventanas

se muestran a tamaño grande y se ajustan al área disponible de la pantalla.



La contraseña solo se envía por HTTPS para validar el acceso; no se guarda en el

launcher. **Recordar sesión** guarda un token protegido por Windows para el usuario

actual. **Cerrar sesión** revoca ese token en el servidor. Este acceso identifica la

cuenta del launcher: no introduce ni guarda automáticamente la contraseña en el

cliente del juego.



En **Rendimiento**, elegir primero el cliente en Biblioteca. Los perfiles Calidad,

Equilibrado y Rendimiento se guardan con el juego cerrado. OpenGL admite el clima,

el detalle de multitudes y las prioridades adaptativas ya soportadas por el cliente;

el moderno admite también sombras, partículas, antialiasing y límite de FPS.

**Restaurar configuración anterior** recupera las opciones modificadas por el

launcher sin sustituir los demás ajustes ni los datos de acceso.



El plan de energía de Windows se cambia solo al pulsar **Activar alto rendimiento**.

Puede aumentar el consumo y la temperatura; **Restaurar plan anterior** recupera

el plan guardado. No se deshabilitan antivirus, servicios, núcleos ni seguridad de

Windows para aplicar estos perfiles.



Conexión muestra mediciones TCP, una gráfica de las últimas mediciones y un

diagnóstico de adaptadores y DNS. Los intentos TCP sin respuesta no son una prueba

de pérdida de paquetes UDP. La ruta es directa: túneles, Multi Internet y un

limitador de tráfico no están implementados por estas herramientas. Requieren un

motor de red y nodos adicionales antes de poder ofrecer esas funciones.



Al jugar, el launcher se minimiza para mantener sus mediciones disponibles.



### Servicio de cuentas



`AccountsApiUrl` en `launcher.json` apunta al servicio público HTTPS de cuentas.

La base de datos se configura únicamente en el servidor mediante

`ConnectionStrings__Accounts`; esa cadena nunca se distribuye con el cliente.

El servicio usa una cuenta de PostgreSQL restringida a autenticación, creación de

cuentas y sesiones. No permite modificar personajes, monedas ni permisos.



El servicio limita los intentos de acceso y registro. La pasarela sobrescribe

`X-Paris-Client-IP` con la dirección verificada por Cloudflare; el contenedor de la

API no expone un puerto público. Las sesiones caducan a las 12 horas, o a los siete

días si se marca Recordar sesión, y los tokens se guardan hasheados en el servidor.

Una cuenta bloqueada no puede validar una sesión. El registro no verifica la

propiedad del correo; no debe anunciarse como verificación por e-mail.



Esta versión se entrega primero como compilación local de revisión. El enlace

público anterior no cambia hasta publicar expresamente el nuevo ejecutable.



`Launcher.exe` es el instalador, actualizador y selector de entrada de Paris Project.

La instalación incluye Clásico (OpenGL de 32 bits) y Moderno (Vulkan o DirectX 12 de

64 bits). El jugador elige cómo entrar, sin mantener dos instalaciones completas.



## Instalar y jugar



1. Descargar `https://patch.hispanogaming.com/windows/Launcher.exe?v=1.0.4` y abrirlo.

2. Una descarga nueva prepara `C:\PARIS PROJECT`. Para elegir otra

   ubicación, usar **Opciones del launcher → Carpeta de instalación**.

   El instalador ofrece una exclusión opcional de Microsoft Defender, desactivada

   por defecto. Aceptarla requiere permisos de administrador y deja toda esa carpeta

   sin análisis de Defender; rechazarla permite continuar con la protección habitual.

   Se puede retirar después desde Seguridad de Windows → Exclusiones. No modifica

   otros antivirus ni desactiva la protección general.

3. Esperar a que termine la barra. La instalación inicial incluye ambos modos.

4. Usar el acceso directo **Paris Project** del escritorio, elegir el modo y pulsar

   **JUGAR**. El acceso directo usa el icono dorado de Paris Project y abre el launcher.

5. **Recordar mi elección** entra automáticamente después de verificar las

   actualizaciones. Mantener Shift al abrir permite volver a elegir.



El launcher detecta las tarjetas físicas y muestra su memoria; no fuerza cambios

en el controlador de vídeo. La calidad visual y los climas siguen en las opciones

del juego. Vulkan y DirectX usan el mismo ejecutable Moderno. OpenGL usa el

ejecutable clásico dentro de `Classic`, con sus propias DLL de 32 bits.



La primera instalación necesita aproximadamente 2,3 GB de descargas sin comprimir

(más el launcher) y espacio temporal para las actualizaciones. Los archivos

idénticos usan enlaces duros NTFS: ambas rutas acceden a los mismos datos. En una

unidad que no permita estos enlaces se hacen copias. Los archivos diferentes se

mantienen separados. No se borran ni modifican las instalaciones originales.



La configuración inicial se crea desde plantillas sin cuentas ni contraseñas.

Las actualizaciones no reemplazan `config.ini`, las preferencias del launcher ni

sus credenciales. Las dependencias CRT de 32 y 64 bits se incluyen por cliente,

sin instalar componentes globales ni pedir elevación.



## Actualizaciones R2



Bucket: `parisproject`; prefijo: `windows/`; dominio público:

`https://patch.hispanogaming.com/windows/`.



La correspondencia es directa: `windows/Data/archivo` se instala como `Data/archivo`

en la raíz del cliente. Los archivos exclusivos del clásico están bajo `Classic/`.

Los recursos idénticos se descargan una vez y el manifiesto declara su segunda ruta.



El launcher lee `windows/manifest.json`. Cada entrada contiene `Path`, `Size`,

`Sha256`, `Client` (`common`, `modern`, `classic`) y opcionalmente `Url` y

`ClassicAliases`. Solo se descargan archivos faltantes o diferentes. Las descargas

usan HTTPS en el mismo dominio y se verifican antes de instalar. Si falla la

instalación, se recuperan los archivos anteriores; un registro permite recuperar

una interrupción al abrir de nuevo. El cliente debe estar cerrado para actualizar.

Las consultas del manifiesto llevan una versión de comprobación y los archivos

usan su SHA-256 como versión en la URL para evitar copias antiguas en la caché CDN.



`launcher.json`, junto al ejecutable, permite cambiar las rutas de los clientes y

la dirección del manifiesto. No contiene claves de R2 ni credenciales del VPS.

El launcher no se reemplaza a sí mismo durante una actualización: nuevas versiones

de `Launcher.exe` se publican por separado en esa misma dirección de descarga.



### Preparar una versión



Compilar con .NET 10 SDK:



```powershell

dotnet publish tools/ParisLauncher/ParisLauncher.csproj -c Release -r win-x64 --self-contained true -o out/launcher-final

dotnet run --project tests/launcher/LauncherChecks.csproj -c Release

```



`tools/ParisLauncher/prepare_release.py --help` describe cómo combinar las dos

carpetas en un destino nuevo. Compara contenido, incluye las DLL apropiadas y

genera las plantillas sin cuentas guardadas. No sobrescribe una versión existente.

Verificar el paquete completo con:



```powershell

dotnet run --project tests/launcher/LauncherChecks.csproj -c Release -- --verify-release out/ParisProject-20261019

```



Subir los archivos y el launcher antes de publicar el manifiesto. No usar `sync`

para borrar archivos del bucket. Publicar `manifest.json` al final, con

`Cache-Control: no-cache`, para no anunciar archivos que aún no están disponibles.

No publicar las carpetas `Screens`, `CrashDumps`, registros, configuraciones del

jugador, claves SSH ni credenciales R2. Conservar copias de las versiones anteriores.



Para R2 en producción se usa el dominio personalizado en lugar de `r2.dev`:

[documentación de Cloudflare](https://developers.cloudflare.com/r2/buckets/public-buckets/).



## Diseño



El arte y el icono se generaron con la herramienta integrada de imágenes. Archivos:

`tools/ParisLauncher/Assets/launcher-art.png`, `launcher-icon.png` y su codificación

Windows `launcher-icon.ico`. El arte conserva la primera maqueta aprobada; los

controles reales no prometen HDR ni otras funciones sin validar.



Prompt del arte: “Paris Project HD launcher, black fine leather and restrained

antique gold, Eiffel logo, dark fantasy Paris castle, classic/modern selector.”

Prompt del icono: “Square Windows desktop icon, gilded Eiffel Tower integrated

with capital P, tiny fleur-de-lis, transparent background, readable at small size.”



La detección de GPU utiliza DXGI, incluyendo memoria dedicada y compartida:

[DXGI_ADAPTER_DESC1](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ns-dxgi-dxgi_adapter_desc1).



## Validación de esta entrega



La versión `2026.10.19.1` contiene 19.368 entradas, incluyendo 17.270 recursos

compartidos mediante enlaces, y 2.275.506.809 bytes de contenido sin duplicar

las segundas rutas. La carpeta local preparada es `out/ParisProject-20261019`.

Pasaron 31 comprobaciones de rutas, grupos de instalación, enlaces NTFS,

reintentos de red, corrupción con tamaño correcto, cancelación y recuperación.

Se verificaron todos los archivos del paquete local, además de ocho descargas

reales desde el dominio R2, incluidos recursos comunes, CRT x86/x64, RML y

shaders DXIL/SPIR-V. Se comprobó la descarga completa y SHA-256 del launcher

público. No se ha completado una instalación limpia de los 2,3 GB desde Internet

ni una sesión de juego de ambos modos mediante el nuevo launcher.



Para publicar futuras versiones desde el VPS: ejecutar

`python3 /root/paris-windows-20261019-publish.py CARPETA_DE_LA_VERSION`.

La copia mantenida del publicador es `tools/ParisLauncher/publish_r2.py`;

verifica los tamaños y hashes y publica el manifiesto al final.



### Corrección 1.0.1



Se corrigió la validación de la carpeta cuando su ruta termina en una barra, como

la ruta proporcionada por el propio ejecutable. Esta condición impedía iniciar

una instalación nueva. Pasan 35 comprobaciones, incluidas las rutas terminadas

en barra o barra doble y ocho descargas reales desde R2 con esa misma condición.

Cerrar el launcher anterior y descargar la versión nueva para aplicar la corrección.



## Instalación interrumpida en 95%



La versión 1.0.3 muestra el número de archivos instalados durante la fase final y guarda un único plan de recuperación completo. Si una instalación anterior quedó incompleta, cerrar el juego y volver a abrir el launcher actualizado en la misma carpeta. El launcher recupera el estado anterior y verifica los archivos antes de habilitar JUGAR. No borrar manualmente `.launcher-update` mientras contiene una recuperación pendiente.



## Equipos con pocos recursos



La versión 1.0.4 ejecuta la actualización fuera del hilo de la ventana y conserva solo el último aviso de progreso, mostrado cada 150 ms. Usa dos descargas simultáneas en equipos de hasta cuatro procesadores lógicos y cuatro en los demás. Reutiliza los búferes de descarga y calcula SHA-256 mientras recibe los datos para evitar una segunda lectura del disco. No elimina la comprobación de archivos ni la recuperación.



El ejecutable distribuido sigue incluyendo .NET para funcionar sin instalar ese componente por separado. Su tamaño de descarga no equivale a su consumo de memoria durante la actualización.





### PC Boost and account header (local preview, 2026-11-11)

The header reads `GET /profile` using the existing bearer session. The server reads VIP tier/expiry from Paris account state and applies the same legacy VIP fallback as ParisVipService. Expired VIP is reported as normal. A failed request is reported as unavailable, never as normal. Read access uses a restricted profile view; account state and membership cannot be changed through this endpoint.

Network charts retain samples separately for each destination for the launcher session. Region flags are embedded PNG assets from FlagCDN. AWS targets are public regional EC2 endpoints, not routing nodes. ICMP and TCP failures are labeled separately. MTR is a short 3-probe TTL trace and does not claim intermediate ICMP filtering is end-to-end packet loss.

DNS comparison starts when the DNS view is opened and caches the last result for five minutes. Changing DNS remains an explicit action with backup. Public resolvers include Cloudflare, Google, Quad9, OpenDNS, Control D, AdGuard and Comodo; filtering is identified in the provider name. Help includes local FAQ, official website, https://muparis.com/Soporte/ and https://discord.gg/m6DGRkRNgr. Accounts confirmed without VIP see a red subscription button linking to https://muparis.com/usercp/pariscoins. The notification inbox currently has no server announcement feed and does not fabricate messages.

References: https://docs.aws.amazon.com/general/latest/gr/ec2-service.html ; https://docs.controld.com/docs/free-dns ; https://adguard-dns.io/en/blog/adguard-dns-new-addresses.html ; https://www.comodo.com/secure-dns/ ; https://github.com/cloudflare/speedtest





### General settings and upload limiter (2.1.0 local preview)

General settings persist in LocalAppData, separately from game/client paths. Optional startup uses the current-user Run key; it is changed only on the user toggle. Close-to-tray uses a Windows notification icon; explicit Exit still quits. Server notifications report TCP monitor failures, not a definitive server outage. Hardware monitoring currently displays live RAM usage. Only Spanish is available. Routing and dual-route controls remain disabled without tunnel infrastructure.

The traffic limiter performs a bounded 10 MB download / 2 MB upload test when requested. It uses application-specific Windows NetQoS outbound throttling in ActiveStore, asks for elevation on Apply, and leaves MU, the launcher and Windows processes excluded. It is not a whole-network download limiter. Only Paris-prefixed policies from the local journal are removed; ActiveStore policies expire at Windows restart. Windows edition/policy restrictions can prevent use. Commands and rollback were syntax-tested without applying limits to the user's machine; real enforcement still needs an elevated user-triggered test.

Reference: https://learn.microsoft.com/en-us/powershell/module/netqos/new-netqospolicy


## Elegir cliente y VPN Paris Project

En Inicio, **Elegir cliente** abre Biblioteca. El botón inferior ofrece la misma
acción hasta que elijas un cliente. Después, **Jugar** abre el cliente elegido.
Marca **Recordar mi elección** si quieres reutilizarlo en el siguiente arranque.

**VPN PARIS PROJECT**, debajo de **Analizador de red**, muestra el estado VIP
de tu cuenta y permite elegir Argentina, Brasil, Canadá o Chile. La conexión
queda deshabilitada mientras los nodos están pendientes de configurar.


Si todavía no tienes el juego, **Descargar** en Inicio abre Biblioteca. Selecciona
OpenGL o el cliente moderno y pulsa **Descargar** para instalarlo en
`C:\PARIS PROJECT`. Cuando vuelvas a abrir el launcher, este comprobará las
actualizaciones de los clientes instalados.

