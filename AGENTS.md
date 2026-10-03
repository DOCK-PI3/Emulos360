# Referencia técnica del proyecto

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
aplazados en el core actual. El 3 de octubre el usuario autorizó incorporar
Edge como segundo core opcional e independiente para la versión 0.4.0; consultar
[la integración](docs/DUAL_CORE.md). No sustituir el core actual ni sus datos. Los parches de `patches/xenia-upstream-candidates/` son candidatos
preparados y no están integrados al motor activo. Una comprobación de contexto
o compilación aislada no autoriza a afirmar compatibilidad de gameplay.

Para la entrega 0.4.0, el usuario realizará las pruebas en su instalación,
el commit, el push y la publicación de la release. Dejar los cambios y paquetes
preparados para revisión; no publicar por cuenta propia.
