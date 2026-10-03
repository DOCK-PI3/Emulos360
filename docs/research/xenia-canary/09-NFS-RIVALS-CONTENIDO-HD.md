# 09 — Rivals: instalación HD, GOD y error de lectura

Fecha: **2 de octubre de 2026**. Investigación del error mostrado por el usuario;
se leyeron capturas, registros, archivos y código. No se inició el juego ni se
instaló contenido, modificó el motor o cambió la configuración.

## Diagnóstico actual

**Actualización tras la prueba del usuario de las 11:12:** el HD y el TU ya
están instalados en BETA12, el juego reconoce el HD y el registro confirma que
aplica los parches a cuatro módulos. Ahora queda detenido en una pantalla negra
de carga sin el diálogo de lectura ni la salida con código 1. La carencia inicial
ha quedado superada; el bloqueo posterior sigue sin causa individual demostrada.
Véase [la nueva fase de carga](#fase-posterior-carga-detenida-tras-instalar-hd-y-tu).

**Prueba posterior sin red:** el usuario confirma el mismo bloqueo en el mismo
punto. La comparación ya se realizó; no volver a proponerla como pendiente.
Reduce el peso de una conexión externa como explicación, sin identificar aún
la llamada que impide avanzar. La prioridad de investigación pasa a la carga y
sincronización del core, manteniendo I/O, contenido y perfil como posibilidades
que todavía deben distinguirse con una traza.

En el primer intento había una carencia comprobada: **los paquetes estaban en la biblioteca y en
Descargas, pero no estaban instalados en el almacenamiento de BETA12**. La primera
captura del juego pide instalar contenido HD; no es una solicitud de Title Update
ni una conexión Xbox Live. El primer paso propuesto es instalar el HD con el
gestor de contenido. Todavía no se ha demostrado que eso permita gameplay en
esta build; existe además evidencia upstream de problemas del core en Rivals.

El icono naranja viene de `state-menus`/`Loads`, con etiqueta
`requires_hdd_install`, en el [issue #561](https://github.com/xenia-canary/game-compatibility/issues/561).
No identifica la causa individual ni garantiza que instalar un TU lo vuelva jugable.

## Archivos comprobados

Se inspeccionaron `D:/XBOX360_GAMES/454109C6` y
`C:/Users/mabe8/Downloads/454109C6`. **Los 40 archivos de ambas carpetas
coinciden en tamaño y SHA-256**, incluido el GOD, sus fragmentos y los cuatro
paquetes de contenido. No hay archivos adicionales en Descargas respecto a la
copia de biblioteca. Esto descarta una diferencia de transferencia entre ellas;
no valida la autenticidad ni todos los bloques internos del origen.

| Archivo relativo al directorio `454109C6` | Nombre en cabecera | Tamaño | Tipo |
| --- | --- | --- | --- |
| `00007000/623722192039774214FA` y su `.data` | Need for Speed Rivals | Cabecera 45.056 bytes; datos fragmentados | Juego GOD `00007000` |
| `00000002/8182BAAB13C42A06A387A9206AFF06435BCF762745` | Need for Speed Rivals HD Content | 1.587.830.784 bytes | Contenido adicional `00000002` |
| `00000002/B9F776ADE88E7404FB1F54921DD5FA66BA604C5645` | Need for Speed Rivals Update | 391.356.416 bytes | Contenido adicional `00000002`, distinto del TU |
| `00000002/E41FE29A5D7BD82B07D7152E4614A9209CDD954145` | Super Pack v5 | 90.112 bytes | Contenido adicional `00000002` |
| `000B0000/tu00000004_00000000` | Need for Speed Rivals Title Update | 17.514.496 bytes | Actualización `000B0000` |

Todos declaran Title ID `454109C6`. El GOD y el TU declaran Media ID
`603CD4F9`, coincidente también con los módulos del registro. Es compatibilidad
de identificación; no prueba que las firmas internas de parche y XEX coincidan
o que el TU llegue a aplicarse. No se debe deducir el catálogo de DLC de
«Super Pack v5» únicamente por su nombre.

La revisión inicial no mostró el TU en la salida parcial; la inspección completa
confirmó que **también está en la biblioteca**. La conclusión definitiva es
ausencia de instalación en el almacenamiento del motor, no ausencia del archivo TU.

## Almacenamiento y registro del primer intento

El registro disponible en
`C:/Users/mabe8/Desktop/Emulos360-BETA12/data/core.log` identifica:

```text
Build: codex/netplay-integration@6dbaa1f on Oct 1 2026
Storage root: C:\Users\mabe8\Desktop\Emulos360-BETA12\data\engine
apply_title_update = true
allow_incompatible_title_update = false
```

No existe un directorio de Rivals (`454109C6`) bajo los perfiles de `content`
ni de `external-content` en esa raíz. La raíz de contenido común contiene
otros títulos, pero no Rivals. La interfaz se ejecuta desde BETA12, según la
captura, y no desde el `out/Emulos360` del repositorio: hay que usar el contenido
de la misma instalación al comprobarlo.

Últimas operaciones registradas:

```text
StfsContainerDevice::ResolvePath(content\0000000000000000\454109C6\00000002)
ResolvePath(UPDATE:) failed - device not found
StfsContainerDevice::ResolvePath(content\0000000000000000\454109C6\00000002)
```

El juego busca el directorio de contenido dentro de su dispositivo de juego.
Un paquete suelto en un directorio hermano del GOD no se convierte por ello en
un archivo dentro del disco virtual. La consulta a `UPDATE:` tampoco encuentra
un dispositivo montado. El registro no contiene una traza completa con el
resultado de cada I/O, por lo que no se atribuye todo el fallo a una única llamada.
No hay evidencia de aplicación efectiva del TU en este intento.

`KernelState::FindTitleUpdate` enumera el contenido instalado de tipo installer;
`LoadTitleUpdate` abre el paquete como `UPDATE`, busca el `.xexp` relativo al
módulo y verifica su firma antes de aplicarlo. `apply_title_update=true` no
significa que cualquier TU que esté junto al GOD se descubra automáticamente.

`ContentManager::ListContent` busca en
`content_root/xuid/title_id/type_name/`. El gestor del producto
(`app/game_content.cpp`) instala paquetes mediante el core en esa raíz. El
importador de juegos (`app/game_importer.cpp`) mueve GOD/XBLA y sus datos;
no instala automáticamente los paquetes HD/DLC/TU como contenido del motor.

## Por qué acaba con error 1

La captura muestra el diálogo **Disc Read Error**. En el código local,
`XamShowDirtyDiscErrorUI_entry`, en `kernel/xam/xam_ui.cc`, muestra ese mismo
texto y después ejecuta `exit(1)`. La aplicación informa el código de salida.
Por tanto, esa secuencia es consistente con la salida explícita por el error
de lectura comunicado por el juego. El código 1 no identifica corrupción del
GOD ni demuestra un fallo GPU o una dependencia de Netplay.

Los avisos de ordinales de `Race.Splash` aparecen antes y no se han demostrado
como desencadenante. El porcentaje de imports implementados tampoco es una
garantía de comportamiento correcto de las funciones de I/O.

## Evidencia upstream y límites

El [reporte Canary #561](https://github.com/xenia-canary/game-compatibility/issues/561)
describe el mismo error al intentar instalar automáticamente las texturas.
También informa un bloqueo al intentar cargar después de instalarlas manualmente.
En una [actualización de agosto de 2026](https://github.com/xenia-canary/game-compatibility/issues/561#issuecomment-5150951534),
el autor señala funcionamiento en Edge `34357e2` y fallo en Canary `7010c86`.
Esto demuestra diferencia reportada entre ramas; no valida nuestro fork Netplay
`6dbaa1f` ni justifica sustituirlo sin estudiar sus cambios.

La otra copia que antes no cerraba el juego no se ha identificado ni comparado
como versión histórica distinta. Las dos carpetas actuales contienen la misma
copia. Una diferencia de empaquetado, contenido HD o almacenamiento usado puede
explicar otro resultado, pero sigue siendo hipótesis hasta comparar esa base.

## Pruebas propuestas después del primer intento

1. Cerrar la sesión del juego. En la biblioteca de **BETA12**, seleccionar
   Rivals → **Contenido** → **Elegir paquete del PC**.
2. Elegir
   `C:/Users/mabe8/Downloads/454109C6/00000002/8182BAAB13C42A06A387A9206AFF06435BCF762745`,
   identificado como **Need for Speed Rivals HD Content**, e instalarlo.
   Comprobar que figure en «Contenido instalado» de esa instalación.
3. Probar de nuevo y conservar el log: verificar si desaparece la instalación HD
   y localizar el siguiente error. Esa prueba cambia una variable y establece
   una base antes de añadir más contenido.
4. Instalar el TU mediante la misma pantalla usando
   `C:/Users/mabe8/Downloads/454109C6/000B0000/tu00000004_00000000`.
   Confirmar en log que carga/aplica el parche, no solo que el gestor lo lista.
   Añadir otros paquetes según lo que necesite el título y comprobar cada resultado.
5. Si continúa el fallo, estudiar la operación I/O/paquete/módulo concreta en
   esa fase y las diferencias de ramas reportadas. No forzar actualizaciones
   incompatibles ni omitir el diálogo de lectura como supuesta reparación.

Durante el primer diagnóstico estas pruebas quedaron propuestas. El usuario
realizó después la instalación del HD y del TU, comprobada en la fase siguiente.
El agente no instaló contenido ni ejecutó el juego.
No se prescribe descargar otra copia ni tocar el servidor Netplay para resolver
este primer error de instalación local.

## Fase posterior: carga detenida tras instalar HD y TU

El usuario aportó `Captura de pantalla 2026-10-02 111256.png` y confirmó que
ahora permanece en carga, sin error ni salida del emulador. Se inspeccionó y
conservó el registro actualizado; no se inició otra sesión ni se cambió la actual.

Comprobaciones locales:

- Hay dos paquetes instalados bajo
  `data/engine/content/0000000000000000/454109C6`: **HD Content** y **Title Update**.
  Sus tamaños y SHA-256 coinciden con los de la biblioteca. No están instalados
  los otros dos paquetes del bundle en esa raíz.
- `XamContentCreateEnumeratorInternal` incorpora **Need for Speed Rivals HD Content**
  al resultado de enumeración: ya no es solo un archivo presente en Descargas.
- El core carga `Default.xexp`, `Race.Splash.xexp`, `Race.Main.xexp` y
  `Engine.BuildInfo.xexp`. En los cuatro registra parche aplicado correctamente,
  de versión base **0.0.0.4** a **0.0.2.4**. La actualización ya no está pendiente
  de descubrir o aplicar en este intento.
- Se inicializan los workers `JobManager`, se registra audio y se resuelven rutas
  de `Data/Xenon/Levels/FrontEnd` y `Data/Xenon/Levels/LoadingScreenHost`.
  La última ruta registrada es `LoadingScreenHost`; no equivale a demostrar que
  ese archivo sea defectuoso, falte o sea la causa del bloqueo.
- La sesión usa **`network_mode=2`**. Inicializar WSA o crear un hilo de presencia
  no demuestra que la carga esté esperando un servidor. No se observó evidencia
  suficiente para atribuir esta pantalla a un endpoint remoto concreto.
- Una muestra de aproximadamente tres segundos conservó el log en **129.561 bytes**
  y registró **6,921875 segundos de CPU agregados** adicionales del proceso.
  El core seguía activo; esto no distingue entre una espera activa, compilación,
  trabajo de carga u otros bucles. No permite diagnosticar un deadlock completo.

Los mensajes `RtlRaiseException(...406D1388)` corresponden a nombrar hilos,
según `xboxkrnl_debug.cc`; no son por sí solos excepciones fatales. El listado
de imports contiene funciones sin implementar y hay avisos de exports, pero
su aparición no demuestra cuál impide avanzar. No convertir un import listado
en una llamada efectivamente ejecutada durante el bloqueo.

El síntoma encaja con el reporte de Canary: carga que no progresa después de
instalar manualmente las texturas. Esto apoya investigar compatibilidad del core,
pero no establece una causa única para el GOD de este usuario.

### Pista de comparación con Edge

El [comentario de agosto](https://github.com/xenia-canary/game-compatibility/issues/561#issuecomment-5150951534)
reporta funcionamiento en Edge `34357e2`. El
[commit citado](https://github.com/has207/xenia-edge/commit/34357e2) activa por
defecto `guest_scheduler`: planificación cooperativa de hilos guest mediante
fibers. La base Netplay local inspeccionada no contiene esa opción en
`kernel_flags.cc` ni una implementación localizada por esa búsqueda.

Es una diferencia concreta para estudiar en scheduling/sincronización. **No
demuestra que ese commit, por sí solo, arregle Rivals**: la build reportada contiene
otros cambios anteriores. Añadir `guest_scheduler=true` a la configuración de
nuestro motor no implementaría ese subsistema. No se propone sustituir el motor
de Emulos360 o portar solo el cambio de valor por defecto.

### Siguiente comparación útil

El usuario ya probó **sin red** y comunicó: «pasa lo mismo en el mismo sitio,
se queda cargando». Registrar este resultado como **prueba local reportada por
el usuario**, distinta de la sesión de modo 2 comprobada en el registro.
Al revisar BETA12, `core.log` y `core-console.log` seguían siendo los archivos
de las 11:12:45, de 129.561 bytes; no había un nuevo registro identificable de
esa prueba. Las configuraciones leídas conservaban `network_mode=2`. Esto no
invalida su observación; el mecanismo de desconexión y el modo emulado efectivo
del nuevo intento no están documentados. Desconectar la red del PC y cambiar
el estado Live del perfil emulado son operaciones distintas. No marcar modo 0
como verificado a partir del registro anterior ni concluir que toda la capa XAM
de red queda excluida.

La siguiente prueba que puede identificar una causa es capturar stacks/PC de los hilos guest y actividad de I/O/JIT en la fase
detenida. Comparar una revisión Edge fijada con el mismo contenido y configuración
controlada, en almacenamiento separado, serviría para aislar diferencias del core.
Tampoco se ha realizado esa prueba. No añadir más DLC ni repetir la instalación
como respuesta automática a una fase que ya reconoce HD y aplica el TU.

Los [datos de esta fase](nfs-rivals-carga-evidencia-20261002.json) y los logs
`core-carga-1112.log` / `core-console-carga-1112.log` conservados en `local/`
complementan la evidencia inicial sin sobrescribirla. La compatibilidad continúa
en **carga sin gameplay confirmado**.

### Comparación estática ampliada de Edge

Se resolvió la revisión reportada a
**`34357e25782dca8214b171fd19cf6889139c442a`**, del 1 de agosto de 2026.
Se consultaron su árbol, 100 commits que afectan al kernel, cinco diffs
seleccionados y seis archivos de esa revisión. Es una inspección acotada;
no es una comparación exhaustiva de todas las diferencias de Edge/Netplay ni
una prueba de Rivals ejecutado en Edge.

El código local `xthread.cc:419` crea un hilo host para ejecutar el hilo guest;
`xobject.cc:205` delega la espera al primitivo host. Edge añade una ruta de fibers
para hilos guest: seis hilos de despacho, uno por CPU lógica emulada, colas por
prioridad y puntos de interrupción en el JIT. Los hilos host internos conservan
su ruta propia. Véanse
[XThread](https://github.com/has207/xenia-edge/blob/34357e25782dca8214b171fd19cf6889139c442a/src/xenia/kernel/xthread.cc#L437)
y [GuestScheduler](https://github.com/has207/xenia-edge/blob/34357e25782dca8214b171fd19cf6889139c442a/src/xenia/kernel/guest_scheduler.h#L29).

| Revisión inspeccionada | Cambio concreto | Límite para Rivals |
| --- | --- | --- |
| [46dca60a](https://github.com/has207/xenia-edge/commit/46dca60a53f765849317a66de0d5b57ac8833ea4) | Introduce el scheduler y la ruta fiber de XThread; su estado inicial usa un solo hilo host | Es el inicio de una serie, no la implementación final de la build reportada |
| [c159d855](https://github.com/has207/xenia-edge/commit/c159d855de7c99dc6494c1a20bb85cd4ec5980ea) | Añade interrupción del JIT y esperas cooperativas para objetos, archivos, sockets y completado de I/O; desplaza llamadas host bloqueantes a un worker | Afecta a 47 archivos y atraviesa CPU/kernel/I/O; no es un parche aislado de configuración |
| [e268e113](https://github.com/has207/xenia-edge/commit/e268e113be17257ff9d867a49928b8589cdb4ce4) | Cambia los safepoints a una bandera por contexto que solicita ceder ejecución | Debe conservarse junto con el scheduler, la estructura PPCContext y los backends JIT |
| [65c0ff70](https://github.com/has207/xenia-edge/commit/65c0ff7000daf244e7b5fa708209a43eb0763653) | Registra una generación de pulso para liberar waiters cooperativos de eventos manual-reset aunque el primitivo host ya se haya reseteado | Corrige la ruta cooperativa de Edge; no demuestra que el evento que espera Rivals pierda una señal |
| [fb53ac24](https://github.com/has207/xenia-edge/commit/fb53ac24cebd662c12c97711f3a2352881f39450) | Impide que un waiter posterior consuma la señal auto-reset reservada al primero | El mensaje del commit atribuye la mejora a Soul Calibur IV, no a Rivals |

En la revisión final, el watchdog y el despertar de un hilo prioritario pueden
solicitar interrupción. La entrega se difiere si se mantiene la región crítica
global o si el IRQL guest es al menos 2. Las llamadas de I/O desplazadas usan
un worker serializado y despiertan al solicitante cuando terminan. Estos detalles
explican por qué portar únicamente la opción, un evento o la primera versión del
scheduler sería incompleto. No se midió rendimiento ni estabilidad de esta ruta
en Emulos360.

**Hipótesis prioritaria:** comparar la progresión de los workers JobManager y del
hilo principal durante carga. Una diferencia de planificación o señalización
puede permitir que un trabajo pendiente finalice en otra rama. Para demostrarlo,
hay que identificar PC guest repetidos, el objeto de espera y el hilo que debe
señalarlo. Si esos datos muestran una lectura incompleta, un callback de audio,
un fence GPU o una función XAM distinta, dirigir la investigación a esa ruta.
El spinner y el consumo de CPU no bastan para elegir entre ellas.

Una comparación futura debe fijar GOD, HD, TU y hashes; usar el mismo backend
gráfico y perfil equivalente con almacenamiento separado; conservar logs; y
alcanzar una carrera controlable. Si Edge progresa, comparar además scheduler
activado/desactivado en esa misma revisión para aislar esa variable. Si solo
cambia la build completa, investigar otras diferencias. No sustituir el core de
producción ni prometer compatibilidad o Netplay basándose en el reporte público.

El [registro de esta ampliación](nfs-rivals-edge-evidencia-20261002.json) conserva
las fuentes con hashes, el resultado sin red reportado y las diferencias
estáticas. Las respuestas completas están en `local/nfs-rivals-edge-20261002/`.

## Evidencia reproducible

El [resumen y hashes](nfs-rivals-evidencia-20261002.json) conserva las cabeceras,
comparación de archivos, hashes de los registros y las consultas públicas.
Los registros completos y respuestas API quedan en
`local/nfs-rivals-audit-20261002/`, fuera de la documentación publicable.
Scripts de investigación: `local/nfs-rivals-audit.py` y `local/nfs-rivals-fetch.py`.
