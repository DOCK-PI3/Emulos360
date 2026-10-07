# 13 — Mejora de CPU reproducida y validación antes de incorporarla

Fecha: **6 de octubre de 2026**. El usuario solicita trabajar ahora en el core,
sin esperar al seguimiento de los lunes, estudiando sus contratos y conservando
compatibilidad, gráficos, estabilidad y Netplay. El seguimiento semanal queda
como complemento. No se añade otro core ni se retoma XBLIG o Rivals.

## Recorrido estudiado para esta primera mejora

El frontend PowerPC convierte instrucciones en HIR. Las pasadas del compilador
pueden evaluar operaciones con operandos constantes antes de generar código
x64. `ConstantPropagationPass` llama a `Value::Extract` cuando el vector y el
índice son constantes. Si los operandos siguen dinámicos, el backend emite
`vpextrb`/`vpextrw`/`vpextrd` con la organización de elementos de `VEC128_B/W`.

Ambos recorridos deben seleccionar el mismo elemento. Un cálculo JIT más rápido
que cambia los datos del juego es un defecto de emulación. Revisar esta frontera
evita confundir geometría errónea producida por CPU con un fallo de texturas GPU.

Fuentes locales estudiadas: `cpu/hir/value.cc`, `cpu/hir/value.h`,
`cpu/compiler/passes/constant_propagation_pass.cc`, `cpu/hir/hir_builder.cc`,
`cpu/backend/x64/x64_seq_vector.cc` y `base/vec128.h` bajo `engine/xenia/src/xenia`.
Se leyeron primero los capítulos [02](02-GRAFICOS.md),
[03](03-CORE-RENDIMIENTO-ESTABILIDAD.md), [05](05-GEARS-Y-CASOS.md) y
[10](10-SEGUIMIENTO-Y-MEJORAS-DEL-CORE.md), además de `AGENTS.md`.

## Defecto confirmado: extracción de vectores constantes

La base Netplay `6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c` todavía contiene:

- Selección de bytes y shorts que no coincide con el orden del backend.
- Una máscara `0x1F` para un array de **16 bytes**: el índice 16 accede fuera
  del array. El índice HIR es I8; sus valores se deben ajustar al número de
  elementos, como ocurre en las instrucciones de extracción del host.
- Lectura de miembros de índice más anchos que I8 en los otros tamaños.

La corrección oficial de Canary
[`5ce7c336df060d043b0a2351889ec8789322d035`](https://github.com/xenia-canary/xenia-canary/commit/5ce7c336df060d043b0a2351889ec8789322d035),
de **Herman S.**, ajusta cuatro expresiones de `Value::Extract` y añade un
comentario. Conserva los cambios de Netplay y no cambia flags, defaults,
scheduler, memoria, XAM, shaders o servicios de sesiones.

El mensaje upstream atribuye errores visuales de FIFA y State of Decay a esta
interacción con promoción de contexto vectorial. Es un **reporte upstream**;
no se ejecutaron esos juegos aquí ni se atribuye a esta corrección el cierre
gráfico de Gears 3.

Se inspeccionó la implementación presente en Canary y la comparación desde el
commit hasta su rama actual: conserva la corrección y no aparece un revert en
los mensajes de los 40 commits de la comparación consultada. Esto no es una
auditoría exhaustiva de cada cambio posterior. Netplay consultado permanece en
la revisión fijada. Las revisiones consultadas constan en la evidencia.

## Implementación preparada

El [parche reducido](../../../patches/xenia-upstream-candidates/cpu-extract-fold.patch)
contiene el hunk de implementación oficial. No importa el conjunto amplio de
tests y otros arreglos de backend de esa serie. La atribución y fuente original
se conservan; el parche completo descargado queda en `local/`.

El [script de preparación](../../../scripts/prepare-core-extract-candidate.ps1)
crea una copia aislada y comprueba revisión, contexto y procedencia. No modifica
la fuente activa `engine/xenia/src/xenia/cpu/hir/value.cc` ni sustituye el
ejecutable usado para jugar. Tampoco se conecta a `prepare-engine.ps1` ni a una
release. El [banco reproducible](../../../tests/core-semantics/README.md) permite
volver a demostrar el antes/después.

## Evidencia automatizada del 6 de octubre

Se ejecuta la función original del core con cinco vectores conocidos y 4096
vectores deterministas. El oráculo construye los elementos a partir de palabras
guest mediante desplazamientos, de forma independiente del código corregido.

| Prueba | Core sin corregir | Candidato aislado |
| --- | --- | --- |
| 123030 extracciones con índices válidos | 98108 resultados incorrectos: 65316 I8 y 32792 I16; I32/I64 sin diferencias | 0 resultados incorrectos |
| Índice 16 en I8 bajo UBSan | `index 16 out of bounds for uint8_t[16]` en `value.cc:834` | Sin errores |
| 4199424 casos, todos los índices I8 y cuatro tamaños | No aprobado; Linux se detiene ante el acceso inválido | 0 resultados incorrectos |
| Windows, MSVC 14.50, C++20, Release | Reproduce el error de elementos válidos | 2/2 tests y ambos recorridos completos pasan |
| Ubuntu, Clang 21.1.8, ASan/UBSan, leaks habilitados | Reproduce error semántico y acceso inválido | 2/2 tests pasan sin informes de sanitizers |

En Linux se compila el archivo `value.cc` completo. En Windows también se
compila completo; el ejecutable de tests enlaza el cuerpo exacto de `Extract`
extraído de ese mismo archivo, debido a la resolución de símbolos no usados
de MSVC. No se suministran implementaciones ficticias de otros subsistemas.
Esas pruebas de componente no ejecutan el JIT. El control I64 tampoco demuestra
soporte de EXTRACT_I64 en x64.

### Prueba adicional: compilador y JIT reales

Se enlazó `xenia-cpu-tests` del fork completo con MSVC Release, C++20 y LTCG.
El [test diferencial](../../../tests/core-semantics/extract_fold_contract_test.cc)
construye en una misma función un `Extract` de vector constante y otro de un
registro vectorial, atraviesa las pasadas HIR y ejecuta el código generado.
Con tres vectores y todos los elementos válidos de 8/16/32 bits son **84
funciones generadas y 252 assertions**: pasan las dos rutas y el oráculo.

Además pasa toda la batería existente de CPU, incluida esta regresión:
**247 casos y 1077 assertions**. No es la batería de GPU, XAM o Netplay.
No se ejecutó el banco JIT con sanitizers ni se valida EXTRACT_I64 dinámico.

El test necesitó registrar `emulos_embedded=false` igual que la app, porque
nuestro XAM declara el flag y el ejecutable de tests no enlaza la ventana.
Ese soporte usa el registro real de cvars y solo se compila para tests.
No se reemplazan funciones de CPU/memoria/kernel.

El [script reproducible](../../../scripts/build-core-extract-candidate.ps1)
audita la copia y ejecuta ambos bancos. Se comprobaron **2029 archivos** de la
fuente activa contra la copia, sin diferencias, exceptuando el `Value.cc`
corregido y bytecodes regenerados de shaders. Las dos fuentes de tests son
adiciones de la copia. El fork ya tenía cambios propios: esta build no es un
checkout Netplay limpio ni debe compararse con una instalación antigua
atribuyendo todas sus diferencias al hunk de CPU.

### Emulador experimental completo

Se compiló y enlazó `xenia-app` con los backends y subsistemas del fork actual,
en Windows x64 Release. Artefacto aislado:
`local/core-maintenance-20261006/jit-windows-build/bin/Windows/Release/Emulos360-core.exe`.
SHA-256: `df826f206bd2bd04a704d0cf1de914db5c7a47c04003b3b172f54107b08ee0a5`.
El enlace completo también pasa al repetir el procedimiento reproducible.
No se ha iniciado ese ejecutable con juegos ni se considera una release.

La fuente activa `value.cc` sigue con SHA-256
`4e4bb8cc81baf7c07c3180377fb6901f457c1a0d4a5def8756e31737b1ef31a2`.
El ejecutable de la instalación usada para jugar sigue con SHA-256
`63f44fb5cfca5ba3a20a4e9475572a8e81839efb4930bc222a6285cb589b2569`.
Perfiles, partidas, contenido y ajustes no se modifican.

Hashes, herramientas, resultados y límites están en
[la evidencia](core-cpu-extract-evidencia-20261006.json). Los logs completos y
objetos de pruebas quedan en `local/core-maintenance-20261006/` y
`/var/tmp/emulos360-core-extract-20261006/`.

## Qué falta para incorporarlo al motor estable

1. Completar la validación del emulador experimental en Windows/Linux. La
   equivalencia HIR constante/dinámico de esta operación ya pasa en el JIT x64
   Windows; falta validar su efecto en contextos de juegos reales y una build
   completa Linux. No activar promoción de contexto globalmente para probarlo.
2. Repetir controles que ya funcionan: Gears 3 y Borderlands 2/The Pre-Sequel,
   con la misma versión/TU, ajustes, contenido y escenas. Registrar sesiones
   completas de Netplay entre equipos cuando corresponda. Incorporar un FIFA
   o State of Decay para el caso visual reportado, si están disponibles.
3. Comparar imagen, comportamiento y fallos con el baseline; medir frametimes
   si se afirma un efecto de rendimiento. Pasar el test de una función no
   garantiza la biblioteca completa.
4. Solo tras esa evidencia, incorporar el hunk aceptado al parche reproducible
   del fork y reconstruir Windows/Linux. Conservar el ejecutable previo.

Estado: **causa reproducida, arreglo preparado, componente y JIT x64 Windows
aprobados; no integrado al motor estable y gameplay pendiente**. No se afirma un aumento
de FPS, más títulos jugables ya instalados o una garantía de cero regresiones.

El emulador experimental completo Windows ya enlaza. La ejecución de juegos
y la compilación completa Linux siguen pendientes; no hay una versión estable
nueva o un paquete de actualización publicado.

## Siguientes áreas de estudio

- Cachés y vida de recursos GPU: los candidatos de mutex y barreras Vulkan
  del capítulo 10 siguen pendientes de ejecución GPU; no reparan por sí mismos
  una sesión D3D12.
- Viewports y tiling: validar la corrección de coordenadas negativas con pruebas
  de límites y consumidores D3D12/Vulkan antes de tocar el código compartido.
- Igualdad vectorial float: estudiar además del cero con signo y NaN los modos
  MXCSR/VMX y los denormales. El backend cambia de modo; no dar por equivalente
  una comparación C++ bajo el estado flotante del compilador.
- Pérdida del dispositivo gráfico: obtener HRESULT/DRED y recursos en vuelo
  antes de proponer una corrección al cierre de Gears 3. El registro del
  6 de octubre confirma D3D12/RTV inferido de la selección, RX 6600 XT,
  resolución 1x y FSR/FXAA desactivados. El mensaje genérico no prueba una
  expulsión por inactividad ni identifica qué recurso o shader falló.

Estas áreas son trabajo propuesto, no nuevas mejoras integradas. Cada iteración
debe terminar con una causa, un cambio acotado, pruebas y evidencia de sus límites.
