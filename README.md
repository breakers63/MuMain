# Actualización incremental PARIS para MuMain SDL3

Este paquete reemplaza el menú plano por el marco plateado y las esquinas del panel de Stats del cliente MuMain. Conserva las doce opciones del menú y conecta Quest y Jewel Bank con los protocolos del servidor Paris existente.

## Qué incluye

- Menú de 2 columnas con estilo de los paneles clásicos de MuMain.
- Ventana Quest con catálogo, progreso, aceptar misión y reclamar recompensa usando el protocolo `0xED` versión 2.
- Ventana Jewel Bank con búsqueda, consulta de saldos, cantidades `x1`, `x10` y `ALL`, depósitos/retiros y lectura de mensajes `[#PJB1`, `[#PJB2`, `[#PJB3]`.
- Integración de recepción para ambos protocolos en `WSclient.cpp`.
- No cambia `vcpkg.json` ni requiere cambios en el servidor.

## Aplicar en Windows

Extrae este ZIP dentro del repositorio local. Abre Git Bash o PowerShell en:

`C:\\Users\\WOoS\\SrcLadoClienteMuMain-SDL3`

Ejecuta:

```powershell
powershell -ExecutionPolicy Bypass -File .\Paris-SDL3-Quest-JewelBank\install-paris-systems.ps1
```

El instalador exige que el parche anterior del menú ya esté aplicado. Guarda copias de los archivos que reemplaza en:

`out\\paris-sdl3-feature-backup`

## Compilar

En Visual Studio selecciona `x64 Release` y el destino `Main`, luego usa **Compilar > Compilar Main**. CMake tiene `CONFIGURE_DEPENDS` para los archivos nuevos y vuelve a configurar al compilar.

Si falla, comparte el primer error de C++ que aparezca en la ventana **Salida**; no el resumen final. Este paquete todavía requiere compilarse en tu checkout SDL3 porque ese repositorio no está montado en el entorno donde se preparó el paquete.
