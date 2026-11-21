# Paris Project para Windows

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
