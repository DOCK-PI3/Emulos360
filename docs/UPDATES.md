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
