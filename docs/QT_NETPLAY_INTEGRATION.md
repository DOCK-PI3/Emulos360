# Ventana del motor y salas — Emulos360

Implementación propia, separada de la red de Xenia Canary Netplay.

## Ventana nativa

`app/core_window_host.*` localiza exclusivamente la ventana `XeniaWindowClass`
del PID devuelto por el QProcess que acaba de iniciar PlayerServices. Qt 6.8
`WindowContainer` aloja el QWindow extranjero; el núcleo conserva el renderizador
D3D12/Vulkan, su proceso y sus menús. No hay navegador web ni copia de fotogramas.

`GameSession.qml` ofrece volver a biblioteca, recuperar el control del juego,
consultar salas y solicitar cierre normal después de confirmar que se ha guardado.
La biblioteca y las salas no pausan el juego. La ventana nativa se oculta cuando
aparece la confirmación Qt para evitar que cubra el diálogo. El mando deja de
traducirse a teclas de navegación Qt mientras existe una sesión, evitando enviar
los mismos botones al dashboard y al juego.

El flag **transitorio** `emulos_embedded` impide que el F11/doble clic del núcleo
deshaga el contenedor mediante pantalla completa independiente. La pantalla
completa la controla la barra de Qt. No cambia los ajustes persistentes.

La integración se implementa para Windows. En otras plataformas se conserva el
lanzamiento externo. Si no aparece una ventana integrable en 60 segundos, se
informa del problema y se sigue buscando cada segundo; no se mata el proceso ni
se libera el bloqueo de partidas.
Un aviso inicial del propio núcleo puede necesitar atención antes de integrarse.
El QWindow es un wrapper: la destrucción real de la ventana pertenece al núcleo.

## Navegador de salas

`app/netplay_rooms.*` consulta mediante GET `<Live.api_address>/sessions`.
`RoomsPanel.qml` aparece en Comunidad y durante una sesión. Consulta al abrirse,
sin exigir guardar ajustes ajenos a esa operación de solo lectura. Hay actualización
manual, cancelación, filtro por título/anfitrión y filtro de juegos instalados.
Se muestran título, anfitrión, jugadores/plazas, presencia, Media ID y versión.
Los textos remotos se presentan como texto plano; no se cargan imágenes ni HTML
procedentes del listado. Hay límite de 8 MB, 1000 sesiones y timeout de 20 segundos.
Cambiar de servidor o cancelar elimina los resultados anteriores.

El esquema se contrastó con `AggregateSessionCommandHandler` del repositorio
[Xenia-WebServices](https://github.com/AdrianCassar/Xenia-WebServices).
El endpoint agrupa las sesiones anunciadas por título y puede deduplicarlas por
anfitrión. **No expone un identificador de sesión ni una operación de unión que
pueda sustituir al juego**. Por eso el botón se llama «Abrir juego»: requiere
GOD completo, modo Netplay guardado, perfil y ausencia de otra sesión.
La creación y la unión se realizan en el menú multijugador del juego. No se
marca una conexión como exitosa por haber abierto el ejecutable.

XBGuard no es necesario para Netplay. XBGuard y su cargador DashLaunch son una
integración distinta que sigue sin estar implementada.

## Comprobaciones limitadas a estos cambios

`EMULOS_NETPLAY_TESTS=ON` añade únicamente `netplay_integration_tests`:

- Análisis de respuesta, normalización de Title ID, rechazo de JSON/esquema
  inválido y límite de tamaño.
- Consulta HTTP a un servidor local, ruta correcta y cancelación.
- Con `EMULOS_TEST_CORE` apuntando al núcleo: QProcess real, ventana dentro de
  WindowContainer, WS_CHILD/padre, redimensionado, ocultación/visibilidad y cierre
  normal. Usa una carpeta temporal, red desactivada y ningún juego.

Resultados: `local/netplay-integration-results.txt` (4 aprobados, contando
inicio/cierre), `local/native-window-final.txt` (3 aprobados). Captura Qt revisada
en `local/rooms-final.png`. Consulta pública de solo lectura el 2026-09-20:
HTTP 200, `Titles: []`. Cero salas en esa consulta no demuestra ni descarta la
compatibilidad online de un título. No se ha realizado una partida entre equipos.

Referencia Qt: [WindowContainer 6.8](https://doc.qt.io/qt-6.8/qml-qtquick-windowcontainer.html).

## Paquete histórico 0.14 (sustituido por 0.15)

En la entrega 0.14, `out/Emulos360/Emulos360.exe` y su subcarpeta `engine` se actualizaron después
de compilar. SHA-256:

- UI: `43F6D4674CC740399101216E82D98E76D3709646F30273CD62C134C9C5F8755E`
- Núcleo: `320761683810B5ABAFFAE129612FE1AC7EEE5BDF41A5A442BDF13991BD742CDB`
- Parche: `96344A6B9B7C7846D019EF41B488EA21F48B7D7056690B2C28088BECB137C9B0`

Identificación Netplay comprobada dentro del binario y parche verificado con
`git apply --reverse --check`. Los recursos originales del backup están fuera
del paquete, en la carpeta privada `/local/` indicada en `AVATAR_SYSTEM.md`.

## Corrección 0.15

El proceso auxiliar inicializaba XConfig, pero Qt conservaba el snapshot anterior:
el siguiente guardado fallaba por cambio externo. Ahora se recarga al finalizar
ese proceso, durante el cual los ajustes estaban bloqueados y no había cambios
pendientes. Se conservan los controles de conflicto con otras aplicaciones.

El IPC incluye el estado Netplay y permite habilitarlo mediante la conversión
upstream, conservando XUID local/partidas y haciendo copia del perfil/dashboard.
Un modo Netplay elegido se aplica al perfil activo descubierto al iniciar Qt.
Los fallos de arranque ya muestran un diálogo; los logs de juego y perfiles se
guardan separados en `data/core.log` y `data/profile-helper.log`.

La corrección se comprobó con el servidor HTTPS real (cuatro salas) y el arranque
de Gears of War hasta su pantalla inicial. Ver `VALIDATION.md`; una consulta de
salas no acredita por sí sola una partida multijugador completa.
