# Respaldo de fuentes del 19 de noviembre de 2026

Estado local de vulkan guardado en [backup/cliente-sdl3-local-20261119](https://github.com/breakers63/MuMain/tree/backup/cliente-sdl3-local-20261119), commit `0c2d6bd065e1932fbf0ada2a35e8604c75ddb669`.

El respaldo conserva 539 archivos distintos respecto de la base, junto con las dependencias modificadas y los archivos en su codificación original. La restauración local verificó todos sus hashes.

Para recuperar: clonar la rama de respaldo con `--recurse-submodules`, seguir `source-backup/README.md` para aplicar los parches de dependencias y ejecutar `python tools/restore_source_backup.py` antes de compilar. El inventario está en `source-backup/manifest.json`.

La rama de trabajo conserva sus cambios más recientes de GitHub; esta instantánea adicional evita reemplazarlos por versiones locales anteriores. Los recursos del juego siguen disponibles mediante los instaladores y paquetes publicados en R2. Este respaldo de código no incluye bases de datos de producción, cuentas guardadas, credenciales ni certificados de firma.
