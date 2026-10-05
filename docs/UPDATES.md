# Publicar una actualización

La primera versión con actualización desde la aplicación es **v0.3.0**. Las instalaciones v0.2.0 deben actualizarse manualmente a v0.3.0 una vez; a partir de ahí Emulos360 consulta la última release estable al abrirse. También se puede comprobar desde Ajustes → Emulos360 → Acerca de.

1. Compila Windows y Linux desde el mismo código y cambia `project(Emulos360 VERSION X.Y.Z ...)` en `CMakeLists.txt` antes de crear los archivos.
2. Crea una release pública con etiqueta `vX.Y.Z`. No la marques como *prerelease*.
3. Adjunta ambos paquetes con estos nombres exactos y sus archivos `.sha256`:

   - `Emulos360-vX.Y.Z-windows-x64.zip`
   - `Emulos360-vX.Y.Z-linux-x86_64.tar.gz`

4. Comprueba que la API de GitHub muestra el campo `digest` (`sha256:...`) del archivo de cada plataforma. El actualizador exige ese dato y verifica el paquete descargado antes de instalarlo.
5. Escribe las novedades en la descripción de la release. Emulos360 las mostrará después del reinicio.

El actualizador instala fuera del proceso principal, conserva `data` (perfil, partidas, carátulas y ajustes), restaura los archivos anteriores si la sustitución falla e intenta reiniciar la aplicación. La build Linux se prepara en Ubuntu 26.04; las dependencias del sistema siguen instalándose con los scripts incluidos en el paquete.

La release histórica **0.4.0** incorporaba Edge. La revisión del 5 de octubre
retira los proveedores externos y mantiene un único core. Los instaladores nuevos
aceptan paquetes sin Edge y conservan `data`. El helper de una instalación 0.4.0
anterior debe corregirse antes de usar una release sin Edge; véase
[la transición y conservación de datos](SINGLE_CORE.md). Para 0.5.0, se entrega
además un ZIP de reparación de ambos ayudantes con copia de seguridad; véanse
[las instrucciones de la release](RELEASE_0.5.0.md).

## Fallo de arranque en Linux

Si aparece «La nueva versión se cerró durante el arranque», consulta
[la recuperación del actualizador y el diagnóstico](LINUX_UPDATE_RECOVERY.md).
El instalador corregido conserva la salida de arranque y permite distinguir el
cierre de la aplicación del proceso de descarga e instalación.
