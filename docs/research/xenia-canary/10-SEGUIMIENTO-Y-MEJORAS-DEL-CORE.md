# 10 — Seguimiento y mejoras del core

Fecha: **2 de octubre de 2026**. El usuario solicita mejoras generales y revisar
periódicamente Canary para incorporar avances. Su prioridad explícita es
**conservar la compatibilidad de los títulos que ya funcionan**. Rivals y el
scheduler amplio de Edge permanecen aplazados.

Esta fase prepara cambios y comprueba su contexto y compilación en una copia
aislada. No se ha aplicado ningún cambio al checkout activo del core, sustituido
el ejecutable de BETA12, ejecutado juegos o modificado configuración, perfiles,
partidas ni contenido. No afirmar una mejora de FPS o compatibilidad ya instalada.

## Estado actual comprobado en GitHub

| Rama | HEAD consultado | Resultado frente al dossier inicial |
| --- | --- | --- |
| [Canary canary_experimental](https://github.com/xenia-canary/xenia-canary/tree/canary_experimental) | `b083312b8b18e07e6e410b82104191f126722794`, 02-10-2026 05:44:06 UTC | Dos commits posteriores a `44f5b4a86080189047e9a60f2d2b8041796fcb68` en la lista inspeccionada |
| [Netplay netplay_canary_experimental](https://github.com/AdrianCassar/xenia-canary/tree/netplay_canary_experimental) | `6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c`, 10-09-2026 | Coincide con la revisión base local |

Se consultaron los doce commits recientes de cada rama y los diffs de cuatro
candidatos. Esta revisión no vuelve a auditar todos los cambios históricos ni
todos los PR abiertos. Los hashes de los capítulos iniciales siguen describiendo
su consulta original; esta ficha registra el avance posterior.

## Primeros candidatos de estabilidad

### 1. Protección de tablas durante la carga paralela de shaders Vulkan

[Canary 1b242658](https://github.com/xenia-canary/xenia-canary/commit/1b242658e535fbd12645e4d2c12693771075b738)
añade un `lock_guard` sobre un mutex ya declarado alrededor del acceso a las
tablas compartidas de layouts. El reporte upstream describe corrupción de
memoria al precargar shaders desde varios hilos. Se conserva la traducción
paralela; no se altera el scheduler guest, CPU, perfiles o protocolo Netplay.

El cambio afecta a **un archivo, tres líneas añadidas**. Encaja en la base local
y el archivo modificado compila con las opciones Release de nuestro build
MSVC. La compilación se realizó en `local/core-maintenance-20261002/vulkan-candidate/`.
Los hashes de las fuentes activas antes y después coinciden.

**Aceptación pendiente:** enlazar una build de prueba, repetir cargas con caché
de shaders existente y traducción paralela, comprobar integridad y cierre, y
comparar juegos ya funcionales en Vulkan. Repetir también un control D3D12 en la
build integrada. La prueba upstream de corrupción y la compilación local no
demuestran que un juego haya dejado de fallar aquí. No se midió rendimiento.

### 2. Mantener la barrera entre dos escrituras a una textura Vulkan

[Canary c8f214e2](https://github.com/xenia-canary/xenia-canary/commit/c8f214e2a9b0209ca8a0ad6cb3dfb155fe545ae2)
evita que una barrera entre uploads consecutivos se descarte por tener iguales
layouts y máscaras de origen y destino. Aunque sus valores sean iguales, hay
dos escrituras que ordenar. Es **un archivo, cuatro líneas añadidas y dos
retiradas**. Encaja y su unidad de compilación pasa en la misma copia aislada.

**Aceptación pendiente:** ejecutar un caso de uploads repetidos al mismo recurso
en una submission, inspeccionar la dependencia con validación/captura Vulkan y
comparar imagen, cierre y tiempos de frame. Mantener una prueba de juegos ya
funcionales. No se han ejecutado GPU, sanitizers ni gameplay durante esta fase.

Ambos candidatos mejoran contratos concretos de estabilidad en Vulkan; no
prometen más FPS en D3D12 ni arreglar cualquier juego. Sus beneficios dependen
de que la ejecución atraviese las rutas corregidas.

## Dos novedades gráficas de hoy

### D3D12: no descartar determinados draws por el tipo del fetch

[Canary b083312b](https://github.com/xenia-canary/xenia-canary/commit/b083312b8b18e07e6e410b82104191f126722794)
añade una excepción condicionada por `gpu_allow_invalid_fetch_constants`.
El autor reporta modelos de personajes ausentes en `5454086C` y `425607FE`,
identificados como **L.A. Noire** y **Cars 2** en el catálogo local.
Es **un archivo, nueve líneas añadidas**; el parche encaja.

En nuestra fuente `gpu_flags.cc`, esa opción tiene default declarado **true**;
no asumir que el parche quedaría inactivo por defecto. Hay que leer además la
configuración efectiva. Se amplía una tolerancia a constantes de tipo inválido,
sin demostrar el origen del dato incorrecto. Priorizar una prueba con la opción
activada/desactivada y otros consumidores del draw. No activar nuevos ajustes
globales a partir del título del commit. Pendientes compilación y ejecución.

### Viewport: permitir un origen negativo

[Canary 6579cc2a](https://github.com/xenia-canary/xenia-canary/commit/6579cc2a4c8812a101c46fdae2e21df34a8521fd)
cambia el cálculo y el tipo de `ViewportInfo::xy_offset` a entero con signo para
conservar un origen negativo. El reporte describe geometría deformada en una
parte de la imagen de **Blue Dragon**, con tiling y desplazamiento de ventana.
El parche afecta a `draw_util.cc` y `draw_util.h` y encaja en ambos.

Es código compartido por backends. Validar consumidores del tipo con signo,
offsets negativos y positivos, límites, viewport vacío, media muestra y escalado
a 1x/2x en D3D12 y Vulkan. Pendientes compilación y ejecución. No convertir el
reporte de un caso en una mejora general demostrada.

Los tres Title ID específicos anteriores no figuran en el inventario de 80
títulos capturado el 2 de octubre. Esto limita la reproducción inmediata de
esos casos con la biblioteca evaluada; no impide investigar el contrato del core.

## Material preparado y límites de la validación

Los [cuatro parches reproducibles](../../../patches/xenia-upstream-candidates/README.md)
conservan atribuciones y contenido exacto descargado. **No están conectados al
script de preparación del motor ni a la actualización del producto.**

Cada parche pasa `git apply --check` en el árbol actual y falla en la comprobación
inversa. Dos archivos Vulkan parcheados compilan con MSVC `14.50.35717`, C++20 y
las opciones Release de `impl-Release.ninja`, incluidos `/O2` y `/WX`. Es
compilación de dos unidades, **no enlazado de un core completo ni prueba de
concurrencia o de imagen**. La copia de investigación solo contiene las dos
fuentes necesarias y usa los headers actuales del proyecto.

El ejecutable `Emulos360-BETA12/engine/Emulos360-core.exe` mantiene SHA-256
`63f44fb5cfca5ba3a20a4e9475572a8e81839efb4930bc222a6285cb589b2569`, igual al
diagnóstico anterior. El [registro de evidencia](core-seguimiento-evidencia-20261002.json)
guarda hashes de fuentes, parches, consultas, logs y resultados. Las respuestas
completas y objetos compilados quedan en `local/core-maintenance-20261002/`.

## Método de incorporación

1. Identificar una causa y una unidad coherente; comprobar dependencias y reverts.
2. Preparar y compilar en una copia de prueba, conservando el motor estable.
3. Validar el caso afectado y otros consumidores del mismo contrato. Incluir
   títulos que el usuario ya haya confirmado funcionales, como Borderlands 2 y
   The Pre-Sequel, y su multijugador cuando la ruta modificada pueda afectarlo.
   No confundir esos controles con una prueba de toda la biblioteca.
4. Registrar imagen/comportamiento, errores, tiempos y regresiones, con el mismo
   contenido y configuración de referencia. Compilar Linux si la incorporación
   se distribuirá allí; la compilación Windows no lo valida.
5. Conservar el cambio aceptado en el parche reproducible del fork y comprobar
   una reconstrucción limpia. Integrar al producto solo con evidencia suficiente
   de compatibilidad; mantener una versión previa recuperable.

Las correcciones de HIR/x64 del capítulo 03 siguen como candidatos históricos.
Requieren pruebas diferenciales de semántica y regresión antes de incorporarse.
Los cambios amplios de ZPD/VIZ_QUERY y scheduler requieren una evaluación mayor.
Una revisión más nueva no sustituye ese análisis.

## Seguimiento periódico creado

Automatización **Mejoras de Xenia Canary**, id `mejoras-de-xenia-canary`, activa
en este chat: **lunes a las 10:00, hora local**. La frecuencia semanal es la
elección inicial para «de vez en cuando»; no se atribuye al usuario una hora
solicitada explícitamente.

Su tarea es comparar Canary y Netplay con la última revisión documentada,
investigar novedades y preparar cambios en una copia aislada. Debe consultar
primero este dossier, conservar el trabajo local y mantener aplazado Rivals.
Cuando falten pruebas de gameplay/regresión, el resultado es un candidato
documentado, no una actualización automática del motor estable. Avisará ante
una mejora concreta, una preparación completada, un fallo o una decisión
necesaria; sin novedades relevantes, permanecerá en silencio.

Este seguimiento necesita el equipo encendido, la aplicación abierta y el
proyecto accesible cuando se ejecute, según la
[documentación oficial de tareas programadas](https://learn.chatgpt.com/docs/automations?surface=app).
La tarea fue creada y se verificó su estado; no se ha observado todavía una
ejecución programada.
