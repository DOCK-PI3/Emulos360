# Referencia técnica del proyecto

## Prioridad vigente del usuario — 5 de octubre de 2026

Retirar Nexia360 y Xenia Edge, por solicitud expresa del usuario. La retirada
está terminada y verificada en el proyecto. XBLIG queda descartado y no es backlog.
Consultar [el estado y las próximas prioridades](docs/NEXT_PRIORITIES.md) y
[el funcionamiento con un único core](docs/SINGLE_CORE.md).
No añadir otros cores. El único motor es Xenia Canary del fork Netplay.
Priorizar compatibilidad, estabilidad, rendimiento y correcciones gráficas,
conservando perfiles, partidas, juegos y Netplay que ya funcionan.
Los datos históricos bajo `data/cores` se conservan: no borrar ni mover vistas
con junctions/enlaces ni sus destinos. No desinstalar los frameworks XNA de Windows.

Para investigar o modificar el core de emulación, gráficos, rendimiento,
estabilidad o multijugador, consultar primero
[el dossier de Xenia Canary](docs/research/xenia-canary/README.md) y el capítulo
correspondiente al subsistema. Es una referencia importante solicitada por el usuario.

Para incorporar o elegir títulos multijugador, leer
[el método](docs/research/xenia-canary/06-METODO-PARA-NUEVOS-TITULOS.md) y
[la evaluación de la biblioteca](docs/research/xenia-canary/08-CANDIDATOS-BIBLIOTECA.md).
Revisar también [las pruebas locales previas](docs/GAME_COMPATIBILITY.md).

La investigación está fechada el 2 de octubre de 2026. Comprobar revisiones,
código y evidencias actuales antes de reutilizar sus conclusiones. Distinguir
reportes upstream, inspección estática, hipótesis y gameplay probado localmente.
Actualizar la documentación cuando nuevas pruebas cambien una conclusión.

Incorporar un título significa lograr multijugador funcional. Localizar primero
la capa que falla: aplicación/perfil, core XAM/XNet/sockets, backend de sesiones
o servicio específico del juego. Un Title ID registrado, una respuesta HTTP
correcta o un lobby visible no demuestran una partida funcional.

El icono naranja de compatibilidad describe «Solo arranca» en los informes de
Canary. No diagnostica una dependencia de Xbox Live ni el estado de Netplay.
La ausencia de un título en la wiki Netplay no demuestra incompatibilidad.

## Mantenimiento selectivo del core

Consultar también [el seguimiento](docs/research/xenia-canary/10-SEGUIMIENTO-Y-MEJORAS-DEL-CORE.md)
para las consultas posteriores al dossier inicial. El usuario prioriza conservar
los títulos y multijugador que ya funcionan; Rivals y el scheduler de Edge están
aplazados en el core actual. No sustituir el core actual ni sus datos. Los parches de `patches/xenia-upstream-candidates/` son candidatos
preparados y no están integrados al motor activo. Una comprobación de contexto
o compilación aislada no autoriza a afirmar compatibilidad de gameplay.

Para la entrega 0.5.0, el usuario realizará las pruebas en su instalación,
el commit, el push y la publicación de la release. Dejar los cambios y paquetes
preparados para revisión; no publicar por cuenta propia.

## Historial retirado

`docs/DUAL_CORE.md`, `docs/XBLIG.md` y los capítulos 11/12 son evidencia histórica,
no autorizan reintegrar proveedores. Los datos y assets privados se conservan
fuera de las releases; no borrar progreso al retirar un ejecutable.
Commit, push y publicación corresponden al usuario.
