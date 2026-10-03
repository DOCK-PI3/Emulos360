# 01 — Base local, diferencias y alcance de la evidencia

## Identidad de la base

| Componente | Identidad comprobada |
| --- | --- |
| Repositorio principal al investigar | `d28a784dea93d948c9f0f610c9ba289db11b9a80`, con modificaciones previas |
| Submódulo `engine/xenia` | `6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c`, 10-09-2026, mensaje sobre búfer de resultados XSession |
| Origen declarado en `.gitmodules` | `AdrianCassar/xenia-canary` |
| Remote `origin` efectivo del checkout | `xenia-canary/xenia-canary`; no coincide con `.gitmodules` |
| Rama predeterminada Netplay consultada | `netplay_canary_experimental`, mismo HEAD que la revisión local |
| Canary consultado | `canary_experimental`, `44f5b4a86080189047e9a60f2d2b8041796fcb68`, 01-10-2026 |
| Xenia-WebServices usado por MultiP | `6abcc4397e0b6e0632307d9418ecae086cc95f48`, según la preparación y documentación local |
| Historial local del submódulo | Checkout superficial: el log local no permite reconstruir toda la genealogía |

Fuentes: archivos locales indicados, `git rev-parse`, `git status`, `git remote -v`, API pública de [Canary](https://api.github.com/repos/xenia-canary/xenia-canary), [Netplay](https://api.github.com/repos/AdrianCassar/xenia-canary) y [comparación de ambas revisiones](https://api.github.com/repos/AdrianCassar/xenia-canary/compare/6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c...44f5b4a86080189047e9a60f2d2b8041796fcb68).

La comparación devuelve `diverged`: **50 commits exclusivos de Canary y 552 exclusivos de Netplay**, con ancestro común `577fb8ee17457af7493690a408492d800d53af58`. Son números de topología Git, no 602 funcionalidades ni 50 cambios íntegramente ausentes. Netplay incorpora modificaciones equivalentes con otros hashes y puede revertir cambios. La presencia se contrastó también con los diffs, no solo con fechas o títulos.

El archivo `command_processor.cc` descargado de la revisión Netplay fijada coincide con `git show HEAD:...` y con el archivo local al normalizar CRLF/LF. No se confundió una diferencia de finales de línea con una nueva implementación.

## Cambios locales que se deben preservar

El estado previo contiene integración de ventanas/perfiles, almacenamiento, avatares, red, adaptación POSIX y compilación. También hay cambios en la interfaz Qt y el actualizador. No se atribuyen a upstream ni se han corregido o deshecho durante la investigación. Se conserva un inventario de rutas y hashes en [inventario-local.json](inventario-local.json), sin perfiles, claves, partidas ni archivos de configuración personal.

MultiP ya aplica ajustes reproducibles a Xenia-WebServices mediante `server/prepare-upstream.cjs`: escucha local, gestión de rutas de datos, reducción de exposición de cabeceras y aplicación del límite de búsqueda después de filtrar. Cualquier futura actualización del backend necesita reevaluarlos.

## Cómo se comprobó la presencia de arreglos

Se descargaron **30 diffs de commits seleccionados** desde sus URLs públicas. Para cada diff se ejecutaron dos comprobaciones de contexto contra el árbol de trabajo actual:

- `git apply --check`: comprueba si el parche se podría aplicar sin modificar archivos.
- `git apply --reverse --check`: comprueba si el cambio exacto se podría retirar.

Interpretación: `present_exact_patch` aporta evidencia de que los hunks exactos están presentes; `candidate_applies` indica que el parche encaja en el contexto actual; `diverged_or_partial` requiere inspección. **Ningún resultado demuestra funcionamiento en ejecución, ausencia absoluta de una solución equivalente, ni aprobación para portar automáticamente.** Cambios sucesivos pueden impedir invertir un parche que sí fue incorporado. Los diffs grandes pueden encajar y aun así requerir código o shaders de otros commits.

Resultados completos y nombres de archivos: [auditoria-commits.json](auditoria-commits.json). Todos los checks terminaron sin aplicar parches.

## Selección de correcciones relevantes

| Cambio | Estado de los hunks en el árbol local | Lectura técnica |
| --- | --- | --- |
| `437a7280cf95` — EDRAM canónica, PR 1163 | Presentes | Ya existe el layout coherente para vistas 1x/2x/4x; no reimplementarlo |
| `3a44f20c7bc6` — precisión escalar y AC6 | Presentes | Ya existe el arreglo general que sustituye el hack del suelo de AC6 |
| `4cc584f47d31` — mezcla con alfa constante RTV | Presentes | Ya existe el cambio D3D12 inspeccionado |
| `95545f8e7603` — invalidación de fetchs | Presentes | Ya está corregido el extremo inclusivo del rango |
| `8ffe24e372fb` — publicación de entradas JIT bajo lock | Presentes | Ya existe la protección inspeccionada de la tabla de entradas |
| `ec5c875122b4` — optimizaciones VMX | Presentes | Ya existen optimizaciones de productos escalares, estimación de raíz, permutas y stores parciales |
| `7ff152a5a7f6` — NaN en min/max vectorial | Presentes | Ya existe esta corrección semántica |
| `ae18e30b8ff3` — bloqueo WSAGetOverlappedResult | Presentes | Ya existe la corrección Netplay; preservar su orden de espera y liberación del mutex |
| `7835c486a2ed`, `8bf03b0038a6`, `6dbaa1fefd1e` | Presentes | IDs XNKID, propiedades ordenadas y cambio de búfer por ID ya forman parte de la base |
| `1b242658e535` — lock de layouts Vulkan | Candidato aplicable | Corrección pequeña de corrupción de memoria al traducir shaders en paralelo |
| `c8f214e2a9b0` — barrera entre uploads Vulkan | Candidato aplicable | Conserva una dependencia write-after-write aunque los masks/layouts sean iguales |
| `9da693480d09` — alias de bits color/profundidad | Candidato aplicable | Caso directamente vinculado a sprites que atraviesan geometría en Judgment |
| `0bd090dbe979` — copia de profundidad MSAA | Candidato aplicable | Sigue al layout canónico ya presente; requiere reconstruir shaders afectados |
| `89609297c7ae` — resolve tiled 8/16bpp | Candidato aplicable | Corrige el reparto de offsets entre base y coordenada |
| `bc6b7a020295` — mipmaps de arrays/volúmenes | Candidato aplicable | Alineación de slices y reducción de profundidad por mip |
| `c3cd8617b18e` — muestreo puntual | Candidato aplicable | Corrección de virtual texturing idTech 5, no parche específico de Gears |
| `44f5b4a86080` — reloj vblank Linux | Candidato aplicable | Corrige deriva temporal bajo carga; no equivale a aumentar FPS renderizados |
| `9363df7d8fe0`, `5ce7c336df06`, `920f35c9386e` | Candidatos aplicables | Semántica de registros temporales y constant folding de vectores |
| `577fb8ee1745` — mcrf | Candidato aplicable, **revertido en Netplay** | La historia Netplay incluye `7035e676...` revirtiéndolo; investigar la razón antes de trasladarlo |
| `3d233a5b2e94` — ZPD acumulativo | Candidato aplicable, cambio amplio | Retira opciones y reorganiza informes y backends; debe tratarse como unidad coherente |
| `692cd59cfef6` — VIZ_QUERY | Contexto divergente | Depende de evolución de oclusión y recursos; no portar antes de analizar sus predecesores |

Cada hash completo y URL se encuentra en el inventario. Los mecanismos se desarrollan en los capítulos 02–04. El estado de un candidato no constituye una solicitud de implementación en esta investigación.

## Jerarquía de evidencia

**Código inspeccionado:** prueba qué hace una revisión, con sus límites y TODOs. **Diff/PR integrado:** muestra el cambio y la intención; la descripción de un autor no acredita todas las plataformas. **Reporte de compatibilidad:** observación de una combinación de versión, hardware y ajustes; puede quedar obsoleta o no reproducirse. **Hipótesis propia:** explicación o propuesta a validar. **Pendiente:** falta de evidencia suficiente.

No se trata el código reverse engineered de Xenia como contrato oficial del XDK. Para una ABI nueva se requieren prototipo, convención PowerPC, estructura, endianness, módulo/ordinal y evidencia de la versión. Un nombre de export o un stub que devuelve éxito no valida esas condiciones.

## Límites de esta revisión

Se consultaron ventanas acotadas: 100 commits generales/GPU, 50 de CPU, 60 de XSession y 60 de XSocket; PRs seleccionadas; cuatro issues y sus comentarios de Gears, wiki Netplay, diez parches de Gears y partes del backend. No se ha hecho una auditoría completa de cada instrucción, export o juego. Las páginas wiki son mutables y se identifican por fecha y SHA-256 del contenido; no se les inventa un commit.

No se ejecutaron títulos ni sesiones entre máquinas, ni se midió el rendimiento del PC del usuario. No se diagnosticó hardware personal. Hay archivos de código privados/locales y datos de usuario que deliberadamente no forman parte del dossier.
