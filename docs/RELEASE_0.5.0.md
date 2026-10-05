# Emulos360 0.5.0

Xenia Canary del fork Netplay vuelve a ser el único core. Se retiran Nexia360 y
Xenia Edge y su selector, conservando la GPU global y por juego. No se han
portado cambios de esos cores al motor principal ni se anuncian nuevos juegos
compatibles.

Ubuntu: el instalador incluye ahora libqt6concurrent6. Su ausencia impedía
arrancar la interfaz tras actualizar (código 127). Se mantienen los registros
de arranque y la restauración de la versión anterior cuando algo falla.
El propietario confirmó solucionada la incidencia en su Ubuntu 26.04.

Los paquetes conservan Netplay/MultiP y distribuyen el mismo core principal.
No incluyen perfiles ni partidas personales. Las actualizaciones conservan
la carpeta data y el progreso histórico de los cores retirados.

## Instalaciones anteriores a 0.5.0

El instalador publicado con 0.4.0 exigía archivos de Edge, incluso si no se usaba.
Antes de actualizar desde esa versión, cierra Emulos360 y el juego y extrae
Emulos360-v0.5.0-reparar-actualizador.zip. Desde la carpeta extraída ejecuta:

Windows (PowerShell):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\repair-windows-updater.ps1 -InstallDirectory "C:\ruta\a\Emulos360"
```

Ubuntu:

```sh
bash scripts/repair-linux-updater.sh "/ruta/a/Emulos360"
```

La reparación sustituye solo el ayudante de actualización y guarda una copia
del anterior; no toca perfiles ni partidas. Después abre Emulos360 y actualiza
normalmente. Es un paso único; 0.5.0 ya contiene el ayudante corregido.
También se puede actualizar manualmente conservando la carpeta data.

Linux requiere Ubuntu 26.04 x86-64. En una instalación nueva ejecuta
`bash install-dependencies.sh` dentro del paquete extraído antes de abrirla.
Para el fallo ya identificado en una instalación anterior:

```sh
sudo apt update
sudo apt install libqt6concurrent6
```

## Archivos para publicar

- Emulos360-v0.5.0-windows-x64.zip y su .sha256
- Emulos360-v0.5.0-linux-x86_64.tar.gz y su .sha256
- Emulos360-v0.5.0-reparar-actualizador.zip y su .sha256

Publicar la etiqueta v0.5.0 como release estable. La aplicación comprueba el
digest SHA256 de GitHub para los paquetes de cada plataforma.
La compilación y las pruebas locales no acreditan partidas de todos los juegos;
las pruebas de gameplay corresponden al propietario.

## Comprobaciones de la entrega

CTest 4/4 en Windows y 4/4 en Ubuntu; siete regresiones del actualizador Linux
y fixtures del empaquetador/actualizador Windows correctos. Reparación Windows
verificada con copia anterior e idempotencia. Arranque de ambos runtimes y
dependencias Linux comprobados. SHA256, ausencia de datos personales y permisos
de ejecución Linux verificados en los archivos finales; cores y fuentes nativas
conservados. [Evidencia](releases/0.5.0-validation.json).
