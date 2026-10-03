# 03 — CPU, rendimiento, memoria, audio y estabilidad

## Mapa de responsabilidades

| Área | Código local | Pregunta que responde |
| --- | --- | --- |
| PowerPC → HIR | `cpu/ppc/ppc_emit_*.cc`, `ppc_hir_builder.*` | ¿Se conserva el significado de la instrucción? |
| Optimización HIR | `cpu/hir/`, `cpu/compiler/` | ¿Constant folding y otras transformaciones conservan resultados y efectos? |
| Backend host | `cpu/backend/x64/`, `cpu/backend/a64/` | ¿Código emitido, registros y capacidades CPU son correctos? |
| Entrada/compilación JIT | `cpu/entry_table.*`, `cpu/processor.cc` | ¿Cada hilo ve una función completamente publicada? |
| Memoria | `memory.*`, `base/memory.*`, `gpu/shared_memory.*` | ¿CPU y GPU ven datos coherentes y límites correctos? |
| Kernel/hilos | `kernel/xthread.*`, `xevent.*`, objetos de sincronización | ¿Esperas, eventos, APC y cancelaciones siguen el contrato? |
| Audio | `apu/xma_decoder.*`, `xma_context*`, `audio_system.*` | ¿Se entregan muestras y eventos a tiempo? |
| VFS/contenido | `vfs/`, `kernel/xam/content_manager.*`, paquetes | ¿Fallos de carga se originan en I/O o en emulación? |

Todas las rutas cortas parten de `engine/xenia/src/xenia/`. El código es evidencia de implementación de esa revisión, no prueba de todas las semánticas del hardware. [Árbol público fijado](https://github.com/AdrianCassar/xenia-canary/tree/6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c/src/xenia).

## 1. Corrección CPU que puede parecer un fallo gráfico o de red

Una comparación vectorial, un registro temporal pisado o una rama mal traducida puede alterar geometría, lógica de IA, selección de recursos y estados de sesión. Antes de parchear GPU o matchmaking, confirmar que los datos que produce CPU son correctos. En particular, optimización HIR y código dinámico deben coincidir cuando operan sobre entradas equivalentes.

### Igualdad float en constant folding

[`920f35c9386e`](https://github.com/xenia-canary/xenia-canary/commit/920f35c9386e1738b7a80e62f9bf8d484da03320) corrige igualdad vectorial float que se plegaba como comparación de bits. `+0.0` y `-0.0` tienen representación distinta pero son iguales según la comparación flotante; NaN no debe volverse igual a sí mismo porque sus bits coincidan.

**Candidato aplicable.** La prueba útil compara resultados constantes con resultados dinámicos e incluye ceros con signo y NaN. No basta comprobar valores positivos normales. Esta es una corrección semántica; no se le atribuye un aumento medido de FPS.

### Lane y extracción constantes

[`5ce7c336df06`](https://github.com/xenia-canary/xenia-canary/commit/5ce7c336df060d043b0a2351889ec8789322d035) corrige la selección de lane en Extract constante y ajusta pruebas vectoriales. La organización de los lanes depende de la representación interna y endianness: un índice lógico no se debe interpretar como offset host sin seguir el contrato. **Candidato aplicable** junto con su validación upstream.

### Registros temporales del backend x64

[`9363df7d8fe0`](https://github.com/xenia-canary/xenia-canary/commit/9363df7d8fe02f575442b57a94cb7f55da5ce9d5) corrige dos secuencias: una máscara de igualdad pisaba el registro que contenía un operando constante, y otra ruta usaba como scratch un registro que el allocator podía asignar a un valor vivo.

**Candidato aplicable.** La regla general es mantener separados los temporales reservados y los registros asignables. Validar operandos constantes/dinámicos y presión de registros; un test sin registros vivos podría no activar el fallo.

### mcrf y un revert que no se debe ignorar

El cambio [`577fb8ee1745`](https://github.com/xenia-canary/xenia-canary/commit/577fb8ee17457af7493690a408492d800d53af58) evita tratar una copia de campo CR como actualización derivada de comparar un valor. El código local contiene `StoreCR` seguido de `UpdateCR`. La rama Netplay incluye [`7035e676...`](https://github.com/AdrianCassar/xenia-canary/commit/7035e67615a9b95f27251ba155f6b37efed7f535) revirtiendo el cambio.

El diff original encaja localmente, pero **se desconoce la justificación funcional del revert**. Antes de recomendarlo, estudiar campos CR, XER/SO, representaciones del builder y tests con patrones de bits, incluyendo el motivo del mantenedor si existe. Un revert en una rama es evidencia que obliga a investigar; no prueba automáticamente que el cambio inicial esté mal ni que se deba reintroducir.

## 2. Optimizaciones que ya existen

- [Publicación JIT bajo mutex](https://github.com/xenia-canary/xenia-canary/commit/8ffe24e372fb5c137f14d776e9645b03eafc703a): protege resultados de entrada en la tabla mientras se resuelve/compila una función. Los hunks están presentes.
- [Optimización VMX/x64](https://github.com/xenia-canary/xenia-canary/commit/ec5c875122b43a596fd5fd18edfa5e441b8521a8): reduce trabajo en productos escalares, estimación de raíz, stores parciales y permutas. Está presente; reevaluar solo ante un cuello de botella medido.
- [Min/max vectorial con NaN](https://github.com/xenia-canary/xenia-canary/commit/7ff152a5a7f69390c1a625d3fdc57681d5ccb7e9): corrige reglas semánticas, ya presente.
- [Invalidación excesiva de fetchs D3D12](https://github.com/xenia-canary/xenia-canary/commit/95545f8e76035d8b904e120a0bf379cbc300d86d): resta uno para calcular el último fetch de un rango inclusivo. Evita invalidar estado ajeno; presente.

No imponer una vía AVX-512 o instrucciones opcionales a todas las CPUs. Los caminos elegidos deben corresponder a detección real de capacidades y conservar resultados. Optimizar un benchmark sin incluir el fallback puede mejorar una máquina y romper otra. No se propone un backend CPU nuevo, recompilación estática universal o sustituir el JIT sin evidencia de necesidad.

## 3. Memoria y coherencia CPU/GPU

Tres espacios deben distinguirse: memoria virtual del invitado, representación host de sus páginas y recursos GPU que contienen/representan esos datos. Una escritura CPU puede invalidar textura o vertex buffer; una exportación GPU puede necesitar llegar a CPU. Suponer coherencia automática o marcar todas las páginas como modificadas desperdicia tiempo o introduce datos antiguos.

Preguntas para cada región compartida:

1. ¿Qué dirección virtual/física y longitud utiliza el juego? ¿Hay alias de páginas o de formato?
2. ¿Quién puede escribir: CPU, GPU, DMA emulado, I/O o un hilo de red?
3. ¿Qué dirty/watch state se actualiza y cuándo se vuelve visible al consumidor?
4. ¿Puede estar en vuelo el recurso al reciclarlo o liberarlo?
5. ¿Cómo se verifica el overflow en suma/multiplicación de tamaño, y la validez de un puntero guest?

El README del fork documenta cambios propios ya reproducidos con UBSan en Linux: lectura/escritura de XConfig mediante copias de bytes y desplazamientos sin signo en LZX. [Registro local](../../FORK_CHANGES.md). No se deben eliminar por considerar que una máquina x64 tolera accesos desalineados. Alineación de C++, tamaño de estructura y resultado en hardware son cuestiones diferentes.

Para mejorar estabilidad, revisar primero límites y vida de objetos frente a hacks que amplían memoria o ignoran accesos inválidos. Opciones como `gpu_allow_invalid_upload_range` o `gpu_allow_invalid_fetch_constants` pueden permitir avanzar sin identificar por qué aparece el estado inválido; el diagnóstico debe conservar el primer origen de ese dato.

## 4. Concurrencia: quitar esperas innecesarias sin quitar garantías

La mejora del [lock de layouts Vulkan](https://github.com/xenia-canary/xenia-canary/commit/1b242658e535fbd12645e4d2c12693771075b738) demuestra una carrera real en la precarga, no una cuestión estética de código. La solución protege el vector y el map durante registro; desactivar el paralelismo solo evita el detonante.

La corrección Netplay de [WSAGetOverlappedResult](https://github.com/AdrianCassar/xenia-canary/commit/ae18e30b8ff3a3dd2d617124dc10108133b01743) ya evita esperar siempre sobre un evento de autoreset y libera el mutex necesario para que la tarea pueda terminar. El patrón aplicable a todo el core es comprobar el estado bajo protección, permitir que el productor avance durante una espera y reevaluar el estado al volver.

Áreas a perfilar, **propuestas**, no defectos confirmados: contención de tablas JIT, locks de cachés GPU, asignaciones por draw, hilos por operación de red y tareas QoS. Sustituirlas por pooling, queues o I/O coordinada requeriría mantener cancelación, orden de completado y ownership. Un hilo más barato que termina después de liberar memoria guest es una regresión de estabilidad.

La investigación posterior de [Rivals y Edge](09-NFS-RIVALS-CONTENIDO-HD.md#comparación-estática-ampliada-de-edge)
fija Edge `34357e25782dca8214b171fd19cf6889139c442a` y estudia su scheduler
cooperativo, safepoints JIT, esperas e I/O. La base Netplay local conserva hilos
host por hilo guest. Esa diferencia es un candidato de investigación, sin
causa del bloqueo ni mejora de rendimiento demostradas. Su integración requiere
una unidad coherente de CPU/kernel/I/O y validación de los juegos ya funcionales;
activar una opción ausente o copiar solo los cambios de eventos no la implementa.

## 5. Audio, reloj y ritmo de frames

En la base, `xma_decoder` declara `new` y `use_dedicated_xma_thread` declara `true`. Los backends de salida varían según plataforma. Un comentario de compatibilidad con un ajuste que elimina audio repetido no prueba una mejora universal del decodificador.

La [PR 1210](https://github.com/xenia-canary/xenia-canary/pull/1210), [`44f5b4a86080`](https://github.com/xenia-canary/xenia-canary/commit/44f5b4a86080189047e9a60f2d2b8041796fcb68), analiza deriva de vblank en Linux: dormir un periodo fijo tras despachar el evento acumula coste de trabajo y latencia de despertar. El audio avanza con su dispositivo; imagen/subtítulos se atrasan respecto a esa línea temporal.

El arreglo avanza una fecha límite por periodo, compensa despertares tardíos y limita recuperación tras congelaciones prolongadas. También revisa la prioridad Linux de ese hilo. El autor reporta una prueba de cutscene de Lost Odyssey con recuentos vblank estables bajo carga. Es **medición upstream de ese caso**, no un benchmark ejecutado aquí. El diff encaja localmente.

Validación propuesta: medir contador vblank invitado, frames presentados y deriva audio/vídeo por separado; reproducir idle, carga sostenida y pausa larga. Frames presentados pueden caer bajo carga aunque el reloj ya no derive. Aumentar artificialmente velocidad del invitado o desactivar vsync no sustituye una solución del reloj.

## 6. Clasificación del cuello de botella

| Síntoma observado | Evidencia a recoger | Familia de mejora a investigar |
| --- | --- | --- |
| Tirón la primera vez que aparece un efecto | Tiempo de traducción y pipeline; repetición con cache caliente | Cache y compilación paralela segura |
| Caída proporcional a resolución/MSAA | Tiempo GPU, píxeles, samples y VRAM | Render targets, transfers, overdraw y shaders |
| CPU alta con GPU poco ocupada | Perfil por función, threads y waits | JIT, HLE, submissions o I/O; distinguir uso activo de espera |
| GPU/CPU esperando una a otra | Fences, readback y tamaño/frecuencia de transferencias | Coherencia y lectura selectiva, preservando consumidores |
| Texturas tardías durante carga | I/O, decode, texture uploads y límites del juego | Streaming y conversión; no asumir disco lento |
| Fluidez media buena y picos largos | Distribución de frametimes, faults, alloc y locks | Ritmo de presentación y estabilidad |
| Audio/vídeo se separan gradualmente | Clock/vblank/cola audio y frames presentados | Reloj y scheduling |
| Cierre al arrancar con cache caliente | Stack, sanitizer y concurrencia de precarga | Ownership y protección de caches |

Instrumentos sugeridos: perfiles CPU host con stacks; eventos/timestamps GPU del backend; PIX/RenderDoc cuando admitan el modo de recursos; trazas Xenia para relacionar el trabajo host con el comando guest. Validación Vulkan y D3D12 debug layer ayudan a detectar errores de API. No usar sus costes como rendimiento de Release.

## 7. Protocolo de medición para futuras mejoras

Registrar build/compiler, OS, CPU/GPU/driver, juego/versión, configuración y escena. Mantener el mismo recorrido y duración, y separar carga fría de caliente. Medir **frametime p50/p95/p99, tiempo de carga, RAM/VRAM, pausas largas, fallos y deriva audio**, además de FPS. Repetir suficiente para estimar variación y conservar datos originales.

Un objetivo futuro puede ser reducir un p99 concreto sin cambiar resultado gráfico o simulado. No se fija una cifra de mejora antes de tener baseline. Para un arreglo de estabilidad el criterio puede ser reproducción que deja de fallar y protección verificada de la causa; no una sesión que arrancó una vez.

Sanitizers: usar pruebas acotadas del componente y una build compatible con el JIT, las reservas de memoria y las bibliotecas. AddressSanitizer/UBSan/ThreadSanitizer no son intercambiables; algunas rutas generadas y GPU no quedan cubiertas. Un test limpio no prueba coherencia GPU ni fidelidad PowerPC. Las propuestas de tests y acceptance se detallan en [07](07-PLAN-Y-VALIDACION.md).
