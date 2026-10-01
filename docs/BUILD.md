# Compilación y ejecución

## Interfaz Windows

Requisitos comprobados: Visual Studio Build Tools 2022 con C++, CMake y Python.
El SDK Qt se descarga dentro de `.tools/`; Python se usa para herramientas, no
para implementar la aplicación.

Desde PowerShell en la raíz:

```powershell
./scripts/setup-qt.ps1
./scripts/build-ui.ps1 -Package
./out/Emulos360/Emulos360.exe --library 'D:\XBOX360_GAMES'
```

También se puede abrir `out/Emulos360/Emulos360.exe` y seleccionar la carpeta en la UI.
La ventana abre maximizada. Durante el arranque se reproduce una intro de 6
segundos mientras se escanea la biblioteca; si el escaneo tarda más, se muestra
«Cargando biblioteca» hasta que termine. En **Ajustes > Emulos360** se puede
elegir y previsualizar Aurora (azul) o Nova (violeta/coral). Los MP4 y sus
miniaturas se copian a `out/Emulos360/intro/`.
F11 desde la biblioteca abre el dashboard Metro `.xex`; Escape vuelve al inicio o abre la guía.
Inicio/Home abre la guía. Las preferencias quedan
en `data/settings.ini` junto al ejecutable; `--data-dir` permite otra ubicación.
Existe un puente nativo XInput en Windows: A seleccionar, B volver, cruceta/stick
navegar, LB/RB cambiar sección, Start abrir guía y Back avanzar el foco.
En Linux, SDL2 ofrece los mismos controles cuando la biblioteca tiene el foco.
En la biblioteca, X abre la confirmación para borrar el juego seleccionado;
F8 hace lo mismo desde el teclado y el clic derecho lo abre sobre la carátula.
En el diálogo, izquierda/derecha seleccionan Cancelar o Eliminar, A activa la
opción enfocada y B cancela. Después del borrado se retiran los directorios
vacíos hasta la raíz de la biblioteca, que siempre se conserva.
En Windows funciona tanto en escritorio como en consola cuando la interfaz tiene
el foco; cede el mando al motor al enfocar el juego y permite navegar por la
biblioteca aunque la sesión siga abierta. En Linux, la navegación del mando
se suspende mientras hay una sesión de juego activa. Al volver del juego se
recupera el foco de navegación.
La ventana conserva su tamaño o estado maximizado al iniciar juegos; solo las
vistas de pantalla completa cambian ese estado temporalmente.
Durante una partida integrada en Windows, F8 o pulsar el stick derecho alterna
la franja superior en pantalla completa. Con la franja oculta, el juego ocupa
toda la ventana. F11 sale de pantalla completa y recupera los controles.
La comprobación con un mando físico sigue pendiente.

## Linux x86-64

La build Linux se genera por separado en `out/Emulos360-linux/`; no utiliza
`out/Emulos360/` ni copia los datos personales de Windows. La build actual se
compiló en Ubuntu 26.04 y necesita glibc 2.43 y Qt 6.10.2 o posterior. En ese
sistema, el paquete incluye un instalador de las dependencias de ejecución:

```sh
cd /ruta/a/Emulos360-linux
bash install-dependencies.sh
./Emulos360
```

`bash install-dependencies.sh --check` comprueba las bibliotecas y los permisos
sin instalar nada. `--launch` instala las dependencias y abre la aplicación en
la sesión gráfica actual. El script detecta una versión de glibc o Qt demasiado
antigua y muestra el motivo; no sustituye bibliotecas básicas del sistema.

Para instalar también las herramientas de compilación en Ubuntu 26.04, ejecuta
`bash scripts/install-ubuntu-deps.sh --build` desde el árbol del proyecto. Esto
incluye CMake, Ninja, Clang, LLD, Rust/Cargo, Qt (Quick, Controls, Quick3D,
Multimedia y ShaderTools), SDL2, Vulkan, libcurl, liblz4, libgtk-3, libc++,
SPIR-V/glslang y `7zip`.

Las fuentes fijadas de libdatachannel, Mbed TLS, Opus e ISO2GOD se descargan a
`.tools/` mediante `scripts/setup-linux-sources.sh`. El script de compilación
lo ejecuta automáticamente. Desde una terminal Linux en la raíz del proyecto:

```sh
bash scripts/build-linux.sh
./out/Emulos360-linux/Emulos360 --library /ruta/a/los/juegos
```

`bash scripts/build-linux.sh --frontend-only` compila la interfaz sin el motor
para desarrollo. La build enlaza Qt y las bibliotecas del sistema de forma
dinámica; para ejecutarla en otra distribución conviene reconstruirla allí o
preparar un paquete con sus dependencias. El motor Linux sigue siendo
experimental y requiere una GPU con Vulkan para ejecutar juegos.
La selección de eventos de red usada por netplay todavía no está implementada
en Linux; falta validar juegos y mandos en un equipo Linux con GPU física.

La edición MultiP se compila con `bash scripts/build-multip-linux.sh` en
`out/Emulos360-MultiP-linux/`. El script compila el motor desde sus fuentes y
genera `out/Emulos360-MultiP-linux.tar.gz` con permisos de ejecución y sin datos
personales; extrae ese archivo desde Ubuntu. `--frontend-only` conserva el
motor existente para desarrollo. Instalación y actualización conservando
perfiles: [LINUX.md](LINUX.md).

Las comprobaciones Linux focalizadas están en `tests/linux-runtime`; utilizan
un perfil temporal y admiten un juego local mediante `EMULOS_LINUX_GAME` y un
motor mediante `EMULOS_LINUX_CORE`. Resultados y límites en [VALIDATION.md](VALIDATION.md).

La biblioteca detecta paquetes GOD/XBLA y ejecutables de juego XEX2. Para una
copia extraída, apunta a la carpeta que contiene los directorios de los juegos;
se usa `default.xex` cuando existe. Los módulos DLL y el dashboard del sistema
no se muestran como juegos. La detección no garantiza que Xenia pueda ejecutar
cada título ni comprueba que estén presentes todos sus archivos auxiliares.

### Recursos privados para el editor de avatares

`setup-qt.ps1` instala también Qt Quick3D, ShaderTools y QuickTimeline 6.8.3.
`windeployqt` recoge sus módulos nativos. La build personal preparada ya contiene
los siguientes recursos bajo `out/Emulos360/data/avatar-system`:

```powershell
python scripts/import-avatar-resources.py 'local/avatar-system-from-hdd/SystemAuxiliary/Content/0000000000000000/FFFE07DF/00008000/FFFE07DF00000002' out/Emulos360/data/avatar-system
python scripts/import-avatar-rig.py out/Emulos360/data/avatar-system
```

El primer importador comprueba SHA-1 de bloques STFS y extrae solo los dos TOC.
El segundo usa las herramientas XDK instaladas y `XEDK` para obtener el esqueleto.
Ambos rechazan sobrescribir recursos diferentes. El HDD y los paquetes origen
se mantienen en solo lectura. No hay dependencia del XDK al ejecutar la build.
Estos archivos no pertenecen al parche ni deben añadirse a un paquete público.

Vista de diagnóstico sin modificar perfiles:

```powershell
./out/Emulos360/Emulos360.exe --avatar-preview
```

`--avatar-manifest archivo` previsualiza un manifiesto original de 1000 bytes.
La pantalla necesita el renderizador nativo; `QT_QUICK_BACKEND=software` no
admite Quick3D. El editor experimental XEX y el renderizado dentro de los juegos
continúan pendientes, como se detalla en `AVATAR_SYSTEM.md`.

Dependencias añadidas: Qt Quick3D declara `LicenseRef-Qt-Commercial OR GPL-3.0-only`
en sus cabeceras, no LGPL; la licencia aplicable deberá conservarse al distribuir.
El decodificador reutiliza LZX de Xenia (BSD) y libmspack (LGPL-2.1) del árbol
upstream. Las herramientas de importación no incorporan código binario del XDK
a los ejecutables. Esta entrega es una build privada local, sin publicación.

Inventario sin abrir la ventana:

```powershell
./out/Emulos360/Emulos360.exe --inventory --library 'D:\XBOX360_GAMES'
```

Las cabeceras y los fragmentos se abren en solo lectura. `complete` en el JSON
significa presencia básica de fragmentos, nunca verificación criptográfica.

## Motor de referencia

La base actual es Netplay `6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c`.
Este espacio de trabajo ya tiene las extensiones aplicadas. Tras clonar el
repositorio, prepara el submódulo y aplica el parche sobre esa revisión limpia:

```powershell
git submodule update --init --recursive
./scripts/prepare-engine.ps1
```

El icono elegido es Orbit 360. Los recursos `assets/emulos360.png` e
`assets/emulos360.ico` se generan desde la opción 2 con
`./scripts/build-icon.ps1`. El ICO incluye 16, 24, 32, 48, 64, 128 y 256 píxeles.
`build-engine.ps1` copia ese icono al fork antes de compilar sus recursos.

En Linux usa `bash scripts/prepare-engine.sh`. Los scripts de compilación llaman
al preparador automáticamente y detectan si el parche ya está aplicado. El
submódulo fija una revisión concreta; sus cambios propios están en el parche del
repositorio principal, porque Git no sube cambios locales de un submódulo.
El motor usa su propio CMake y las herramientas de Visual Studio detectadas por
su script. En este equipo seleccionó Build Tools 2026, mientras Qt usa 2022.

```powershell
./scripts/setup-shaders.ps1
./scripts/build-engine.ps1 -Jobs 4
```

Se necesitan `glslangValidator`, `spirv-opt` y `spirv-dis` en PATH. Se pueden
obtener con el SDK oficial de Vulkan. Para esta preparación local se descargó
glslang del proyecto Khronos y se compilaron las herramientas SPIR-V de la
revisión fijada por Canary:

- [glslang main-tot Windows release](https://github.com/KhronosGroup/glslang/releases/download/main-tot/glslang-main-windows-x86_64-release.zip).
  SHA256 del archivo descargado: `1B505A8EA63D4921C2B62E62804A25202FE35F0DF3EAA3D7ACAC323BC0A52CEA`.
  El alias local `glslangValidator.exe` es una copia de `glslang.exe`.
- SPIRV-Tools: `engine/xenia/third_party/SPIRV-Tools`, revisión
  `33e02568181e3312f49a3cf33df470bf96ef293a`.
- SPIRV-Headers: `2a611a970fdbc41ac2e3e328802aed9985352dca`.

El enlace main-tot es mutable: conservar el hash o sustituirlo por un artefacto
fijado al reproducir el entorno. No ignorar diferencias de hash.

Para compilar las herramientas con esos fuentes:

```powershell
cmake -S engine/xenia/third_party/SPIRV-Tools -B .tools/spirv-build -G 'Visual Studio 17 2022' -A x64 -DSPIRV_SKIP_TESTS=ON -DSPIRV_SKIP_EXECUTABLES=OFF -DSPIRV-Headers_SOURCE_DIR=C:/ruta/absoluta/a/SPIRV-Headers
cmake --build .tools/spirv-build --config Release --target spirv-opt spirv-dis --parallel 4
```

Utilizar barras `/` en la ruta CMake de SPIRV-Headers. No copiar juegos, partidas
ni logs a Git. Las carpetas `.tools`, `build*`, `local` y `out` están ignoradas.

## Verificación opcional

No se construyen ni ejecutan pruebas por defecto. `-Test` habilita las pruebas
locales explícitamente; usarlo solo cuando un cambio propio lo justifique.
No repetir pruebas del motor upstream sin cambios.

El motor renombrado se genera como
`engine/xenia/build/bin/Windows/Release/Emulos360-core.exe`.


`library_tests`: cabeceras truncadas, endian, rechazo de otro contenido, límites
de fragmentos y conservación de paquetes al escanear.
`ui_tests`: carga QML, escaneo asíncrono, filtro, pantalla completa y persistencia.
Los resultados detallados se escriben en `build/*-results.txt`.

El renderizado offscreen sirve para comprobar diseño y navegación; no mide el
rendimiento de la GPU ni sustituye una prueba de juego real. Linux y mando físico
requieren validación posterior.
# Integración de perfiles y partidas (2026-09-18)

Compilar primero el núcleo con `scripts/build-engine.ps1`, y después ejecutar
`scripts/build-ui.ps1 -Package`. El paquete copia el núcleo a
`out/Emulos360/engine/Emulos360-core.exe`; Qt lo usa tanto para sesiones como
para el puente de perfiles. La instalación de desarrollo y el paquete usan
carpetas de datos separadas salvo que se pase `--data-dir`.

Para revisar un apartado directamente: `Emulos360.exe --page settings`,
`--page profiles`, `--page achievements`, `--page saves` o `--page community`.
Logros requiere el motor actualizado; `--console --page achievements` abre la
misma sección en el dashboard.

Pruebas focalizadas opcionales de las extensiones propias:

```powershell
cmake -S . -B build -DEMULOS_SERVICE_TESTS=ON
cmake --build build --config Release --target services_tests
$env:PATH = "$PWD/.tools/Qt/6.8.3/msvc2022_64/bin;$env:PATH"
./build/Release/services_tests.exe
python scripts/check-profile-bridge.py
python scripts/check-storage-bridge.py
python scripts/check-netplay-catalog.py
```

Los comandos Python requieren el núcleo compilado y usan carpetas temporales sin
abrir títulos. Para esta integración se ejecutaron solo los casos Qt
`storageDevicesAndMetadata saveBackupRestoreAndCorruption`, no toda la suite.
`-DBUILD_TESTING=OFF` sigue siendo el valor normal; la opción
`EMULOS_SERVICE_TESTS` también está desactivada por defecto en un build nuevo.

Comprobación focalizada de la pantalla de logros, con `ui_tests` compilado:

```powershell
python scripts/check-achievements-bridge.py --ui build/Release/ui_tests.exe
```

Crea un perfil desechable y GPD sintéticos, verifica la lectura y sus hashes,
y comprueba filtros, navegación y cambio de perfil en QML. También admite
`--ui build-asan/Release/ui_tests.exe` para la variante AddressSanitizer.
