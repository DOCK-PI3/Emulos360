# Registro de cambios propios de Emulos360

Base actual: `AdrianCassar/xenia-canary`, rama `netplay_canary_experimental`, revisión
`6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c` (2026-09-20).
Base anterior: `xenia-canary/xenia-canary@6f9840568bb7dc9f5a9adb94801a99b18a7b2a60`.
Se conservan licencias, atribuciones y el stash `emulos360-before-netplay` de la
instalación local. No se han sobrescrito perfiles ni juegos para cambiar de base.

## Reparación Linux — 2026-09-29

- El importador ignora herramientas sin permiso de ejecución y busca 7-Zip en
  el sistema. El instalador repara permisos y comprueba las herramientas incluidas.
- Linux filtra los backends exclusivos de Windows y pasa los valores adecuados
  al motor; la ventana GTK utiliza X11/Xwayland para su superficie Vulkan XCB.
  Se conservan stdout/stderr en `core-console.log` y `profile-helper-console.log`.
- El paquete aporta el catálogo y el esqueleto originales del propietario como
  recursos, sin cuentas ni partidas; un catálogo personal conserva prioridad.
- XConfig se lee/escribe con copias de bytes para evitar accesos desalineados en
  sus estructuras empaquetadas. LZX utiliza desplazamientos sin signo; ambos
  defectos se reprodujeron con UndefinedBehaviorSanitizer en Linux.
- El adaptador POSIX traduce los errores de sockets al contrato Winsock del
  invitado; una operación de red conserva códigos de Xbox ya almacenados.
- Al cerrar Qt se desconectan las señales del proceso antes de destruir los
  miembros del controlador. Se corrige un acceso a un objeto ya destruido.
- Los scripts compilan el motor nativo y generan un `.tar.gz` con permisos 0755
  y sin `data`. El actualizador copia rutas de ejecución y conserva los datos.

## Pantalla de logros — 2026-09-22

- Sección Logros en escritorio y consola, con historial por perfil, progreso,
  puntos, búsqueda, filtros, fechas, iconos y ocultación de secretos.
- Comando IPC `achievements`: monta el perfil sin iniciar sesión, valida límites
  y cadenas de los GPD y reutiliza los lectores del motor. No escribe sus archivos.
  La UI evita lecturas simultáneas con una sesión, conserva la última lectura y
  descarta los datos al cambiar el perfil o la carpeta de contenido.
- `scripts/check-achievements-bridge.py --ui build/Release/ui_tests.exe` comprueba
  datos sintéticos, filtros y navegación. Verifica los hashes del perfil antes y
  después y comprueba perfiles vacíos/desconocidos y bases incompletas/corruptas.

## Integración 0.17 — biblioteca XBLA e importación de contenido

- `app/library.*` reconoce paquetes XBLA STFS bajo `000D0000`, además de GOD
  SVOD bajo `00007000`. Publica el formato en el modelo Qt; XBLA no exige los
  fragmentos `Data0000` propios de GOD. El escaneo admite atributos de sistema
  de copias Xbox, pero rechaza enlaces.
- `emulos_profile_bridge`: comando transitorio `import-content`, con una ruta de
  origen por operación. Inspecciona mediante `ProcessContentPackageHeader`,
  admite solo `kProfile`/`kSavedGame`, rechaza archivos no regulares y enlaces,
  y se niega a sobrescribir contenido existente. La extracción escribible usa
  `InstallContentPackage` de la base.
- `Emulator::ProcessContentPackageHeader` permite la inspección sin ventana y
  omite únicamente la decodificación de miniaturas cuando no existe un
  `ImGuiDrawer`; el flujo normal con interfaz conserva esa decodificación.
- Un perfil original se importó en el contenido interno y se seleccionó
  como jugador 1. Se instalaron seis partidas del disco original en interno y
  siete del pendrive en externo. Las dos variantes de Diablo III se conservaron
  en unidades distintas. Las copias `local/recuperacion-*` permanecen intactas.
- Antes de tocar el contenido activo se creó
  en `data/engine/profile-backups/`; la habilitación Netplay creó su
  propia copia previa. El gestor Netplay no se usa como instalador de contenido.

## Integración 0.16 — editor de avatares originales

- `emulos_avatar_assets.h`: lector acotado de TOC v1/v2, STRB, bloques LZX,
  mallas, índices, texturas BC1/2/3 y sustituciones geométricas de rasgos/ropa.
- `emulos_avatar_scene.h`: manifiesto original de 1000 bytes, ensamblaje de
  componentes, texturas faciales y cabello adaptado al sombrero.
- `emulos_avatar_animation.h`: lectura de poses comprimidas, cuaterniones y
  recurso privado del esqueleto. Estos formatos propios del fork no dependen
  de cabeceras o DLL del XDK al compilar/ejecutar el emulador.
- `AvatarStudio`, `AvatarEditor.qml`, materiales: Qt Quick3D nativo, articulación
  de mallas con las animaciones originales, selección de componentes y colores,
  giro y personalización. La animación se detiene al cerrar el editor.
- `emulos_profile_bridge`: `avatar-read` / `avatar-write`, montaje del perfil,
  copia previa, serialización GPD upstream y verificación tras desmontar/remontar.
  No inicia sesión ni cambia el jugador activo. No crea perfiles ficticios.
  Inicializa el GPD si el perfil nuevo solo contiene Account; rechaza un GPD
  existente inválido en lugar de sustituirlo por uno vacío.
- Importadores separados para el paquete del HDD y el esqueleto del XDK local;
  recursos privados en `local/` y `out/`, excluidos del parche/repositorio.
- `extract-hdd-profile.py`: lectura FATX de la partición de datos, selección por
  la identidad Account descifrada por el núcleo, extracción a destino nuevo,
  conservación de XUID/paquetes y verificación SHA-256. El nombre de presentación
  del contenedor STFS no se confunde con el gamertag. El primer backup
  inspeccionado no contenía la cuenta; el disco original montado después sí.
- `recover-usb-saves.py`: respaldo de contenedores originales desde un USB
  legible, validando propietario/título/tipo y SHA-256 de origen y destino.
  Mantiene los guardados separados del contenido activo; no cambia el XUID.
- La interfaz usa Qt Quick3D (GPL-3.0-only o licencia comercial, según sus
  cabeceras instaladas). Se reutilizan LZX de Xenia y libmspack; ver BUILD.md.
- **No implementa todavía la entrega de recursos CPU/GPU y objetos de animación
  por XAM a los juegos.** No se activa globalmente el stub de inicialización.

## Integración 0.15 — reparación de arranque y salas

- `PlayerServices`: vuelve a cargar TOML/XConfig tras el proceso de perfiles. La
  creación de `xconfig.settings` ya no invalida la siguiente selección/guardado.
  Descubrimiento al abrir Qt, selección automática únicamente si hay un solo
  perfil y ninguno activo. Errores de arranque visibles en diálogo.
- `emulos_profile_bridge`: comando `enable-netplay`, validación de XUID existente,
  copia previa del directorio de perfil/dashboard y conversión mediante la API
  upstream. Conserva XUID local y partidas; exporta `netplay` en el IPC. Qt ofrece
  botón por perfil y lo habilita para el perfil activo al descubrirlo con el modo
  Netplay ya elegido. No se conecta a servicios durante el proceso auxiliar.
- `NetplayRooms`: consultas de solo lectura permitidas con ajustes pendientes;
  consulta al entrar en salas y diagnóstico HTTP. No crea salas ficticias.
- Logs separados `data/core.log` y `data/profile-helper.log` mediante argumento
  explícito, evitando que actualizar perfiles borre el diagnóstico del juego.
- Avatares: lector acotado del cuerpo original, metadatos opacos, rechazo de
  manifiestos incompletos y validación básica de escritura; `AVATAR_ABI.md`.
  Modelos y animaciones 3D **no están terminados**.
- Comprobación focalizada del fallo comunicado: Gears of War llegó a «Press Start»
  mediante PlayerServices y Qt. Captura directa de GPU revisada. Consulta HTTPS
  real desde Qt devolvió cuatro salas en la comprobación; no acredita una partida
  multijugador entre equipos ni una campaña completa.

## Integración 0.14

- `core_window_host.*`, `GameSession.qml`: ventana extranjera de un PID validado
  alojada en Qt; navegación, foco y cierre normal. Solo Windows por ahora.
- `netplay_rooms.*`, `RoomsPanel.qml`: consulta HTTP nativa de salas públicas,
  filtros y apertura de GOD instalado. Las operaciones de red del juego siguen
  perteneciendo a Netplay upstream.
- `emulos_embedded`: flag transitorio en `emulator_window.cc` que deja la pantalla
  completa bajo control de Qt. No añade opciones persistentes al catálogo.
- En modo integrado, `XamLoaderLaunchTitle` sin ruta y
  `XamLoaderTerminateTitle` cierran el juego limpiamente al solicitar volver al
  dashboard, sin mostrar el diálogo de aserción del motor independiente.
- `build-engine.ps1` regenera version.h incluso en compilaciones incrementales,
  evitando mostrar el commit de la base anterior después de migrar a Netplay.
- `inspect-xbox-hdd.py`: inventario y extracción FATX seleccionada en solo lectura.
  Recursos privados en `/local/`; ninguna modificación de XAM de avatares se
  presenta como realizada. Véase `AVATAR_SYSTEM.md`.
- Comprobaciones propias en `tests/netplay_integration_tests.cpp`; no repiten
  suites de upstream ni arrancan juegos.

## Integración 0.13

El parche aplicable a la base actual es **`patches/xenia-netplay/0001-emulos360.patch`**.
Los tres parches de `patches/xenia/` son históricos de la base anterior; no deben
aplicarse encima de Netplay. El parche actual incluye:

- Marca/icono y puente de perfiles adaptados a Netplay; el cierre sin ventana
  comprueba el puntero del gestor UI y no elimina sesiones online.
- `Storage.emulos_external_content_root`, dispositivo externo ID 3/USBMASS,
  nombres de unidades, enumeración y espacio real. `content_manager.*` dirige
  apertura, creación, listado, miniaturas y borrado al dispositivo elegido.
  `xcontent_package.cc` conserva la identidad de unidad también en paquetes abiertos.
- Montaje `\\Device\\Emulos360External` y `usb0:`; las rutas de XamContentResolve
  y XamContentGetDeviceVolumePath reconocen la unidad externa.
- Servidor comunitario público como valor inicial. Actualizador binario upstream
  bloqueado para que no reemplace el motor que contiene nuestras extensiones.
- Comandos de diagnóstico transitorios `storage-check` (requiere marcador en
  carpeta desechable) y `catalog`; no aparecen como opciones del usuario.

La implementación de red, protocolos, sesiones y amigos es **de Netplay upstream**.
Emulos360 aporta la selección de modo en Qt, el lanzamiento del gestor nativo y
la integración con la biblioteca. No se atribuye la red al fork propio.

Fuera del núcleo: `ConsoleDashboard.qml`, `ConsoleTile.qml`, `gamepad_navigation.*`,
selección de unidad en `PlayerServices`/`SavesScreen`, catálogo de red actualizado
leyendo `.cc` **y `.cpp`**, y copias versión 2 con `Headers/00000001`.
Las copias de versión 1 siguen siendo legibles; no contienen cabeceras que nunca
se guardaron. La restauración versión 2 verifica ambos conjuntos antes de activar
la copia; conserva carpetas previas y revierte errores normales de renombrado.
Un corte de corriente entre renombrados puede requerir recuperar esas carpetas.

La navegación XInput solo envía entradas a la ventana Qt activa. Los juegos siguen
en el proceso del motor y reciben su propio sistema de mandos. En modo consola
se solicita pantalla completa al lanzar un título.

## Frontera del fork

`app/`, `assets/`, `scripts/`, `tests/` y la documentación del repositorio superior
son la capa de Emulos360. `engine/xenia` contiene el motor heredado. Los cambios
en archivos del motor se preservan como parches en `patches/xenia/`; no se da por
hecho que esos cambios existan en la rama principal de Xenia.

| Implementación propia | Archivos | Reutilización de Xenia |
| --- | --- | --- |
| Biblioteca GOD/XBLA | `app/library.*`, `controller.*`, `qml/LibraryScreen.qml`, `GameCard.qml` | Cabeceras XContent SVOD/STFS; lectura de originales |
| Carátulas regionales | `controller.*`, `qml/CoverPicker.qml` | No cambia el motor; véase COVERS.md |
| Marca e icono Orbit360 | `assets/`, `app/app.rc.in`, recursos Qt y scripts | Parches 0001 y 0002 |
| Catálogo de ajustes | `scripts/update-netplay-catalog.py`, `app/settings-catalog.json`, `settings-labels.json` | 282 opciones persistentes comprobadas contra el binario Netplay |
| Persistencia de ajustes | `app/engine_settings.*` | TOML nativo, tipos y secciones de Xenia |
| Ajustes de consola | `app/console_config.*` | Incluye el `xconfig.h` de Xenia: 22 campos/controles de su diálogo de consola |
| Presentación de ajustes | `qml/SettingsScreen.qml`, `SettingRow.qml`, `AppPreferences.qml` | Interfaz Qt propia; búsqueda, categorías, restauración e importación |
| Sesiones y perfiles | `app/player_services.*`, `qml/ProfilesScreen.qml` | Proceso del motor propio y `ProfileManager` nativo mediante parche 0003 |
| Avatares locales | `player_services.*`, `ProfilesScreen.qml` | PNG propios de la interfaz; no modifica el formato Avatar de Xbox |
| Partidas y copias | `app/save_store.*`, `qml/SavesScreen.qml` | Conserva `content/XUID/TitleID/00000001` y el guardado nativo de los juegos |
| Clientes comunitarios | `qml/CommunityScreen.qml`, `player_services.*` | Aplicaciones XEX independientes; se rechazan módulos DLL como títulos |
| Paquetes XBGuard | `app/community_package.*`, `CommunityScreen.qml`, `main.cpp` | Importación completa HDD/USB, análisis de launch.ini y manifiesto; no ejecuta plugins ni autentica |

## Parches históricos del motor anterior

1. `0001-emulos360-branding.patch`: nombre del binario y título de ventana.
2. `0002-emulos360-icon.patch`: recurso Windows del icono. El script de compilación
   copia `assets/emulos360.ico` del repositorio superior al directorio del motor.
3. `0003-emulos360-profile-bridge.patch`: módulo propio `emulos_profile_bridge.*`,
   integración en CMake y ruta de gestión sin ventana en `xenia_main.cc`.
   Añade una comprobación de puntero en `Emulator::Setup` para no acceder a ImGui
   en esta ruta. No se inicia ningún título durante una operación de perfiles.
   Conserva el constructor original de `Emulator` y añade una sobrecarga para
   aplazar el diálogo de primer inicio durante comandos sin ventana. El diálogo
   normal se conserva para la primera sesión de juego.

El puente recibe flags **transitorios**, nunca contraseñas:
`emulos_profile_command=list|create|enable-netplay|avatar-read|avatar-write|import-content`,
los parámetros propios de cada comando y `emulos_profile_output`. Devuelve TOML
versión 1 con `ok`, `error`, rutas instaladas cuando corresponda y una lista de
gamertags/XUID. El frontend usa un archivo único y espera el cierre del proceso.
El motor conserva su validación, generación de XUID y cifrado de `Account`.
La selección del jugador 1 escribe `Profiles.logged_profile_slot_0_xuid`.
Los otros tres puestos están disponibles en Ajustes → Perfiles.

La ruta de gestión inicializa los subsistemas necesarios del motor, incluida
la GPU sin presentación; no es un parser independiente de perfiles. Desactiva
la presencia Discord y la escritura de configuración al cerrar. El frontend
anula el inicio automático de los cuatro perfiles durante las consultas.

## Estado y límites explícitos

- El núcleo se ejecuta en un proceso y una ventana propios, con el TOML y las
  carpetas elegidos en Qt. No hay todavía una superficie de juego integrada en Qt.
- Los perfiles son cuentas locales reales de Xenia, no cuentas de Xbox Live.
- El editor de avatar dibuja una imagen personalizable para Emulos360. Se puede
  seleccionar un editor 3D `.xex` aportado por el usuario; varias funciones de
  `xam_avatar.cc` siguen marcadas como stub en la base y no se garantiza su funcionamiento.
- XBGuard ya está identificado y auditado con los archivos del usuario. Son
  tres DLL cargadas por DashLaunch, no una aplicación de login independiente.
  La importación está implementada, pero falta su entorno de ejecución y la
  autenticación. Evidencias y requisitos en `docs/XBGUARD.md`.
- Las copias son de partidas nativas. No son instantáneas de CPU/GPU/RAM ni
  permiten guardar en cualquier punto saltándose el sistema del juego.
- No se han descargado ejecutables, firmware, avatares propietarios ni juegos.

## Conservación de datos

Los originales de la biblioteca se abren en lectura. Configuración, perfiles,
avatares y copias usan `data/` junto al frontend, salvo rutas de contenido
personalizadas explícitamente en Ajustes. No se modifica la biblioteca para
guardar avatares o partidas.

El TOML se escribe mediante `QSaveFile`, conserva claves desconocidas y crea
copias `.bak` antes de reemplazar archivos existentes. La reescritura normaliza
el formato y no conserva comentarios en el archivo activo. El archivo `.bak`
mantiene el texto previo. XConfig conserva bytes ajenos a los campos editados.
Se rechazan cambios si los archivos cambiaron externamente desde la lectura.

Durante una sesión se bloquean los ajustes y las operaciones de copia/restauración.
Un `QLockFile` en la carpeta de contenido coordina instancias de Emulos360; no
puede impedir que un Xenia iniciado por separado use esa misma carpeta. Cerrar
ese otro motor antes de gestionar partidas. La aplicación Qt permanece abierta
mientras una sesión o una operación de archivos está activa.

Las copias incluyen manifiesto SHA-256. Se verifican al restaurar, se rechazan
enlaces/junctions y se mantiene el XUID original. La restauración primero copia
a una carpeta temporal hermana y mueve la versión anterior a
`00000001.emulos-before-<id>`. Si se interrumpe una copia, pueden quedar carpetas
incompletas; no se presentan como copias válidas sin manifiesto completo.

## Mantenimiento y validación

Al actualizar Xenia, fijar la nueva revisión, revisar/aplicar los parches en orden,
comparar `xconfig.h` y regenerar el catálogo desde una configuración de esa misma
revisión. No regenerar a partir de un TOML personalizado sin revisar sus valores
predeterminados. El puente se documenta como extensión, no como API upstream.

Comprobaciones focalizadas nuevas: `tests/services_tests.cpp` (ajustes y copias)
y `scripts/check-profile-bridge.py` (IPC y persistencia de perfiles en una carpeta
temporal). Las compilaciones habituales mantienen las pruebas desactivadas. Los
resultados concretos se registran en `docs/VALIDATION.md`.
