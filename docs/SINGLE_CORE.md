# Xenia Canary como único core

Retirada solicitada el **5 de octubre de 2026**, proyecto activo
`C:/Users/mabe8/Documents/GitHub/Emulos360`. Estado: **retirada terminada y verificada en el proyecto**. Builds de interfaz
y regresiones Windows/Ubuntu completadas. No se ha publicado otra release.

Los juegos GOD/XBLA/XEX, las operaciones de perfiles y logros, los avatares nativos,
el dashboard, Netplay y MultiP utilizan exclusivamente `engine/Emulos360-core`.
Se han retirado la selección de proveedor, las rutas externas de Nexia/Edge,
la importación del runtime XNA y el soporte de biblioteca XBLIG.
No se han portado cambios de esos motores al core principal.

## Ajustes gráficos y preferencias anteriores

Ajustes → Emulos360 → Gráficos conserva GPU global. Biblioteca → Gráficos conserva
GPU por título. Direct3D 12 está disponible en Windows y Vulkan en ambas plataformas.
«Usar ajustes del motor» respeta la configuración principal.

El 5 de octubre se añaden [perfiles gráficos globales y por juego](GRAPHICS_PROFILES.md),
desde 720p mejorado hasta 4K. Se aplican al iniciar mediante los controles
existentes de resolución/CAS/FSR. Predeterminado recupera los valores originales
de esos controles y Personalizado conserva la configuración manual anterior.

`data/cores.ini` conserva `renderers/netplay` y las opciones gráficas válidas por
Title ID. Al abrir la interfaz se respalda el archivo anterior como
`cores.ini.before-single-core` y se eliminan únicamente las selecciones de core y
las GPU globales de proveedores retirados. Si existe una copia anterior, se usa
un nombre adicional; no se sobrescribe. La migración no accede a perfiles/partidas.

## Preservación de datos

No se borran ni mueven carpetas `data`, `.before-shared`, perfiles, partidas,
contenido instalado o enlaces de las vistas antiguas. Los datos históricos de
Edge/XBLIG permanecen en sus ubicaciones originales. Los guardados indie bajo
`data/cores/xblig/saves` se conservan separados por juego; ya no tienen lanzador.
Esto evita romper posibles enlaces de un perfil a esos almacenes.
Los frameworks XNA instalados en Windows permanecen instalados.

La instalación personal `Desktop/Emulos360-BETA12` permanece intacta; los cambios
se preparan en el proyecto, sin sustituir su ejecutable en uso. Antes de la
retirada se guardaron las fuentes y una huella del core y de 293 archivos de
perfiles, partidas y ajustes en `local/core-removal-20261005` (ignorado en Git).

## Builds y actualización

Los empaquetadores distribuyen solo el motor principal y excluyen los dos
proveedores aunque la carpeta de salida contenga restos de builds anteriores.
La validación de Windows/Linux exige solo la interfaz, el core principal y el
instalador de su plataforma. Se conserva la reparación de rollback y registros
de arranque Linux. Las actualizaciones sustituyen `engine` y preservan `data`.

**Transición desde la release 0.4.0:** su instalador anterior exige Edge. Antes de
publicar un paquete sin Edge debe actualizarse ese helper en la instalación
antigua, o realizar la primera sustitución manual conservando `data`.
Copiar `updates/apply-update-windows.ps1` o `updates/apply-update-linux.py` de esta
revisión a la carpeta `updates` de la instalación, conservando una copia anterior,
resuelve ese requisito obsoleto. Se entregan `repair-windows-updater.ps1` y `repair-linux-updater.sh` con copias
de seguridad en el ZIP de reparación de 0.5.0; véase [la release](RELEASE_0.5.0.md).
La versión se ha elevado a 0.5.0 por solicitud del propietario; la publicación
corresponde al usuario y permanece pendiente.

## Verificación

Las interfaces compilan en Windows y Ubuntu 26.04. CTest pasa 4/4 en ambas
plataformas. Las pruebas adicionales de perfiles/partidas, rutas Netplay y
migración de GPU pasan; ASan/UBSan no detectan errores en esa migración.
La prueba Windows utiliza además el core real para crear, leer y habilitar un
perfil temporal para Netplay. Los empaquetadores e instaladores pasan sus
fixtures sin Edge/Nexia y conservan perfiles/guardados en éxito y fallo.

La interfaz de la build portable Windows arranca y su captura se ha revisado
con datos de prueba vacíos. Detecta los **84 títulos GOD/XBLA/XEX** de la biblioteca. Los 293 archivos protegidos y el código/binario nativo permanecen
idénticos. Se conserva el archivo de recuperación Linux anterior.

Build local: `out/Emulos360-MultiP`; solo contiene `engine/Emulos360-core.exe`.
No incluye `data` ni cores retirados. Fue una build de desarrollo 0.4.0,
sin una release nueva. Las builds/descargas antiguas se conservan ignoradas en
Git como historial, excluidas de los empaquetadores actuales.

[Evidencia reproducible y límites](research/xenia-canary/single-core-retirada-evidencia-20261005.json).
No se han probado partidas reales ni ampliado compatibilidad de juegos en esta
retirada. El 5 de octubre se preparan distribuciones Windows/Linux 0.5.0 con
las interfaces recompiladas y los mismos binarios de sus cores principales.
Las pruebas de gameplay y la publicación corresponden al usuario.

## Entrega 0.5.0

Interfaces Release Windows/Ubuntu 26.04 compiladas; CTest 4/4 en ambas.
Arranque de los runtimes comprobado con datos temporales; dependencias Linux
completas, incluida Qt Concurrent. Regresiones del actualizador y reparación
Windows con conservación de datos comprobadas. ASan/UBSan de la migración GPU
sin errores. El propietario confirmó solucionada su actualización Linux.
Paquetes y notas: `out/release-v0.5.0`. Véase [la entrega](RELEASE_0.5.0.md).
