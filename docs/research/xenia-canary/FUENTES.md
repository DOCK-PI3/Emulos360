# Fuentes, trazabilidad y comprobaciones de la investigación

Consulta: **02-10-2026, Atlantic/Canary**. Se obtuvieron 84 respuestas públicas de un conjunto de 86 consultas HTTP para el análisis de repositorios, historial, PRs, wikis, comentarios, parches y código. Dos rutas wiki de Canary devolvieron 404; no se trataron como documentación disponible. Las fuentes complementarias consultadas mediante búsqueda web se enumeran aparte.

## Evidencia persistente

| Archivo | Contenido y alcance |
| --- | --- |
| [fuentes.json](fuentes.json) | URL, SHA-256 y tamaño de cada respuesta obtenida; errores de consulta explícitos |
| [auditoria-commits.json](auditoria-commits.json) | 30 commits seleccionados, hashes/títulos/fechas/URLs, archivos y resultado de los checks de contexto |
| [inventario-local.json](inventario-local.json) | Revisiones, cambios ya existentes y 127 archivos de referencia local con hash de bytes/hash normalizado LF y algunas anclas de símbolos |

Los hashes de fuente identifican **la respuesta HTTP obtenida**, no necesariamente el commit de la página. Los SHA de Git, hashes del sistema de parches Xenia y SHA-256 de archivos son identificadores distintos. En particular, la wiki tiene URL mutable; no se inventó una revisión inmutable.

El árbol de game-patches consultado se identifica por objeto tree `01bff555f18e58dde570588449cf3477ce2ea0b8`. Los diez TOML se descargaron por ese identificador y sus SHA-256 están en el manifest. No se denomina commit a un objeto tree.

Los cuerpos completos de las respuestas se conservan en la caché local ignorada por Git `local/xenia-research-20261002/`. Los scripts auxiliares `local/xenia-research-fetch.py`, `local/xenia-research-details.py` y `local/xenia-research-ledger.py` solo apoyan la investigación; no son cambios del motor ni scripts de compilación. **El dossier versionable contiene síntesis e inventarios, no una copia íntegra de wikis, issues o documentación del XDK.** La síntesis permite continuar aunque esa caché local no se distribuya.

## Repositorios y código fijado

- [Xenia Canary](https://github.com/xenia-canary/xenia-canary), HEAD consultado `44f5b4a86080189047e9a60f2d2b8041796fcb68`.
- [Xenia Canary Netplay](https://github.com/AdrianCassar/xenia-canary), HEAD consultado/base local `6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c`.
- [Comparación entre las revisiones](https://api.github.com/repos/AdrianCassar/xenia-canary/compare/6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c...44f5b4a86080189047e9a60f2d2b8041796fcb68): divergencia, ancestro común y commits exclusivos; no una lista de cambios listos para aplicar.
- [Xenia-WebServices fijado](https://github.com/AdrianCassar/Xenia-WebServices/tree/6abcc4397e0b6e0632307d9418ecae086cc95f48): versión utilizada por la preparación local, aunque `main` evolucione.
- [Controller de sesiones](https://github.com/AdrianCassar/Xenia-WebServices/blob/6abcc4397e0b6e0632307d9418ecae086cc95f48/src/infrastructure/presentation/controllers/session.controller.ts), [repositorio de sesiones](https://github.com/AdrianCassar/Xenia-WebServices/blob/6abcc4397e0b6e0632307d9418ecae086cc95f48/src/infrastructure/persistance/repositories/SessionRepository.ts) y [controller de títulos](https://github.com/AdrianCassar/Xenia-WebServices/blob/6abcc4397e0b6e0632307d9418ecae086cc95f48/src/infrastructure/presentation/controllers/title.controller.ts).
- [Game patches](https://github.com/xenia-canary/game-patches): diez variantes de Gears inspeccionadas; identificadores y hashes en `fuentes.json`.

El catálogo de fuentes incluye ventanas de commits y búsquedas de PRs: una entrada de historial sin diff auditado se describe como localizada, no como implementación examinada. El ledger de 30 commits contiene los diffs efectivamente contrastados con el árbol local.

## PRs gráficas analizadas

| Fuente | Evidencia utilizada |
| --- | --- |
| [PR 1016](https://github.com/xenia-canary/xenia-canary/pull/1016) | Modelo de informes/segmentos, modos de consulta, QueryBatch pendiente en esa etapa |
| [PR 1218](https://github.com/xenia-canary/xenia-canary/pull/1218) | Reescritura ZPD acumulativa, contadores y opciones retiradas |
| [PR 1111](https://github.com/xenia-canary/xenia-canary/pull/1111) | VIZ_QUERY/predicación y limitaciones expresadas por el autor |
| [PR 1163](https://github.com/xenia-canary/xenia-canary/pull/1163) | Layout canónico EDRAM y coherencia de aliases MSAA |
| [PR 1222](https://github.com/xenia-canary/xenia-canary/pull/1222) | Alias disjuntos de color/profundidad vinculados a Judgment |
| [PR 1190](https://github.com/xenia-canary/xenia-canary/pull/1190) | Arreglo general de precisión escalar relacionado con AC6 |
| [PR 1250](https://github.com/xenia-canary/xenia-canary/pull/1250) | Muestreo puntual y virtual texturing; plataformas reportadas y pendiente Intel Arc |
| [PR 1210](https://github.com/xenia-canary/xenia-canary/pull/1210) | Reloj vblank Linux, causa, solución y medida upstream acotada |

Se verificaron estados de integración y hashes devueltos por la API de estas PRs. El texto de un autor aporta intención y reportes; no sustituye prueba propia ni contrato oficial del hardware.

## Netplay y compatibilidad

- [Compatibilidad Netplay](https://github.com/AdrianCassar/xenia-canary/wiki/Netplay-Compatibility): marcas por juego y notas de servicio/System Link.
- [System Link](https://github.com/AdrianCassar/xenia-canary/wiki/Systemlink): interfaz, LAN/VPN y casos que dependen de servicio por carencias del fork.
- [Procedimiento de pruebas](https://github.com/AdrianCassar/xenia-canary/wiki/Testing-Procedure): entornos separados, LAN y clasificación local/pública.
- [Config Setup](https://github.com/AdrianCassar/xenia-canary/wiki/Config-Setup) y [FAQ Netplay](https://github.com/AdrianCassar/xenia-canary/wiki/FAQ): configuración vigente publicada y límites.
- [Team XLink — Xenia Support](https://www.teamxlink.co.uk/wiki/Xenia_Support): alcance experimental del puente y limitación de interoperabilidad con consola física; sus instrucciones pueden corresponder a versiones anteriores.
- Issues gráficos primarios: [Gears 1](https://github.com/xenia-canary/game-compatibility/issues/44), [Gears 2](https://github.com/xenia-canary/game-compatibility/issues/46), [Gears 3](https://github.com/xenia-canary/game-compatibility/issues/39), [Judgment](https://github.com/xenia-canary/game-compatibility/issues/45). Se revisaron las entradas y hasta 100 comentarios por issue; no se afirma haber seguido todas sus conversaciones externas.

Solo se usan fuentes primarias para afirmaciones técnicas. Mirrors, vídeos, blogs y mensajes de terceros localizados por buscadores no se convierten en evidencia de implementación. Los enlaces a distribución de juegos contenidos en páginas de compatibilidad no forman parte de este dossier.

## Fuentes complementarias consultadas en web

- [Microsoft — Rendering en D3D12](https://learn.microsoft.com/en-us/windows/win32/direct3d12/rendering): función de ROV en el ordenamiento de accesos UAV.
- [Khronos — Synchronization and Cache Control](https://docs.vulkan.org/spec/latest/chapters/synchronization.html): necesidad de dependencias para hazards, incluida escritura tras escritura.
- [Khronos — Synchronization Examples](https://docs.vulkan.org/guide/latest/synchronization_examples.html): referencia para futuras pruebas de barreras.

Estas páginas se consultaron mediante herramienta web; no se les atribuye un SHA-256 que no se calculó ni se las incorpora al recuento de respuestas de la caché.

## Resultado de las comprobaciones locales

De 30 diffs: **11** con hunks exactos presentes, **13** candidatos que encajan y **6** divergentes o parciales. Se conservaron retornos de ambas comprobaciones. Las comparaciones no escriben código y no demuestran que un candidato funcione, que tenga todas sus dependencias o que deba aplicarse.

Se verifican los JSON, referencias internas, hashes de la caché y correspondencia de URLs de commits con los metadatos obtenidos. No se han ejecutado tests de gameplay, benchmarks, compilación, tráfico de partidas ni capturas GPU. Las matrices de pruebas de los capítulos 06 y 07 son propuestas para la implementación posterior.

## Reglas para reutilizar el dossier

1. Confirmar primero que las revisiones y hashes todavía coinciden, especialmente con trabajo local activo.
2. Citar el commit y su fuente cuando se adopte un mecanismo; no atribuir a Canary cambios propios de Emulos360 o del backend privado.
3. Mantener separado el reporte upstream del resultado local. Documentar versiones, hardware y modo al afirmar compatibilidad.
4. Cuando cambie el modelo o desaparezca una opción, actualizar catálogo/UI y las recomendaciones por juego.
5. Guardar nueva evidencia de reproducción y no elevar una hipótesis a corrección confirmada por haberla escrito en un documento.
