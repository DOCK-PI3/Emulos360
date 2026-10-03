# 08 — Candidatos de la biblioteca y diagnóstico de Sacred 2

Consulta: **2 de octubre de 2026**. Alcance: inventario de metadatos e investigación;
no se iniciaron juegos, partidas, el motor de emulación ni el servidor. No se
modificaron perfiles, opciones de los juegos o código del producto.

## Recomendación

Para investigar una **incorporación de multijugador todavía no documentada en
la wiki Netplay**, el primer candidato es **Castlevania: Harmony of Despair
(`58410A7A`)**. **Sacred 2: Fallen Angel (`4B4D07D1`)** es el segundo candidato,
con evidencia particularmente útil para separar bloqueo de emulación y red.
Esta prioridad es una valoración de ingeniería previa a pruebas locales; no
significa que alguno necesite necesariamente código nuevo o que sea fácil.

Si el objetivo inmediato es obtener una primera partida en nuestro servidor
con soporte ya reportado, elegir **Gears of War 1** como control. No sería una
incorporación nueva al fork. Su recorrido conocido ayuda a detectar problemas
de configuración, perfil, servidor o NAT antes de atribuirlos al título nuevo.
[Matriz de Gears](05-GEARS-Y-CASOS.md).

## Qué biblioteca se revisó

Se escaneó `D:/XBOX360_GAMES` con el ejecutable empaquetado:

```powershell
out/Emulos360/Emulos360.exe --inventory --library D:/XBOX360_GAMES
```

`app/main.cpp` procesa `--inventory` y sale después de `scanLibrary`, antes de
crear el controlador y la interfaz. El ejecutable de `build/Release` no pudo
iniciarse por falta de dependencias; se usó el paquete de `out/Emulos360`, que
terminó correctamente con JSON válido. Solo se leyeron metadatos de biblioteca.

Resultado: **80 entradas**, con Title ID y formato, marcadas completas por el
scanner. Esa marca comprueba la estructura que reconoce el scanner; no es una
verificación criptográfica de todo el contenido ni una prueba de emulación.
No se presupone que las 80 estén visibles en la selección actual de la interfaz:
el alcance es la carpeta completa, incluidos subdirectorios.

Se cruzaron los Title IDs con `app/compatibility-canary.json` y la tabla Netplay:
**36 entradas** tienen una fila en la wiki, con distintos grados de soporte;
**44 no la tienen**. Estas últimas incluyen títulos sin multijugador: no deben
contarse como 44 juegos online incompatibles. El snapshot antiguo de
`local/current-library.json` tenía 13 juegos y ya no representa esta biblioteca.
Destiny y Black Ops III no fueron detectados en el escaneo actual; no se usaron
como candidatos actuales aunque existan informes/pruebas históricas.

El [inventario cruzado](biblioteca-netplay-20261002.json) conserva las 80 entradas,
estados, URLs de Canary, columnas de Netplay, fecha y SHA-256 de los insumos.
Omite rutas individuales a juegos y enlaces de distribución. La tabla Netplay
obtenida de nuevo coincide byte por byte con la consultada para el dossier.

## Comparación de candidatos

| Título presente | Evidencia de emulación | Evidencia Netplay | Prioridad y limitación |
| --- | --- | --- | --- |
| Castlevania: Harmony of Despair | Canary: Jugable; reporte de capítulos 1–6, con defectos menores | Sin fila en la tabla consultada; Xbox confirma coop online de 2–6 | Primera investigación nueva: permite concentrar el diagnóstico en multijugador; faltan prueba local y trazas de protocolo |
| Sacred 2: Fallen Angel | Canary: Solo arranca; reporte señala que Netplay alcanza gameplay | Sin fila en la tabla consultada; manual confirma host en consola | Segunda investigación: arquitectura favorable, pero carga y gráficos requieren separar causas |
| DAYTONA USA | Canary: Jugable; reporte indica todas las pistas/carreras sin problemas | Sin fila en la tabla consultada | Alternativa con buena base de emulación; falta investigar recorrido de red antes de compararlo por esfuerzo |
| Hydro Thunder Hurricane | Catálogo: Jugable; issue vivo etiqueta gameplay y describe tirones/defectos | Sin fila en la tabla consultada | Alternativa: revisar discrepancia catálogo/issue y oclusión antes de afirmar estabilidad |
| Sacred 3 | Sin datos en catálogo local | Sin fila en la tabla consultada | Menor prioridad: aún falta establecer una base de gameplay |
| Army of Two / BAJA | Canary: En juego | Sin fila en la tabla consultada | Reservas: investigar dependencia de servicios y reproducir estabilidad primero |
| Gears 1, 2 y 3 / Modern Warfare 2 / Left 4 Dead | Buena base según catálogo | Funcionamiento público reportado | Controles y validación de nuestra integración, antes que soporte nuevo |
| Gears Judgment / Diablo III UEE / Unreal Tournament 3 | Estados distintos según título | Probados localmente upstream | Validar alcance local/público; «probado localmente» no garantiza conexión por Internet |
| Modern Warfare 3 / Ghosts / Future Soldier | Gameplay o Jugable | Público marcado con fallo; System Link reportado | Recuperar Live público puede ser una tarea mayor; no confundir System Link con soporte Live |

Fuentes: [wiki Netplay](https://github.com/AdrianCassar/xenia-canary/wiki/Netplay-Compatibility),
[Castlevania #171](https://github.com/xenia-canary/game-compatibility/issues/171),
[Xbox: modos de Castlevania](https://www.xbox.com/en-us/games/store/Castlevania-Harmony-of-Despair/BZS1294V9FQ3),
[DAYTONA #639](https://github.com/xenia-canary/game-compatibility/issues/639),
[Hydro Thunder #738](https://github.com/xenia-canary/game-compatibility/issues/738).

La preferencia por Castlevania procede de su base de emulación y de disponer de
un modo cooperativo repetible; no se ha demostrado que sus servicios sean más
sencillos que los de Sacred 2. Ser 2D o pequeño no garantiza un protocolo simple.
La topología y posibles servicios específicos de Castlevania siguen pendientes.

## «Incorporar un título» y el servidor Netplay

En esta investigación significa **hacer funcionar su multijugador en
Emulos360/MultiP**. Puede requerir cambios en el servidor de sesiones, pero
también en perfiles, llamadas XAM, búsquedas/XLast, buffers guest, QoS, sockets,
operaciones asíncronas, invitaciones o servicios del editor.

La API de Xenia-WebServices permite descubrir y coordinar sesiones. El juego
ejecuta su protocolo de partida. El core emula las llamadas de plataforma que
conectan ambos recorridos. Añadir un Title ID a una tabla, aceptar una solicitud
o ver el lobby no equivale a completar ese recorrido. Algunos juegos pueden
funcionar ya con contratos genéricos sin añadir ninguna excepción por título.
[Arquitectura y código inspeccionado](04-NETPLAY-Y-SYSTEM-LINK.md).

La wiki incorpora títulos cuando al menos dos participantes conectan y entran
en gameplay. **Ausencia de fila significa ausencia de esa evidencia en la tabla**;
no demuestra que el backend los rechace o que el core no los soporte.
[Criterio de la wiki](https://github.com/AdrianCassar/xenia-canary/wiki/Netplay-Compatibility).

## Qué significa el naranja de Sacred 2

`GameCompatibility::describe`, en `app/game_compatibility.cpp`, transforma
`Loads` en **Solo arranca**, color `#f8c85c`: el juego arranca pero puede quedarse
en introducción, menús o carga. No contiene diagnóstico de Xbox Live ni usa
el resultado de una conexión Netplay para elegir el color.

Para Sacred 2, el catálogo local toma el issue #868, actualizado el 2 de agosto
de 2026, con `state-menus`, `gpu-drawing-corrupt`, `supports-1080p` y
`requires_protect_zero_false`. Es un informe de Canary, no una medición de nuestro
fork con este GOD. Por ello **el icono no prueba que se quede cargando por exigir
Xbox Live**. [Leyenda y resultados previos](../../GAME_COMPATIBILITY.md).

## Sacred 2: evidencias que cambian el diagnóstico

1. **Tiene juego sin conexión.** El manual permite comenzar para un jugador y
   después invitar a otros. No hay fundamento para tratar una conexión Live
   permanente como requisito de toda campaña.
2. **La partida la sirve el anfitrión.** El manual describe hasta cuatro
   participantes y la consola anfitriona como servidor del resto. Esto favorece
   investigar sesiones y transporte del core antes de diseñar un servidor
   dedicado que replique el mundo. No excluye servicios auxiliares pendientes.

[Manual español original de Xbox 360, alojado en un mirror, páginas impresas 9–15](https://device.report/m/6dc96e62bab6779d90c9517214153b29cd87b0e48560ffbd9c30e98c7d274a0f.pdf).
Se usa el documento del editor como fuente primaria; no instrucciones del mirror.

3. **El bloqueo difiere entre forks.** El issue #868 describe un bloqueo de
   título con protección de cero y un bucle al entrar en gameplay en Canary,
   y señala que ambos no aparecen en el fork Netplay. No identifica el hash
   exacto probado de Netplay, el modo de red usado o una partida entre dos PCs.
   Es evidencia para comparar el core; no prueba soporte multiplayer.
   [Reporte #868](https://github.com/xenia-canary/game-compatibility/issues/868).
4. **La actualización de agosto confirma esa diferencia.** El comentario del
   2 de agosto de 2026 sigue señalando que hace falta la build Netplay para
   alcanzar gameplay. También atribuye la corrección de modelos oscuros en
   D3D a `da47dfaacfae1134b238af9083a7f3d413c6cbbe`.
   [Comentario](https://github.com/xenia-canary/game-compatibility/issues/868#issuecomment-5159904424).
5. **Ese arreglo gráfico ya está en el código local inspeccionado.** El commit
   corrige el registro destino del sesgo de redondeo firmado en memexport DXBC.
   Las dos rutas locales de `dxbc_shader_translator_memexport.cc` escriben en
   `round_bias_temp`, en las líneas 198 y 425, como el arreglo upstream. No es
   una mejora pendiente que haya que volver a portar. Esto no demuestra que el
   binario instalado se compilara con ese archivo ni que resuelva los demás
   defectos de Sacred 2.
   [Diff del arreglo](https://github.com/xenia-canary/xenia-canary/commit/da47dfaacfae1134b238af9083a7f3d413c6cbbe).
6. **Existe evidencia histórica de funciones de plataforma ausentes.** El
   reporte de 2018 menciona `NetDll_WSAGetOverlappedResult`. La base local ya
   implementa esa entrada y delega en `XSocket`; no debe diagnosticarse como
   todavía ausente solo por el reporte antiguo. Faltaría validar su contrato
   para las llamadas de este juego.
   [Reporte histórico #967](https://github.com/xenia-project/game-compatibility/issues/967).

El documento preliminar de Ascaron de julio de 2008 también describe el host
en consola, pero es anterior al lanzamiento y contiene funciones planeadas.
No se usa para garantizar protocolos del producto final.
[Factsheet de Ascaron](https://sacred-tribute.com/webarchive/GC2008/content/Game%20Information/Factsheet_Xbox360.pdf).
Un emulador de lobby para Sacred 2 PC no demuestra compatibilidad con su versión
Xbox 360. Tampoco se ha confirmado aquí una ruta System Link de Sacred 2.

## Próxima investigación ejecutable, sin inventar la causa

Primero registrar build/binario real, Title ID, Media ID, región, versión/TU,
GPU y configuración efectiva. Conservar perfiles y guardados. Mantener iguales
las revisiones entre anfitrión y cliente. La biblioteca solo confirmó Title ID
y contenedor, no la combinación Media ID/TU que se necesita para conectarlos.

Para Castlevania, alcanzar un capítulo offline; después crear/buscar/unir una
partida de dos participantes con el mismo contenido base. Registrar llamadas,
XLast/query IDs, HTTP y tráfico de sockets hasta el primer resultado incorrecto.
El issue cita actualización 4.0.2 para DLC: no traducir ese número automáticamente
a un índice TU sin comprobar metadata. Comenzar sin depender de DLC adicionales.

Para Sacred 2, comparar tres casos en el mismo core: campaña para un jugador
en offline, modo Live emulado sin compañero y sesión host/cliente. Localizar si
la carga falla antes de publicar sesión, durante una llamada de perfil/estadísticas,
al devolver una operación asíncrona o en el transporte. La existencia de un
socket inicializado no demuestra una dependencia externa obligatoria.

Solo después decidir el cambio: configuración específica si basta, contrato del
core si devuelve datos/estado incorrectos, backend si filtra mal o falta un
endpoint, o ingeniería de un servicio particular si se observa su protocolo.
Evitar respuestas ficticias de éxito que hacen avanzar un menú sin implementar
lo que el juego utilizará después. [Procedimiento completo](06-METODO-PARA-NUEVOS-TITULOS.md).

Criterio mínimo: dos participantes en gameplay con estado compartido durante una
sesión sostenida, salida/unión reproducible y guardados conservados. Comprobar
Internet/NAT por separado del ensayo LAN y no anunciar cuatro/seis jugadores
hasta probarlos. No marcar naranja/verde de Netplay alterando el estado Canary
como sustituto de estas pruebas.

## Evidencia conservada

El inventario publicable y sus hashes están en
[biblioteca-netplay-20261002.json](biblioteca-netplay-20261002.json).
Las consultas adicionales a GitHub se registran en
[fuentes-candidatos-20261002.json](fuentes-candidatos-20261002.json).
Las seis respuestas API/wiki se guardan en
`local/library-candidate-sources-20261002/`; la tabla y los issues pueden volver
a consultarse por sus enlaces. El manual, la ficha histórica y la tienda Xbox
se consultaron con la herramienta web, sin atribuirles un hash de descarga.
