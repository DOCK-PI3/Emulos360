# Validación del primer prototipo — 2026-09-15

## XBLA e importación de perfil — 0.17, 2026-09-22

- `local/xbla-library-results.txt`: 9 resultados aprobados, 0 fallos. Cubre
  cabecera XBLA/STFS, GOD/SVOD, rechazo de tipos y límites inválidos, XBLA sin
  fragmentos GOD y GOD incompleto.
- Inventario final de `D:\XBOX360_GAMES`: 16 GOD y 14 XBLA, 30 títulos. Los 14
  XBLA tienen cabecera válida y aparecen completos según las reglas del formato.
  Este inventario no afirma que cada juego complete su arranque.
- El nuevo `import-content` se ejecutó primero en
  `local/profile-import-command-check/` con un perfil de prueba y el paquete
  GearsCheckpoint. Ambos fueron inspeccionados e instalados por el núcleo en una
  raíz desechable; los TOML devolvieron `ok=true` y la ruta instalada.
- En el paquete activo se creó una copia previa y se importaron el perfil, seis
  partidas del disco original en interno y siete del pendrive en externo. El
  núcleo lista el perfil importado con Netplay habilitado; el TOML
  del paquete lo selecciona como jugador 1.
- Se conservaron por separado las dos versiones de Diablo III. Los archivos de
  recuperación se leyeron como fuente y permanecen intactos.

No se inició ningún juego, campaña ni sesión online durante esta entrega. La
validación de compatibilidad se hará al probar cada título y guardado.

## Integración Netplay y consola — 2026-09-20

Estado actual detallado: [STATUS.md](STATUS.md).
Compilados núcleo Netplay y frontend Release; motor copiado al paquete con hash
coincidente. Registros: `local/netplay-build-final.log`,
`local/netplay-package-final2.log`.

- Puente nativo de dos unidades: crear, enumerar, cerrar/reabrir y desconectar,
  con contenido distinto por unidad: superado (`check-storage-bridge.py`).
- Puente de perfiles adaptado a Netplay: crear/listar/persistencia Account:
  superado (`check-profile-bridge.py`).
- Pruebas de metadatos, aislamiento, modo de red y copias alteradas: superadas
  (`local/storage-services-results.txt`). Se corrigió la omisión de `.cpp` al
  leer opciones de red.
- Catálogo comparado con el binario: 282 claves, sin diferencias
  (`local/netplay-catalog-report.json`). Se excluyeron dos flags bajo `#if 0`
  y un flag exclusivo de Android.
- Capturas `local/console-dashboard-final.png`, `local/storage-final.png` y
  `local/netplay-final.png`, revisadas; sin mensajes de error QML.
- Parche completo de extensiones contra Netplay verificado por aplicación inversa.

No se han iniciado títulos ni conexiones de juego online. Quedan pendientes
campañas, sesión entre dos participantes y comprobación con mando físico.

## Historial anterior

Registro histórico. Desde el 2026-09-17 no se repiten pruebas del motor sin
cambios por indicación del usuario. La compilación normal excluye las suites.

Actualización Emulos360 0.2: interfaz y motor renombrado compilados en Release;
captura revisada del nuevo diseño nativo. Se consultaron imágenes regionales
de Gears 1 y la fuente alternativa x360db durante la implementación. No se
ejecutaron juegos, CTest, sanitizadores ni pruebas de rendimiento en este cambio.
La descarga y elección de carátulas está implementada; su interacción completa
en el paquete no se ha probado automáticamente.

## Interfaz

- C++20 / Qt 6.8.3 / MSVC 19.44 / Windows SDK 10.0.26100.0.
- Compilación Release completada.
- Paquete autónomo generado en `out/LOS360`; apertura y capturas de escritorio
  y consola verificadas sin añadir el SDK de Qt al PATH. Ruta de biblioteca
  del usuario guardada en los ajustes locales del paquete.
- Análisis `los360_qmllint` completado sin advertencias tras registrar el tipo
  C++ del controlador explícitamente y regenerar los metadatos obsoletos.
- CTest: 2 ejecutables de prueba, ambos superados.
- Lector: truncamiento en todos los tamaños inferiores a la cabecera mínima,
  endian, contenido incorrecto, conteos fuera de rango, fragmentos ausentes y
  no modificación del paquete fuente.
- UI: carga QML, resultados asíncronos, filtro de búsqueda, cambio de modo y
  persistencia de preferencias. Se corrigió una carrera entre la finalización
  del worker y la publicación del resultado en el hilo de interfaz.
- AddressSanitizer: pruebas del lector superadas; MSVC no proporciona UBSan,
  por lo que queda pendiente para un entorno Clang/GCC.
- Capturas offscreen y captura con GPU real revisadas. El registro de Qt
  identifica AMD Radeon RX 6600 XT; no se forzó dispositivo de software en esa
  ejecución. No se midió latencia de entrada ni rendimiento de emulación.

## Biblioteca real, solo lectura

13 paquetes GOD reconocidos en la ruta completa. Para los títulos prioritarios:

| Título | Title ID | Fragmentos presentes/esperados | Tamaño aproximado |
| --- | --- | --- | --- |
| Gears of War | 4D5307D5 | 40/40 | 6,27 GiB |
| Gears of War 2 | 4D53082D | 40/40 | 6,28 GiB |
| Gears of War: Judgment | 4D530A26 | 45/45 | 7,13 GiB |

No se comprobaron firmas, hashes de contenido, arranque ni campañas. El indicador
de presencia de fragmentos nunca se presenta como compatibilidad jugable.

## Motor

Revisión: `6f9840568bb7dc9f5a9adb94801a99b18a7b2a60`.
Preparación de submódulos y configuración CMake completadas. Se resolvieron la
carga del entorno de Visual Studio y la ausencia de herramientas GLSL/SPIR-V.
Compilación Release completada: `engine/xenia/build/bin/Windows/Release/xenia_canary.exe`.

Prueba breve de arranque de Gears 1 con Direct3D 12, cerrada al recoger el log:
el motor seleccionó la RX 6600 XT, mapeó los 40 fragmentos SVOD, cargó
`GAME:\default.xex`, identificó Title ID `4D5307D5` y lanzó el módulo invitado.
Se observaron lecturas de recursos de WarGame y creación de hilos invitados.
Esto prueba la ruta básica de carga; NO demuestra imagen correcta, audio,
campaña jugable, guardado ni estabilidad prolongada. Datos y logs de la prueba
quedaron en `local/xenia-baseline`, fuera de D: y de Git. Proceso cerrado.

## Pendiente

Integración de la superficie del motor en Qt, entrada con mando físico, pruebas de campaña,
mediciones de rendimiento, revisión con escala de texto aumentada y validación
en Linux. El prototipo no representa todavía el acabado visual final.

## Extensiones Emulos360 — 2026-09-18

- Compilados el frontend Qt y el puente nativo de perfiles. El paquete incorpora
  `engine/Emulos360-core.exe` y comparte rutas/configuración mediante argumentos.
- Ajustes: 266 opciones persistentes de la revisión fijada y 22 controles de
  consola. Comprobación focalizada de persistencia TOML/XConfig, validación de
  valores, preservación de claves importadas y detección de edición externa.
- Copias: comprobada copia/restauración con datos ficticios, conservación de la
  versión anterior y rechazo de una copia cuyo contenido fue alterado.
- `services_tests`: 5 resultados correctos, 0 fallos (incluye inicio y cierre del
  ejecutable QtTest; tres casos funcionales). Datos en directorios temporales.
- `scripts/check-profile-bridge.py`: lista vacía, creación de perfil local,
  presencia del Account nativo y lectura del mismo XUID/gamertag en otro proceso.
  Se corrigió el bloqueo del diálogo inicial en la nueva ruta sin ventana.
- Capturas de Ajustes, Perfiles, Partidas y Comunidad revisadas. Corregidos el
  contraste de pestañas y un bucle de tamaño del diálogo de restauración.
  No se ejecutaron títulos ni
  pruebas de campaña del motor upstream durante este cambio.
- Sin validación de autenticación con XGuard, avatares 3D, ni guardado en una
  campaña real. Estas limitaciones se explican en `FORK_CHANGES.md`.

Los registros locales de esta integración están en `local/build-final-ui.log`,
`local/build-profile-bridge.log` y `local/services-results.txt` (fuera de Git).

## Importador XBGuard — 2026-09-19

- Compilación Release y paquete actualizados (`local/build-xbguard.log`).
- Inspección estática de ambas variantes del usuario: tres módulos DLL,
  dependencias presentes y orden de carga conservado. Informes locales en
  `local/xbguard-hdd-audit.json` y `local/xbguard-usb-audit.json`.
- Importada la variante HDD en `out/Emulos360/data/community/xbguard/`, con los
  cuatro archivos originales y manifiesto propio. Hashes comparados después de
  copiar; no se modificó Downloads. `runtimeSupported` continúa siendo false.
- Ejecutado únicamente el caso nuevo `communityPackageIntegrityAndPaths`:
  conservación de INI y recursos adicionales, rechazo de variante incorrecta,
  rutas con `..`, plugins truncados y cabeceras fuera de límites. Superado.
- Pantalla Comunidad con el paquete importado capturada y revisada; registro
  QML sin errores. No se ejecutaron plugins, juegos ni conexiones de XBGuard.
- Alternativas de online investigadas en fuentes primarias y documentadas en
  `NETPLAY_OPTIONS.md`; no se instaló Netplay, VPN, driver ni servidor.
# Integración Qt de ventana y salas — 2026-09-20

Comprobaciones nuevas limitadas a Emulos360: parser de sesiones, consulta HTTP
local y cancelación (4 resultados aprobados contando inicio/cierre), contenedor
con el proceso real del núcleo sin juego (3 aprobados: inicio, ventana/cierre,
limpieza). El caso nativo comprueba PID/padre/WS_CHILD, redimensionado y visibilidad.
Resultados en `local/netplay-integration-results.txt` y `local/native-window-final.txt`.
El aviso inicial del core impidió el primer intento; el caso final lo atiende
exclusivamente en el proceso temporal lanzado por la comprobación.

La consulta pública GET `/sessions` devolvió HTTP 200 y ninguna sala anunciada.
Captura nativa Qt de Comunidad: `local/rooms-final.png`. Los recursos de avatar
se inspeccionaron en solo lectura, no se validó renderizado 3D ni compatibilidad
con juegos. Detalles en `QT_NETPLAY_INTEGRATION.md` y `AVATAR_SYSTEM.md`.

## Reparación del arranque/perfil y consultas — 0.15, 2026-09-20

- `local/profile-room-check.txt`: creación, selección, guardado y relectura real
  del perfil mediante Qt/QProcess, consulta HTTP con cambios pendientes y consulta
  HTTPS del servidor público; 5 resultados aprobados contando inicio/cierre. El
  servidor devolvió cuatro sesiones, no una lista generada por Emulos360.
- `local/gears-launch-check.txt` y `local/gears-render-check.txt`: perfil temporal,
  habilitación Netplay con copia, XUID local conservado y carga del GOD de Gears
  of War mediante PlayerServices/WindowContainer. Cierre normal. No se escribieron
  partidas del usuario ni archivos del juego en D:.
- Captura GPU original revisada: `local/gears-gpu-complete.png` muestra «Press
  Start». La captura Qt del padre era blanca porque no recoge la superficie GPU
  del hijo extranjero; no se usó como prueba del renderizado. La captura del
  motor se copió de nuevo al terminar su escritura y se comprobó visualmente.
- El helper final de captura espera que el PNG se pueda decodificar, evitando
  aceptar un archivo recién creado pero aún vacío.
- Compilación Release final de Qt y núcleo terminada; registros
  `local/launch-package-ui-build.log` y `local/avatar-manifest-core-build.log`.
- La ejecución final de `originalAvatarManifest profileConfigurationRoundTrip`
  fue bloqueada antes de arrancar: la revisión automática de permisos falló por
  límite de uso. **No se considera superada ni ejecutada**. Incluía el nuevo
  lector del cuerpo y la activación automática/idempotente del perfil; el flujo
  manual equivalente de conversión sí se comprobó antes.
- No se acredita aún renderizado de avatares originales, campaña completa,
  audio correcto durante una partida ni multijugador entre dos equipos.

Paquete actualizado en `out/Emulos360/`, conservando `data` y las dependencias
Qt ya desplegadas. SHA-256:

| Archivo | SHA-256 |
| --- | --- |
| Emulos360.exe | 3119F45FDB435EC75BA80F08F8B286D064F0D87A51D417C2F024166CDE898CFA |
| engine/Emulos360-core.exe | E6C9258F93DAEB71D03415C9443E91560E730CC3E645E17D29BAF0C323E77D35 |
| patches/xenia-netplay/0001-emulos360.patch | B3B5DDCA4F549AA8031BCE31B21E9EFF143782FA0976D907D164DDEB1F326111 |

Parche completo actualizado, incluido el nuevo lector de manifiesto; comprobación
no ejecutable `git apply --reverse --check` correcta sobre el árbol actual.
## Editor original 3D — 0.16, 2026-09-21

- Compilación Release de Qt Quick3D y núcleo completada. Recursos privados
  importados con SHA-256; DLL Qt y módulos QML desplegados por windeployqt.
- `local/avatar-assets-results.txt`: catálogo v2 con 1103 recursos, 589 modelos,
  349056 vértices, 396666 triángulos, 1619 texturas y 60 animaciones decodificadas;
  ensamblaje masculino/femenino de seis componentes. La comprobación previa del
  catálogo v1 cubrió 44 recursos, tres modelos, 3017 vértices y 5564 triángulos.
- `originalAvatarProfileRoundTrip`: dos perfiles temporales, animación observable
  en vertexData, cuerpo femenino, pelo/color, escritura, reapertura y segunda
  escritura con manifiesto idéntico de 1000 bytes y jugador activo conservado.
  **Superado: 3 resultados, cero fallos** (incluidos inicio/cierre de QtTest).
  Archivo `local/avatar-profile-results.txt`. No se tocaron perfiles de usuario.
- La comprobación detectó que un perfil nuevo aún no tiene dashboard GPD; el
  puente ahora lo inicializa sin login. Un GPD existente inválido se rechaza.
- Captura de la aplicación empaquetada, con dependencias locales, revisada:
  `local/avatar-package-final.png`. Geometría/texturas originales visibles en
  D3D11; no se sustituye por un dibujo 2D. El entorno restringido avisó que no
  podía escribir la caché de pipelines de AppData; la captura terminó con éxito.
- No se ejecutaron suites generales de upstream ni campañas de juegos. Estos
  resultados **no acreditan avatares dentro de títulos**: el servicio XAM de
  recursos y animaciones permanece pendiente.
- Backup HDD abierto en lectura: tres Account identificados por el núcleo,
  ninguno coincide con el gamertag solicitado; dos paquetes de guardado asociados
  a otro perfil. No se importaron esos perfiles ni partidas a la build. Informe
  privado `local/hdd-profile-identification/inventory.json`.
- Parche completo del fork regenerado incluyendo los cuatro headers de avatares;
  `git apply --reverse --check` superado. Hashes finales en el LEEME de la build.

## Lectura de la tienda — 2026-09-28

- Reproducido contra la web real: respuesta HTTP 200 con títulos, pero sin el
  enlace oculto a `/vault/999999` que el lector exigía para aceptar cada fila.
  `build/store-probe/live-before.txt` registra el fallo anterior.
- El lector reconoce ahora la tabla del catálogo y delimita su contenido
  contando las tablas anidadas de la cabecera. Admite filas actuales y antiguas,
  cambios en el orden de las clases y etiquetas de contenido `span` y `b`.
  Los enlaces de otras tablas y los marcadores ocultos quedan fuera del listado.
- `vimmCatalogParser`, `vimmLetterParser`, `vimmCurrentCatalogParser`,
  `rapidStoreLetterSwitches` y `liveVimmCatalog`: siete resultados aprobados,
  cero fallos, incluidos inicio/cierre de QtTest. Evidencia en
  `build/store-probe/store-after.txt`.
- Consulta real: siete resultados para Banjo digital; 61 títulos en A de
  Xbox 360, 102 en B y 457 en A digital, incluidas sus páginas adicionales.
  Estos números corresponden al catálogo consultado ese día.
- Paquetes Windows y Linux recompilados y actualizados. Captura Linux revisada
  en `build/store-probe/linux-store.png`, con los 61 títulos de A en el modelo
  y las primeras filas en pantalla. Se usaron datos temporales.
- SHA-256 del ejecutable Windows empaquetado:
  `E4BA0CFE37CB6D8CACC29B7375023147B04F44AFA3983C0E255264DABBE95BD5`.

## Edición MultiP privada — 2026-09-28

- Build Release Windows en `out/Emulos360-MultiP`, con Qt, Node 24.15.0,
  MongoDB 8.0.32 y Xenia-WebServices del commit
  `6abcc4397e0b6e0632307d9418ecae086cc95f48`. Build Linux independiente en
  `out/Emulos360-MultiP/linux`, compilada y ejecutada en Ubuntu 26.04 mediante
  WSL. El ejecutable de `out/Emulos360` conserva el SHA-256 anterior.
- Pruebas Node con el backend real: cuatro casos aprobados en Windows y Linux.
  Cubren autenticación, rechazo de certificado incorrecto, roles, invitaciones,
  revocación, pasarela local, perfiles, salas, puertos por título y almacenamiento
  QoS en el directorio privado. Registros en
  `.tools/multip-server/windows-server-final.txt` y
  `.tools/multip-server/linux-server-tests.txt`.
- Qt Windows: cinco resultados aprobados, cero fallos, incluidos inicio y
  cierre. Ciclo anfitrión/cliente, renderizado del panel con servidor e invitación
  reales y regresión del lector de tienda. Registro
  `.tools/multip-server/windows-qt-report.txt`. El test de interfaz avisa del
  icono omitido en el ejecutable de pruebas; el recurso sí está en la aplicación.
- Qt Linux: tres resultados aprobados del ciclo anfitrión/cliente con
  AddressSanitizer y UndefinedBehaviorSanitizer, sin diagnósticos de los
  sanitizers. Registro `.tools/multip-server/linux-native-tests.txt`.
- Capturas de los paneles Windows y Linux revisadas visualmente. La build utiliza
  ajustes, perfiles y base de datos propios; las pruebas usan datos temporales.
- Instalador Ubuntu ejecutado: instaló los paquetes gráficos que faltaban. Su
  comprobación final de Qt, bibliotecas del frontend/núcleo, Node y MongoDB
  terminó correctamente; registro `.tools/multip-server/linux-dependency-check.txt`.
- SHA-256 del frontend Windows MultiP:
  `BC85DA88B74CD3514B2CABEE286B1150E0917B3C73E44E1CB7880AFB24AB8F21`.
  SHA-256 del frontend Linux MultiP:
  `6EDA2458D9E81C08652C037EC6C4D2CBF5C2243177F40F5EB3943519D0ACFCF5`.
- Pendiente una partida completa entre dos equipos físicos. No se ha probado
  UPnP en un router real. Las pruebas del servicio no acreditan compatibilidad
  de todos los juegos ni una mejora del ping del tráfico de partida; no se ha
  implementado una retransmisión universal de ese tráfico.

## Reparación Linux — 2026-09-29

- Frontend y motor nativo Release recompilados en Ubuntu 26.04 mediante WSL,
  Qt 6.10.2 y Vulkan Mesa/llvmpipe. Paquete actual independiente:
  `out/Emulos360-MultiP-linux`; no se cambió `out/Emulos360` de Windows.
- `tests/linux-runtime`: nueve resultados aprobados, cero fallos u omisiones,
  con AddressSanitizer y UndefinedBehaviorSanitizer en el frontend de prueba.
  Cubren cierre con motor activo, formato binario XConfig, errores de sockets,
  importación 7z con herramienta sin permisos, ajustes copiados de Windows,
  catálogo de avatares empaquetado y arranque de un título real con perfil temporal.
  Registro `.tools/linux-repair/runtime-tests.txt`.
- After Burner Climax XBLA arranca con el motor recién recompilado, crea la
  superficie Vulkan, ejecuta hilos del título y muestra gráficos durante la
  introducción. Cierre normal mediante WM_DELETE_WINDOW; captura revisada en
  `.tools/linux-repair/game-linux.png`. El editor muestra el avatar original:
  `.tools/linux-repair/avatar-linux.png`. No se tocaron los juegos originales.
- El instalador base se ejecutó en Ubuntu 26.04; 7-Zip e ISO2GOD se abren.
  La actualización se comprobó con perfiles, partidas y ajustes temporales;
  sus SHA-256 se conservaron. No se incluyeron cuentas personales en el paquete.
- La prueba mediante WSL usa Vulkan por software. No acredita rendimiento ni
  resuelve por sí sola el bloqueo observado con la GPU de la instalación Ubuntu
  del usuario. SIGKILL (9) identifica el cierre forzado, no la causa anterior.
  El paquete incluye `diagnose-linux.sh` y captura adicional de stderr para ese
  diagnóstico cuando se vuelva a probar en el equipo.
- El ejecutable Windows habitual conserva el SHA-256
  `BC85DA88B74CD3514B2CABEE286B1150E0917B3C73E44E1CB7880AFB24AB8F21`.
- Archivo Linux de 142 MiB extraído en Ubuntu: no contiene `data`, conserva
  permisos 0755 y su instalador supera `--check`. Se quitaron los permisos a
  frontend, motor, 7z, ISO2GOD, Node y MongoDB: `--check` detectó el fallo; el
  instalador los reparó y la comprobación como usuario normal volvió a pasar.
  Registros `.tools/linux-repair/extracted-final-check.log` y
  `.tools/linux-repair/extracted-repair.log`. El script de diagnóstico generó
  correctamente su informe en esa instalación temporal.
- SHA-256 de `out/Emulos360-MultiP-linux.tar.gz`:
  `332747AA1FC32175B8B0FEE547F0F46D8044329FF2D5960E4C5E33C4F8068D67`.
  Parche completo del motor actualizado y validado con
  `git apply --reverse --check`, sin modificar el árbol durante la comprobación.

## Indicadores de compatibilidad — 2026-09-29

- Catálogo público Canary de Xenia Manager consultado ese día: 1104 registros,
  de los que 1102 tienen identificadores válidos no nulos. Snapshot incluido en
  recursos Qt y caché local validada; actualización diaria y botón manual.
- Biblioteca: búsqueda por Title ID. Tienda: coincidencias de nombres exactos
  normalizados, sin mezclar demos/betas o nombres ambiguos. Informes de una misma
  edición duplicados se resuelven por fecha; los cinco estados conservan el
  significado de la fuente. SVG propios, tooltip y leyenda en Ajustes → Emulos360.
- `tests/compatibility` en Ubuntu 26.04 con ASan/UBSan: siete resultados
  aprobados, cero fallos u omisiones, sin advertencias. Cubre snapshot, identidad,
  ambigüedad, caché corrupta, descarga coalescida, límite de tamaño, persistencia
  y conservación de estados tras un error. Registro
  `.tools/compatibility/linux-tests.txt`.
- Qt Windows: cinco resultados aprobados, cero fallos u omisiones. Cinco iconos
  en la biblioteca, leyenda a dos tamaños, tienda real (61 títulos de A) y cambio
  rápido de letras. Capturas revisadas en `.tools/compatibility/{store,library,legend}.png`.
  Registro `.tools/compatibility/windows-ui-tests.txt`.
- Linux Release ejecutada con datos temporales sobre la biblioteca real (80
  títulos). Se detectó el plugin SVG ausente, se añadió `qt6-svg-plugins` al
  instalador y se repitió la captura: `.tools/compatibility/linux-library.png`,
  sin errores de decodificación de iconos. El chequeo de dependencias pasa.
- Frontend Windows actualizado en `out/Emulos360` y `out/Emulos360-MultiP`;
  todos los archivos de `data` de la instalación habitual conservan sus SHA-256.
  Backup del ejecutable en `.tools/compatibility/Emulos360-before-compatibility.exe`.
  Frontend Linux actualizado en su carpeta independiente; el motor conserva
  el SHA-256 anterior `14286C6D2DA37AC70E2D5F3BBFD7C98B70D953DA6C06DD69C5A40DF8AFCC3C9A`.
- SHA-256 frontend Windows:
  `DA0F8B17F0D5F6F533DE0963AFE90273315F5E6BF9D3B09277EF1676647C0225`.
  SHA-256 frontend Linux:
  `AC834596507E03367C25CF90DB1F792BA6731BA440D9033BBC4092E4B2CF234F`.
- Los estados son referencias de Xenia Canary y no pruebas de una campaña con
  cada juego en Emulos360 ni garantías para una GPU, sistema o configuración.
- Paquete Linux final comprobado: incluye el plugin SVG, conserva permisos 0755
  en los seis ejecutables y coincide con los binarios publicados; no contiene
  datos personales. SHA-256 de `out/Emulos360-MultiP-linux.tar.gz`:
  `1A57365314E0A854B10C6F0C01DDF0053C820CC79B3821C16B3A4661F3E13FB0`.
