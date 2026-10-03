# 05 — Gears of War y casos que ayudan a generalizar

## Alcance de la saga

Aquí se estudian **Gears of War, Gears of War 2, Gears of War 3 y Gears of War: Judgment de Xbox 360**. No se extienden los resultados a Ultimate Edition, Gears 4, Gears 5, Tactics u otras plataformas. Compartir saga o motor de juego no implica compartir binario, APIs de red o pipeline gráfico.

La [tabla Netplay del fork](https://github.com/AdrianCassar/xenia-canary/wiki/Netplay-Compatibility) consultada el 02-10-2026 distingue funcionamiento público, pruebas locales, solo local y System Link. Es un reporte del proyecto. No aporta en cada fila build completa, TU y todos los modos; esos detalles deben fijarse al reproducir.

| Título Xbox 360 | Title ID de la edición principal | Netplay según wiki | System Link según wiki |
| --- | --- | --- | --- |
| Gears of War | `4D5307D5` | Marcado funcionando públicamente | Sin servidor |
| Gears of War 2 | `4D53082D` | Marcado funcionando públicamente | Sin servidor |
| Gears of War 3 | `4D5308AB` | Marcado funcionando públicamente | Sin servidor; reportado más estable que modo Live |
| Judgment | `4D530A26` | Probado localmente; sin marca de funcionamiento público | Sin servidor |

En Gears 3 la fila menciona player/ranked y versión base como combinación más estable para matchmaking y cooperativo. No convierte todos los modos, campañas, DLC y TUs en validados. En Emulos360 sigue pendiente una sesión completa entre dos equipos: [estado previo](../../NETPLAY_OPTIONS.md).

## Qué se puede afirmar de su funcionamiento online

Los títulos pueden aprovechar la implementación común de perfiles, XSession/XGI, direcciones XNet, QoS, sockets y backend descrita en [04](04-NETPLAY-Y-SYSTEM-LINK.md). El listado y código inspeccionados no demuestran un adaptador exclusivo que implemente toda la red de Gears fuera del juego. Para saber qué exports recorre cada modo hace falta una traza de ejecución por título y versión.

Esto delimita el trabajo futuro: probar primero el System Link que upstream reporta autónomo; después comparar las llamadas de un modo Live. Si ambos PCs ven el lobby pero no entra el cliente, estudiar direcciones, handshake y overlapped. Si ni siquiera encuentra la sala, estudiar anuncios, query ID, contextos, propiedades, QoS y perfiles. Si cae en juego, separar desincronización/protocolo de CPU, gráficos y cierre de recursos.

La configuración de render puede también afectar estabilidad online: un device lost o bloqueo del core termina la comunicación aunque la API de sesiones funcione. No diagnosticar una desconexión como red sin revisar el primer fallo de ambos procesos.

## Gears 1: iluminación, flares y streaming

Issue primario: [compatibilidad 44](https://github.com/xenia-canary/game-compatibility/issues/44). El [reporte sobre ZPD de junio](https://github.com/xenia-canary/game-compatibility/issues/44#issuecomment-4592783565) vincula consultas reales con flares correctamente ocluidos. Hay reportes posteriores de diferencias Vulkan/Linux y de problemas de sombreado; el hardware y la fecha importan.

El parche base se identifica con `hash = 1B591620508434A2`. El arreglo de sombreado modifica estados relacionados con tiling/MSAA; el de streaming aumenta el pool configurado, retira un límite por frame y estabiliza un factor de presupuesto. Son **modificaciones del juego** que evitan o alteran rutas problemáticas, no pruebas de que el core haya emulado esas rutas exactamente. La edición TU5 y Japón usan ficheros separados.

Investigación propuesta: capturar un personaje con iluminación incorrecta sin parche, seguir light environment → MSAA/resolve → sampling y contrastar con target host/ROV. Para streaming, medir cuánto tarda en aparecer la textura, lecturas/upload y memoria; aumentar pool no es necesariamente reparar el cargador.

## Gears 2: las mejoras de calidad y rendimiento tienen efectos distintos

Issue primario: [compatibilidad 46](https://github.com/xenia-canary/game-compatibility/issues/46). Un [reporte de ZPD](https://github.com/xenia-canary/game-compatibility/issues/46#issuecomment-4592773074) favorece `strict` para flares; es observación de su contexto. Los comentarios también incluyen device lost y conjuntos de parches amplios: no permiten atribuir causalidad a una opción aislada.

Parche base: `BDB97753374D520A`; TU6: archivo distinto. El arreglo de sombreado desactiva una ruta MSAA/tiling. Los ajustes AO/DoF pueden evitar artefactos de escalado; las opciones que los desactivan cambian el aspecto. La optimización de sombras reduce carga mediante cambios del juego. La modificación de streaming altera pool y cálculo de presupuesto.

Para separar causas: baseline 1x1; sombreado sin otros parches; escalado con AO/DoF; oclusión; rendimiento con sombras. Comparar imágenes y frametimes. En multiplayer, un desbloqueo de FPS no se presupone seguro para simulación: probar compatibilidad temporal con el resto de participantes, especialmente si sus parches difieren.

## Gears 3: QueryBatch, full-depth occlusion y escalado selectivo

Issue primario: [compatibilidad 39](https://github.com/xenia-canary/game-compatibility/issues/39). Parche base `EF4AD5E9DD1982ED`; existe variante para TU6 disco/TU1 XBL. El parche de oclusión fuerza una ruta de test a profundidad de resolución completa para reducir popping. Esa modificación ayuda a una interacción concreta con las queries, no demuestra que todos los contadores del core sean correctos.

La reescritura ZPD de [PR 1218](https://github.com/xenia-canary/xenia-canary/pull/1218) es una mejora general para estudiar QueryBatch. El [comentario sobre umbral de escalado](https://github.com/xenia-canary/game-compatibility/issues/39#issuecomment-5064853506) propone valores 364 o 512 para preservar postprocesado con targets host. Deben probarse como candidatos; el código clasifica por pitch alineado y algunas transferencias MSAA/alias pueden alterar la resolución efectiva.

Un [seguimiento de septiembre](https://github.com/xenia-canary/game-compatibility/issues/39#issuecomment-5580362288) procede de un proyecto derivado de recompilación estática, con su propio renderizador adaptado. **No acredita reproducción ni arreglo en Xenia Canary actual.** No se usa aquí como base para implementar automáticamente el cambio sugerido.

Banco de validación propuesto: objetos ocultos y flares, escena con AO/DoF, cinemática, cambio de zona y partida cooperativa. Medir native/scaled y con/sin threshold, además de una vista con MSAA reinterpretada. Comparar detalle real, no solo tamaño del framebuffer final.

### Cierre aislado local del 2 de octubre

El usuario reporta que Gears 3 cerró tras instalar un paquete de contenido y
arrancar con Netplay activo. La captura muestra la pantalla de desafíos,
build `codex/netplay-integration@6dbaa1f`, versión de título `0.0.0.9`, hilo
`XThread0784 (F800014C)` y dirección `Emulos360-core.exe+2EA781`.
El diálogo imprime `0x3221225477`, pero el formateador local antepone `0x` a
un número decimal: `3221225477` equivale a `0xC0000005`, violación de acceso.
La salida decimal `-1073741819` que muestra la aplicación es el mismo código.

**Inspección local:** el ejecutable desplegado en
`C:/Users/mabe8/Desktop/Emulos360-BETA12/engine/Emulos360-core.exe` y el de
`engine/xenia/build/bin/Windows/Release/` tienen el mismo SHA-256
`63f44fb5cfca5ba3a20a4e9475572a8e81839efb4930bc222a6285cb589b2569`.
La resolución local con DbgHelp y el PDB de esa build sitúa el RVA `0x2EA781`
en `xe::kernel::xboxkrnl::xeKeSetEvent+0x71`,
`xboxkrnl_threading.cc:587` (`ev->Set(increment, !!wait)`). Esto localiza el
punto de fallo en la ruta de eventos de sincronización; no identifica el
llamador, el acceso inválido exacto ni el origen del estado incorrecto.
No hay dump ni stack completo del incidente analizado. El hash del binario
en el instante del cierre no quedó capturado; la captura coincide con la
identificación de build del ejecutable disponible.

**Gameplay reportado:** después jugó un par de partidas sin conexión y varias
rondas tras volver a activar Netplay, sin otro cierre. No se especifican otros
equipos/jugadores conectados ni modos, por lo que no se eleva este reporte a
validación de multijugador entre equipos. Los registros disponibles a las
14:29:41 son de una ejecución posterior con `network_mode = 2`, actividad de
sesiones y XStorage, y terminan en `Cheap-skate exit!`; no contienen el fallo
de las capturas tomadas sobre las 13:43–13:44. El logger abre el archivo con
modo `wt`, sobrescribiendo la ejecución anterior.

**Hipótesis pendiente:** orden de inicialización, vida/estado de un evento o
concurrencia entre tareas. No hay evidencia suficiente para atribuirlo al DLC,
a XStorage, a un servidor o a Netplay, ni para afirmar que jugar offline lo
reparó. Conservar configuración y contenido que ahora funcionan. Si se repite,
preservar ambos logs antes de volver a ejecutar el core y obtener un dump/stack
para seguir el origen. No se modificó ni ejecutó el motor durante el diagnóstico.

## Judgment: dos fallos diferentes con evidencia concreta

Issue primario: [compatibilidad 45](https://github.com/xenia-canary/game-compatibility/issues/45). Parche base `8429D0AE190C10DB`; TU4 separado.

1. **Sprites/nubes a través de paredes:** [PR 1222](https://github.com/xenia-canary/xenia-canary/pull/1222) corrige el tratamiento de alias disjuntos color/profundidad. Candidato local, explicado en [02](02-GRAFICOS.md). No es simplemente bajar calidad o borrar sprites.
2. **Escenas excesivamente brillantes:** el [reporte de readback](https://github.com/xenia-canary/game-compatibility/issues/45#issuecomment-5491463243) propone `readback_resolve` fast/full. El core puede necesitar devolver datos GPU a CPU para un cálculo posterior. No mezclar ese ajuste con `occlusion_query = strict`.

Los parches de oclusión y escalado también existen, pero las modificaciones de postprocesado y FXAA cambian el pipeline y calidad. Judgment es un buen primer caso porque una corrección del core tiene vínculo upstream explícito con el síntoma.

## Identificación de parches por versión

Se inspeccionaron diez TOML del [repositorio de game-patches](https://github.com/xenia-canary/game-patches/tree/01bff555f18e58dde570588449cf3477ce2ea0b8/patches). La selección depende del hash de módulo y edición, no solo Title ID. Los hashes anteriores son **identificadores del sistema de parches de Xenia**, no SHA-256 de archivo ni prueba de Media ID/TU idénticos.

No editar un hash para forzar un parche sobre otro binario: las direcciones cambian y un write correcto para una versión puede corromper instrucciones de otra. Para crear una variante, localizar semánticamente la instrucción/dato en la nueva versión, verificar bytes previos y contexto, y restringirla a su hash. Un parche público habilitado no demuestra seguridad multijugador ni efectos sobre todos los modos.

Los scripts de investigación no descargaron juegos, saves o DLC. Los enlaces de archivos de terceros presentes en la wiki no se usan como evidencia ni se reproducen como fuentes del dossier.

## Otros títulos: clases de problemas transferibles

Todas las filas siguientes son **reportes de la wiki Netplay consultada**, no validación propia. [Fuente](https://github.com/AdrianCassar/xenia-canary/wiki/Netplay-Compatibility).

| Caso | Qué reporta el proyecto | Lección para nuevos títulos |
| --- | --- | --- |
| Call of Duty 3 | System Link depende de servicio por MAC | Implementar metadata/direcciones correctas; una LAN funcional no resuelve todos los contratos |
| Red Dead Redemption | System Link depende de servicio por QoS | El payload de consulta puede ser parte de la admisión del lobby |
| Call of Duty: Black Ops | Netplay marcado no funcional, System Link sí; MP/Zombies reportados | Modos de red distintos necesitan validación distinta |
| Unreal Tournament 3 | Probado localmente y System Link sin servidor | Comparar un título con ruta de plataforma parecida, sin asumir binarios/protocolos equivalentes |
| Test Drive Unlimited | Player/ranked reportado; free roam requiere servidor dedicado | Compatibilidad es por modo; descubrimiento genérico no reemplaza servicios del mundo persistente |
| God Mode | System Link sin servidor y receta histórica de QueryBatch | Las recetas deben migrarse al modelo de oclusión de la build actual |

Fuera de red, el [arreglo de idTech 5](https://github.com/xenia-canary/xenia-canary/pull/1250) permite estudiar texturas virtuales y precisión de muestreo; el [layout EDRAM](https://github.com/xenia-canary/xenia-canary/pull/1163) enseña a corregir coherencia entre vistas; y el [vblank Linux](https://github.com/xenia-canary/xenia-canary/pull/1210) distingue estabilidad del reloj de potencia del renderizador. Son patrones transferibles, no una lista de presets universales.
