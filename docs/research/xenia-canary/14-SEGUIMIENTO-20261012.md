# 14 — Seguimiento selectivo del 12 de octubre de 2026

## Revisiones y alcance

Consulta del seguimiento autorizado del único core Xenia Canary Netplay.
Se leyeron AGENTS.md, prioridades, dossier, capítulos 02/03/04, seguimiento
previo y GAME_COMPATIBILITY.md antes de seleccionar cambios. Rivals continúa
aplazado; no se investiga otro core ni XBLIG. El checkout está en `main`, con
trabajo local de controles y cambios propios del submódulo: se conservan.

| Repositorio / rama | Seguimiento previo | HEAD consultado | Diferencia |
| --- | --- | --- | --- |
| [Canary / canary_experimental](https://github.com/xenia-canary/xenia-canary/tree/2372a54105d9d598ab5bc7ba94a72ef1bea4b271) | `cc4981a4659086acb1c25a485adebefc3f432ae1` | `2372a54105d9d598ab5bc7ba94a72ef1bea4b271`, 11-10 19:24:09 UTC | 13 commits posteriores, sin divergencia en esta comparación |
| [Netplay / netplay_canary_experimental](https://github.com/AdrianCassar/xenia-canary/tree/1892233f02b5758972ccd6ccb978dbde107f32ba) | `6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c` | `1892233f02b5758972ccd6ccb978dbde107f32ba`, 11-10 23:06:08 UTC | 15 commits posteriores, sin divergencia en esta comparación |

La base local permanece en `6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c`.
Se inspeccionaron completos los diffs de seis commits de cada repositorio,
además de la lista de los 28 commits y sus archivos. No se auditó íntegramente
cada cambio de esas ramas. No hay un revert identificado por mensaje en esos
28 commits; los tres cambios Netplay y los dos Canary preparados se contrastaron
con el código del HEAD consultado y siguen presentes. Esto no sustituye un
análisis exhaustivo de equivalencias o regresiones.

**Estado de esta ejecución:** cinco parches oficiales adicionales conservados,
con atribución y tests upstream cuando están incluidos. Todos pasan la
comprobación individual `git apply --check` sobre el árbol local actual;
la comprobación inversa falla. Los hashes de las cinco fuentes vigiladas
coinciden antes/después. No se aplicaron parches, no se compiló, no se ejecutó
el core, ni hubo pruebas funcionales, de GPU, gameplay o Netplay. No se repiten
las compilaciones y pruebas CPU/Vulkan históricas.

[Evidencia con hashes y salidas](core-seguimiento-evidencia-20261012.json).
Los parches son material para una copia de prueba, no están conectados a
scripts de producto ni incluidos en `0001-emulos360.patch`.

## Cinco candidatos preparados

| Parche | Revisión y contrato | Estado / límite |
| --- | --- | --- |
| [netplay-bind-null-upnp.patch](../../../patches/xenia-upstream-candidates/netplay-bind-null-upnp.patch) | [c7f5f15e1232](https://github.com/AdrianCassar/xenia-canary/commit/c7f5f15e1232a5f1aac77c40e2a171e12faec3f8): evita usar UPnP antes de comprobar el puntero | Contexto válido; falta probar bind sin UPnP, puertos automáticos y mapeados |
| [netplay-fd-set-bounds.patch](../../../patches/xenia-upstream-candidates/netplay-fd-set-bounds.patch) | [493d2d32ddb0](https://github.com/AdrianCassar/xenia-canary/commit/493d2d32ddb0d4b3d7073b1e4cc98da49305e28c): valida puntero y contador antes de recorrer fd_array | Contexto válido; revisar también el extremo permitido y consumidores |
| [netplay-qos-copy-size.patch](../../../patches/xenia-upstream-candidates/netplay-qos-copy-size.patch) | [4a904011fb1a](https://github.com/AdrianCassar/xenia-canary/commit/4a904011fb1a18f30a2bb77e3477b3510e35d063): copia la longitud usada para reservar y comprueba la asignación | Contexto válido; la conversión de tamaño a uint16 sigue truncando valores grandes |
| [posix-created-suspended-resume.patch](../../../patches/xenia-upstream-candidates/posix-created-suspended-resume.patch) | [1c81b8674738](https://github.com/xenia-canary/xenia-canary/commit/1c81b867473806b2845a1946b2d185c9d888a319): publicación del estado suspendido y espera por semáforo; implementa Query de evento | Contexto válido, incluye dos tests; afecta POSIX, no corrige el scheduler Windows |
| [utf8-upper-ascii.patch](../../../patches/xenia-upstream-candidates/utf8-upper-ascii.patch) | [19c23eb1123f](https://github.com/xenia-canary/xenia-canary/commit/19c23eb1123f891d44f6548429758b612fe0ae7e): convierte minúsculas ASCII a mayúsculas | Contexto válido, incluye tests; sin beneficio de juego concreto demostrado |

### Bind y listas de sockets

La fuente local desreferencia `upnp` antes de su `if (upnp)`. El parche parte
del puerto real obtenido del socket y solo consulta el mapeo si existe UPnP.
No se ha reproducido aquí un bind con objeto nulo, ni se atribuye un cierre
pasado de un juego a esta ruta. Validar UDP/TCP, puerto cero, puerto explícito,
UPnP ausente/inactivo/activo y error de bind, conservando selección de interfaz
y los cambios privados de traducción de errores POSIX.

El contador guest de `verify_x_fd_set` controla un array de capacidad fija.
El guard añade rechazo previo del puntero nulo y de un contador excesivo.
**Límite pendiente:** el guard permite igualdad con `X_FD_SETSIZE`, pero
`host_set::Load` todavía contiene `assert_true(fd_count < X_FD_SETSIZE)`.
Comprobar el máximo en Debug y Release y la validez del descriptor host,
además de conjuntos vacíos, punteros omitidos y handles inválidos. No presentar
este hunk como validación completa de todos los buffers de sockets.

### QoS: longitud y asignación

La base reserva una longitud convertida a `uint16_t` y copia `chunk.size`
completo. El parche reutiliza la longitud reducida para la copia y no escribe
si la reserva falla. Reduce esa discrepancia, pero **no decide cómo admitir
un payload de más de 65535 bytes**: la conversión estrecha sigue ahí. Probar
0, 1, 65535, 65536 y 65537 bytes y reserva fallida, flags/data_len/data_ptr,
cancelación y liberación. El tratamiento correcto de una respuesta excesiva
necesita verificar el contrato; no inventar un truncado válido para todos los
juegos. Para gameplay, comparar descubrimiento/admisión y partida en títulos
ya funcionales con QoS, sin cambiar el backend ni sus datos.

### POSIX: Resume perdido

La base publica estado y notifica el arranque antes de establecer el contador
de suspensión. `Resume` puede observar cero en ese intervalo. El cambio
establece el contador bajo el mismo mutex y usa `WaitSuspended`, cuyo semáforo
ya es notificado por el Resume local. No requiere portar el cambio amplio de
polling/futex para que el diff encaje. Incluye Query de evento y dos pruebas
nuevas de creación suspendida/reanudación y suspensión posterior.

El autor reporta Lost Odyssey detenido en carga en 5/10 intentos Linux y 0/10
con el cambio. **Es evidencia upstream, no gameplay probado en Emulos360.**
Pendientes compilación nativa Linux, pruebas de hilo/evento bajo estrés,
ASan/UBSan y regresiones de ejecución/cierre y Netplay. Una compilación Windows
no prueba el código POSIX. Este parche no reabre la investigación de Rivals.

### ASCII

Se confirma en la implementación local que `upper_ascii(uint32_t)` aplica la
misma transformación que `lower_ascii`; el hunk corrige ese contrato. El
parche conserva pruebas para texto ASCII y caracteres no ASCII, que deben
mantenerse sin una conversión Unicode general. La búsqueda en .cc/.h solo
encuentra la implementación, wrapper y declaración; no se ha identificado un
consumidor de juego que explique un fallo. Prioridad inferior a los defectos
de memoria/sincronización. Los tests descargados no se han ejecutado.

## Otras novedades inspeccionadas, sin preparar ni integrar

- [6435e79904e1, espera multiobjeto Linux](https://github.com/xenia-canary/xenia-canary/commit/6435e79904e1b7c3b34651d92bd017036aa4ad62): reemplaza polling por contador/epoch y futex, con fallback POSIX y límite de espera. El autor mide menos ciclos CPU en una escena de Lost Odyssey a los mismos FPS. No extrapolar ese resultado a nuestro equipo ni a Windows. Requiere comprobar señales, autoreset, wait-any/all, timeouts y callbacks en signal handlers; solo inspección estática.
- [f41ee57bc3af, mutex de propiedades](https://github.com/xenia-canary/xenia-canary/commit/f41ee57bc3af652d9b9163fbea0c45cd01a9d07f): protege accesos a un vector y cita Poker Smash. La base Netplay tiene más lógica de perfiles; revisar todos los lectores/escritores. `GetProperty` devuelve un puntero a un elemento: el lock se libera al retornar, por lo que falta auditar la vida del puntero ante reallocación posterior. No presumir que añadir el mutex elimina toda carrera.
- [a7ca21e0caf1, mantenimiento al terminar título](https://github.com/AdrianCassar/xenia-canary/commit/a7ca21e0caf193ef34c173823d9cda3588dfc006): llama StopPeriodicMaintenance antes de la región crítica global. La operación existe localmente; revisar cancelación/destrucción de callback y sucesivos arranques, sin borrar perfiles o guardados. Solo inspección estática.
- [ee0d5d54c468, XStorageBuildServerPath](https://github.com/AdrianCassar/xenia-canary/commit/ee0d5d54c46831dcfa992a815d1049b55471fb97): comprueba sesión/perfil antes de obtener XUID. Probar perfil ausente, invitado y cambios de sesión antes de aceptar el nuevo orden de errores.
- [7609feaff3b3, XStorageEnumerate](https://github.com/AdrianCassar/xenia-canary/commit/7609feaff3b31cc44a2c70350e667ca6b02e055b): evita restar el header a un buffer menor. Falta auditar Deserialize/ZeroResults y punteros/tamaños antes de cualquier escritura, además de tamaños de entrada grandes. Las pruebas futuras deben usar datos temporales, conservando partidas reales.
- [2372a54105d9, redondeo XMA](https://github.com/xenia-canary/xenia-canary/commit/2372a54105d9d598ab5bc7ba94a72ef1bea4b271): sustituye truncado de muestras por lrintf después de clamp. Requiere comparar PCM y modo de redondeo; no acredita resolver cortes ni deriva de audio.
- [04f85416e0f8, tipo guest en objetos](https://github.com/xenia-canary/xenia-canary/commit/04f85416e0f8ab59808e2275d6e6b05630c3c044): conserva tipo de objeto invitado y cambia la comprobación de handles; cita imágenes Kinect de 5345084D. Afecta SaveObject/RestoreObject: auditar compatibilidad del formato y consumidores antes de portar. No se prepara automáticamente por afectar a un caso particular.

Los otros 16 commits quedaron **identificados por metadata**, pendientes de
inspección completa si su contrato resulta necesario. Incluyen otros cambios
XAM/NUI, herramientas, notificaciones, XHTTP, sockets, búsquedas/presencia,
XStorage y UPnP. No se mezclan automáticamente los nuevos stubs HTTP o cambios
UPnP con nuestras APIs/sockets: hay modificaciones privadas en esas rutas.

## Gráficos y aceptación

Los 13 commits nuevos de Canary no incluyen un diff nuevo de GPU en la
comparación consultada. La corrección de posiciones MSAA relacionada con
Gears 1 ya estaba documentada el 7 de octubre; permanece pendiente, sin volver
a compilarla ni presentarla como una novedad de esta semana. Los candidatos
anteriores CPU/Vulkan/viewport/fetch mantienen sus estados y límites.

Para integrar cualquiera de los nuevos candidatos: preparar una copia completa
que preserve los cambios propios, aplicar una unidad coherente, ejecutar las
pruebas de contrato indicadas y comprobar regresiones en títulos y Netplay que
ya funcionan (por ejemplo Gears 3, Borderlands 2 y The Pre-Sequel). Registrar
plataforma, juego/TU, configuración, reproducción y logs; distinguir prueba
sintética, inspección de contexto y partida real. No sustituir el motor estable
hasta contar con evidencia suficiente. No hay publicación ni commit/push en
este seguimiento.
