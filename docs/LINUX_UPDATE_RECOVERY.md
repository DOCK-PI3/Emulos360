# Recuperar y diagnosticar la actualización de Linux

## Diagnóstico confirmado — 5 de octubre de 2026

Los archivos `update-startup.log` y `update-rollback-startup.log` copiados del
Ubuntu 26.04 del propietario, junto con la captura `photo_2026-10-05_10-30-02.jpg`,
identifican la causa del cierre de la actualización a 0.4.0:

```text
error while loading shared libraries: libQt6Concurrent.so.6:
cannot open shared object file: No such file or directory
```

La captura confirma el código de salida 127. El cargador de Linux no puede
resolver Qt Concurrent; la nueva interfaz termina antes de ejecutar su código.
El core Xenia no llega a iniciarse. No se trata de una incompatibilidad del
juego, de Xbox Live, de la GPU ni de una diferencia de distribución: el equipo
usa Ubuntu 26.04.

`CMakeLists.txt` enlaza la interfaz con `Qt6::Concurrent`, pero
`scripts/install-ubuntu-deps.sh` omitía `libqt6concurrent6` de las dependencias
para ejecutar el programa. Su lista de compilación sí incluía `qt6-base-dev`,
que depende de esa biblioteca: la instalación de desarrollo bajo WSL ocultó
la omisión al comprobar el arranque. El proveedor se ha verificado con
`dpkg-query -S /usr/lib/x86_64-linux-gnu/libQt6Concurrent.so.6` y los metadatos
APT oficiales de Ubuntu 26.04: `libqt6concurrent6`, versión 6.10.2+dfsg-7.

El registro de restauración muestra que la versión anterior inicializó Qt
Multimedia y abrió la introducción. Los avisos de VDPAU y PipeWire aparecen en
ese proceso restaurado; no son la causa del fallo de carga de la versión nueva.
La captura muestra la interfaz anterior con el diálogo de error. Esto confirma
la restauración, pero no demuestra funcionamiento de juegos ni de audio/vídeo
sin incidencias.

## Recuperar esta instalación

Cierra Emulos360 y ejecuta en una terminal **del equipo Ubuntu**:

```sh
sudo apt update
sudo apt install libqt6concurrent6
```

Después abre Emulos360 con tu acceso habitual y repite la actualización.
El paquete oficial satisface la biblioteca que falta; no hay que copiar `.so`
de terceros ni instalar todo el entorno de desarrollo. Si aparece otro error,
conserva los nuevos registros de arranque para identificarlo. El propietario confirmó el 5 de octubre de 2026 que la actualización en su Ubuntu
físico terminó correctamente tras instalar la dependencia.

El instalador del proyecto añade ahora `libqt6concurrent6` tanto a la
instalación como a la comprobación `--check`; el diagnóstico Linux también
consulta su versión. `build-linux.sh` lo entrega como `install-dependencies.sh`
y `build-multip-linux.sh` como `install-base-dependencies.sh`, llamado por el
instalador MultiP. La release 0.4.0 ya publicada sigue llevando la lista antigua.
La corrección no modifica ejecutables del core, perfiles ni partidas y no
requiere recompilar la interfaz. No se ha publicado una nueva release.

## Historial del diagnóstico y recuperación del ayudante

El 4 de octubre el mensaje disponible era solamente «La nueva versión se cerró
durante el arranque»; la primera captura correspondía a Rivals en Windows.
Por sí solo ese mensaje no permitía distinguir entre el cargador, Qt o la
aplicación. El paquete público `Emulos360-v0.4.0-linux-x86_64.tar.gz`, con SHA256
`2308cf7fdb45fd60670a1be413787d809364634914bfa9e8474d4fc51970126c`, coincidía con
el preparado para la release. Las pruebas aisladas 0.3.0 → 0.4.0 arrancaron
bajo WSL con bibliotecas de desarrollo instaladas; no reprodujeron la ausencia
de Qt Concurrent del equipo del propietario.

La reparación del ayudante conserva stdout y stderr de la nueva versión en
`data/update-startup.log`, muestra código de salida y últimos 4 KiB y guarda
`update-last-error.txt`. La restauración usa `update-rollback-startup.log` para
no sobrescribir la causa original. Los registros del 5 de octubre confirman
que este diagnóstico permitió identificar la dependencia omitida.

La aplicación ejecuta el ayudante de la versión instalada. Para una instalación
antigua que todavía no conserve los registros, cierra Emulos360 y ejecuta desde
el proyecto o el paquete de recuperación:

```sh
bash scripts/repair-linux-updater.sh "/ruta/a/tu/Emulos360"
```

Este script guarda una copia del ayudante anterior. Si actualizas a la release
0.4.0 publicada, vuelve a repararlo después: esa release lleva el ayudante
anterior. El procedimiento es independiente de instalar Qt Concurrent y no
hace falta repetirlo para diagnosticar los registros ya obtenidos.

Conserva `data/update-last-error.txt`, `data/update-startup.log` y
`data/update-rollback-startup.log`; con `--data-dir` estarán en esa carpeta.
`bash diagnose-linux.sh` genera un informe de bibliotecas y sesión gráfica.

El ayudante también trata fallos al cerrar el proceso o crear la carpeta de
trabajo, rechaza datos dentro de un directorio de runtime que se sustituye y
mantiene la restauración y los datos externos. La copia manual de
`scripts/update-linux.sh` incluye `updates`, para no dejar el ayudante antiguo.

## Evidencia y límites

- Incidencia real: registros de arranque y restauración y captura del propietario.
- Inspección: dependencia ELF directa de Qt Concurrent y proveedor en APT.
- Pruebas locales previas: siete casos del actualizador con datos ficticios,
  arranque, restauración y protección de rutas; no ejecutan juegos. Las primeras
  pruebas de la release incluían Edge, ya retirado del proyecto actual.
- Reproducciones históricas: `local/edge-integration/linux-update-launch-reproduction.json`,
  `linux-update-native-launch-reproduction.json` y
  `linux-update-fixed-helper-reproduction.json`.
- Validación de la corrección: sintaxis Bash correcta; `--check` detecta la
  ausencia de Concurrent sin llamar a APT; el modo instalación solicita el
  paquete y la comprobación posterior pasa con comandos aislados.
- APT simulado contra un estado dpkg vacío, sin recomendaciones: la lista
  anterior omitía exactamente `libqt6concurrent6` entre los proveedores de las
  21 dependencias ELF directas de la interfaz local; la lista corregida cubre
  todas. No se instalaron ni desinstalaron paquetes durante estas pruebas.
- Informe y comprobación reproducible en
  `local/linux-dependency-recovery-20261005/validation.json` y
  `check-ubuntu-runtime.py`.
- Confirmación del propietario (5 de octubre): actualización en Ubuntu físico
  completada; problema solucionado. Esa prueba corresponde a la corrección de
  dependencias de 0.4.0, previa a preparar la distribución 0.5.0.
