# Preparar y subir el código

Este repositorio contiene el frontend C++/QML, el servicio privado, scripts,
pruebas, recursos propios y un submódulo de Xenia Canary Netplay fijado a la
revisión `6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c`. Las modificaciones
de Emulos360 para el motor están en `patches/xenia-netplay/0001-emulos360.patch`.
Git no incorpora los cambios locales de un submódulo al subir el repositorio
principal; `scripts/prepare-engine.ps1` y `.sh` aplican el parche al clonar.

No se incluyen `out/`, `local/`, `.tools/`, compilaciones, dependencias Node,
juegos, perfiles, partidas, carátulas descargadas ni registros. La instalación
local sigue donde está y no se modifica al publicar. El dashboard Metro propio
sí se incluye como `XEXplugins/MetroDashboard/package/default.xex` junto a su
`font.png`, para que ambas builds puedan empaquetarlo desde un clon nuevo.
Reconstruir ese `.xex` requiere un XDK instalado por separado. Antes de publicar
el repositorio o una build, comprueba los términos de distribución aplicables
al binario generado con el XDK.

## Primer envío

El repositorio local ya está preparado en la rama `main`. Crea en GitHub un
repositorio vacío, sin README ni `.gitignore` iniciales, y copia su URL. Desde
PowerShell en la raíz:

```powershell
git remote add origin <URL_DEL_REPOSITORIO>
git push -u origin main
```

Si ya existe un remoto `origin`, usa `git remote set-url origin <URL_DEL_REPOSITORIO>`.
Comprueba primero la URL con `git remote -v`. No uses `git push --force` para
resolver una divergencia de historias: integra antes los commits del remoto.

## Comprobar el clon

```powershell
git clone --recurse-submodules <URL_DEL_REPOSITORIO> Emulos360-fresh
cd Emulos360-fresh
./scripts/prepare-engine.ps1
git -C engine/xenia apply --reverse --check ../../patches/xenia-netplay/0001-emulos360.patch
```

En Ubuntu, usa `bash scripts/prepare-engine.sh`. Consulta [BUILD.md](BUILD.md)
para compilar. Las fuentes de voz e ISO2GOD se preparan con
`bash scripts/setup-linux-sources.sh`; el XDK solo se necesita para reconstruir
el dashboard. El empaquetado MultiP para Linux aún
parte de un servidor preparado en Windows, como describe su script.

## Licencias y futuras versiones

El código propio aún no tiene una licencia general declarada: publicar el
repositorio no concede por sí solo permisos de reutilización. Antes de
distribuir binarios públicos, elige una licencia compatible con las
dependencias y revisa los avisos de Qt Quick3D, Xenia, ISO2GOD, 7-Zip y las
bibliotecas de voz y del dashboard generado con el XDK.

La versión de la aplicación sale de `project(Emulos360 VERSION ...)` en
`CMakeLists.txt`. Cada futura versión distribuible deberá llevar una etiqueta
`v<versión>`, notas de cambios y paquetes separados para Windows y Linux.
El plan del actualizador está en [UPDATES.md](UPDATES.md).
