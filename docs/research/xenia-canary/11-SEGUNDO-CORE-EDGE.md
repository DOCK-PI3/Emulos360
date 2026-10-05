# 11 — Segundo core opcional: Canary Edge

> **Historial de una integración retirada el 5 de octubre de 2026.**
> No es una función actual ni una prioridad futura. Véase
> [el funcionamiento con un único core](../../SINGLE_CORE.md).


Fecha: **3 de octubre de 2026**. El usuario autorizó integrar Edge como segunda
opción en el proyecto GitHub, manteniendo el core actual. La implementación,
revisión fijada, contratos auditados, almacenamiento y pruebas están en
[la documentación de integración](../../DUAL_CORE.md).

Este trabajo no porta el scheduler de Edge al motor Netplay ni incorpora los
parches candidatos del capítulo 10. El binario del core actual se conserva.
El dossier del 2 de octubre sigue siendo el registro histórico; sus conclusiones
sobre un cambio de scheduler general no equivalen a prohibir un core independiente.

La evidencia actual verifica compilación, selección, lanzamiento, aislamiento y
cierre de ventanas. No amplía automáticamente la compatibilidad de juegos ni
resuelve Rivals. Para incorporar multijugador sigue aplicándose el método del
capítulo 06: una ventana abierta, un lobby o un Title ID no demuestran una partida.
