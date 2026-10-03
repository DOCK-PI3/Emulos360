# Investigación del core y multijugador de Xenia Canary para Emulos360

Fecha de consulta: **2 de octubre de 2026**. Alcance: investigación y documentación; no se ha modificado, compilado ni ejecutado el motor, instalado software de red o cambiado perfiles, juegos o configuración. Las comprobaciones de parches usaron exclusivamente `git apply --check` y `--reverse --check`, que no aplican cambios.

Esta documentación sirve como base de trabajo para futuras implementaciones. No es una garantía de emulación perfecta: cada cambio requiere reproducción, revisión y validación con los juegos y equipos afectados. Las conclusiones distinguen código inspeccionado, reportes upstream e hipótesis de mejora.

## Lectura y navegación

| Documento | Utilidad |
| --- | --- |
| [01 — Base local y comparación upstream](01-BASE-Y-EVIDENCIA.md) | Qué motor tenemos, qué arreglos están presentes y cuáles son candidatos reales |
| [02 — Gráficos y correcciones](02-GRAFICOS.md) | EDRAM, MSAA, shaders, oclusión, texturas, readback, sincronización y diagnóstico |
| [03 — CPU, rendimiento y estabilidad](03-CORE-RENDIMIENTO-ESTABILIDAD.md) | Recompilación PowerPC, HIR/VMX, memoria, hilos, audio y medidas de rendimiento |
| [04 — Arquitectura de red](04-NETPLAY-Y-SYSTEM-LINK.md) | Recorrido XAM → sesiones → API → sockets; System Link, QoS, NAT y límites |
| [05 — Gears of War y otros casos](05-GEARS-Y-CASOS.md) | Estado reportado de la saga de Xbox 360, parches por versión y casos transferibles |
| [06 — Incorporar un título](06-METODO-PARA-NUEVOS-TITULOS.md) | Procedimiento para localizar la función/protocolo ausente y demostrar compatibilidad |
| [07 — Prioridades y validación](07-PLAN-Y-VALIDACION.md) | Backlog concreto, dependencias, pruebas propuestas, criterios y plantillas |
| [08 — Biblioteca y Sacred 2](08-CANDIDATOS-BIBLIOTECA.md) | 80 títulos cruzados, candidatos de multijugador, leyenda y evidencia de Sacred 2 |
| [09 — Rivals y contenido HD](09-NFS-RIVALS-CONTENIDO-HD.md) | Error inicial, carga detenida con HD/TU también sin red según la prueba del usuario, y diferencias del scheduler de Edge |
| [10 — Seguimiento y mejoras del core](10-SEGUIMIENTO-Y-MEJORAS-DEL-CORE.md) | Canary actualizado, cuatro candidatos preparados, compilación aislada de dos fuentes Vulkan y revisión semanal |
| [11 — Segundo core opcional: Edge](11-SEGUNDO-CORE-EDGE.md) | Integración 0.4.0, selección por título, aislamiento, builds y límites de las pruebas |
| [Fuentes e inventarios](FUENTES.md) | Enlaces, revisiones, hashes y límites de la investigación |

## Conclusiones que deben orientar el siguiente trabajo

1. **Actualizar selectivamente la base antes de inventar arreglos.** El submódulo está fijado al fork Netplay `6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c`. Canary consultado llega a `44f5b4a86080189047e9a60f2d2b8041796fcb68`. Las ramas divergen; reemplazar el motor por Canary perdería trabajo Netplay y cambios propios. [Comparación reproducible](01-BASE-Y-EVIDENCIA.md).
2. **Separar corrección de hardware, configuración y modificación del juego.** ROV/FSI, el layout de EDRAM y la sincronización corrigen clases de problemas. Un parche que elimina AO, sombras o MSAA puede aliviar un defecto y cambiar la imagen. Debe registrarse ese coste. [Análisis gráfico](02-GRAFICOS.md).
3. **Medir el cuello de botella.** Tirones por compilación, esperas de readback, saturación GPU, JIT, streaming y deriva del reloj son fenómenos distintos. Más FPS medios no demuestra estabilidad, fidelidad o mejor ritmo de presentación. [Core y medición](03-CORE-RENDIMIENTO-ESTABILIDAD.md).
4. **Netplay no replica universalmente todo Xbox Live.** El fork reproduce servicios y contratos necesarios para títulos compatibles. Xenia-WebServices coordina sesiones; el transporte de partida sigue dependiendo de los sockets y del juego. Un servicio específico de un editor puede requerir ingeniería independiente. [Recorrido de red](04-NETPLAY-Y-SYSTEM-LINK.md).
5. **La saga de Xbox 360 ofrece un buen banco de casos.** La wiki marca Gears 1, 2 y 3 con funcionamiento público, y Judgment como probado localmente. Los cuatro tienen System Link reportado sin servidor. Esto es evidencia upstream, no una partida validada en Emulos360. [Matriz y límites](05-GEARS-Y-CASOS.md).

## Uso en futuras sesiones

La referencia a este dossier está también en el `AGENTS.md` de la raíz. La evaluación posterior de biblioteca usó únicamente `Emulos360 --inventory`; no ejecutó juegos ni el core.

Antes de modificar el core, leer el capítulo del subsistema y consultar `auditoria-commits.json`. Verificar que el hash y el código actual todavía corresponden al inventario; los cambios locales pueden haber evolucionado. Reproducir un defecto concreto, formular su contrato e identificar el primer resultado incorrecto. Portar una unidad coherente con sus shaders, estructuras y opciones; volver a medir. No convertir una hipótesis de esta investigación en un hecho comprobado ni activar ajustes para todos los títulos por una prueba aislada.

La documentación previa del producto sigue siendo útil: [arquitectura](../../ARCHITECTURE.md), [Netplay](../../NETPLAY_OPTIONS.md), [MultiP](../../PRIVATE_NETPLAY.md), [integración Qt](../../QT_NETPLAY_INTEGRATION.md), [cambios propios](../../FORK_CHANGES.md) y [estado](../../STATUS.md). Este dossier añade mecanismos, historial y criterios de ingeniería.

## Seguimiento posterior del 2 de octubre

La investigación inicial anterior conservó el motor sin compilarlo. La fase de
[mantenimiento del capítulo 10](10-SEGUIMIENTO-Y-MEJORAS-DEL-CORE.md) añade una
preparación y compilación aislada de dos fuentes Vulkan parcheadas; no cambia
el core activo ni demuestra gameplay. Canary consultado después avanzó a
`b083312b8b18e07e6e410b82104191f126722794`. Para reutilizar conclusiones sobre el
estado actual, consultar también esa ficha y su evidencia, manteniendo los
hashes iniciales como registro histórico.

## Integración posterior del 3 de octubre

El usuario autorizó un segundo core opcional. Consulta el [capítulo 11](11-SEGUNDO-CORE-EDGE.md) y [DUAL_CORE.md](../../DUAL_CORE.md) antes de cambiar sus contratos. El core activo conserva su binario y el gameplay pendiente continúa distinguiéndose de las pruebas de integración.
