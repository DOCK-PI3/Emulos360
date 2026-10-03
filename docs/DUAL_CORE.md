# Segundo core: Canary Edge (0.4.0)

Integración realizada el **3 de octubre de 2026** en el proyecto GitHub
`C:\Users\mabe8\Documents\GitHub\Emulos360`.

## Uso

- **Ajustes → Cores:** selecciona el core global y su motor gráfico.
- **Biblioteca → Motor:** fija core y motor gráfico para el Title ID seleccionado,
  o pulsa «Usar globales» para volver a heredar la selección global.
- Windows permite Direct3D 12 o Vulkan; Linux permite Vulkan. «Predeterminado del
  core» conserva la configuración nativa. Cada core conserva su selección gráfica
  global; cambiar uno no cambia la del otro.
- «Abrir ajustes de Edge» abre su interfaz nativa. Las otras secciones de Ajustes
  continúan configurando el core actual de Emulos360.
- Los cambios se aplican al siguiente inicio. La selección está bloqueada durante
  una sesión o una copia de contenido. No se cambia de core durante una partida.

La instalación y la actualización mantienen **Emulos360 · core actual** como
selección inicial. Edge es opcional, experimental y se abre en una ventana
independiente. En Windows, «Controlar juego» la enfoca y «Cerrar sesión» solicita
su cierre nativo. En Linux, el cierre de la barra envía SIGTERM; guarda antes de
cerrar y usa la propia ventana del juego para gestionar su foco y pantalla completa.

## Preservación de compatibilidad

El core actual no se sustituye, recompila ni recibe cambios de scheduler en esta
release. El ejecutable Windows conservado tiene SHA256
`63f44fb5cfca5ba3a20a4e9475572a8e81839efb4930bc222a6285cb589b2569`.
Sin una selección gráfica explícita, su ruta de lanzamiento mantiene los
argumentos anteriores. Esto reduce el riesgo de regresión del motor; no demuestra
por sí solo compatibilidad de gameplay de los 80 títulos de la biblioteca.

Netplay, MultiP, herramientas de perfiles, avatares y dashboard continúan utilizando
el core actual. El Edge distribuido no implementa la integración `--emulos_*` de
Emulos360. No se le pasan esos argumentos ni los de nuestra API de sesiones.
Intentar iniciar Edge dentro de una sesión MultiP produce un mensaje que pide
seleccionar el core actual. No hay cambio silencioso a otro ejecutable si falta el
core elegido. La interfaz de Logros y Partidas de Emulos360 consulta el core actual;
los avances de Edge se consultan en Edge o en su carpeta de contenido.

## Almacenamiento independiente

Todo se encuentra bajo la carpeta de datos elegida por el usuario:

| Ruta | Contenido |
| --- | --- |
| `cores.ini` | Core global, GPU por core y excepciones por Title ID |
| `cores/edge/xenia-edge.config.toml` | Ajustes nativos de Edge |
| `cores/edge/content/` | Perfiles, partidas, DLC y TU usados por Edge |
| `cores/edge/cache/` | Cachés de Edge |
| `cores/edge/core.log`, `core-console.log` | Diagnóstico de Edge |
| `cores/edge/imports/` | Marcadores de copia inicial por perfil y Title ID |

Al iniciar un título en Edge por primera vez se copian, en segundo plano, el perfil
seleccionado y su contenido de ese título desde las unidades interna y externa.
También se copia contenido público del título (DLC/TU). Solo se añaden archivos
que faltan. Las fuentes se leen y ningún archivo existente de Edge se sobrescribe.
Las carpetas enlazadas o solapadas se rechazan y se bloquea el almacenamiento
mientras se prepara la sesión. Una unidad externa desconectada no se recrea.

Después de la copia inicial, perfiles y partidas evolucionan por separado. Se
siguen incorporando archivos nuevos de DLC/TU, pero **no se sincronizan partidas,
logros ni paquetes reemplazados con el mismo nombre**. La aplicación no elimina
contenido antiguo de Edge. Para reemplazar deliberadamente un TU o DLC con el mismo
nombre, cierra ambos cores y gestiona la copia independiente desde «Partidas de
Edge». Conserva una copia de seguridad antes de reemplazar progreso.

La configuración inicial copia únicamente idioma y notificación de logros.
Después se conserva cualquier edición nativa. Los parches específicos, las
opciones de red y los ajustes gráficos detallados no se transfieren automáticamente.
Un juego que necesite un parche debe configurarse para la revisión de Edge y su
versión del ejecutable; no se da por válida una dirección de parche de otro core.

## Procedencia y construcción

Se distribuye el binario oficial de [has207/xenia-edge](https://github.com/has207/xenia-edge),
fijado a la revisión `0788c561e3689d23814224e95d6efc5b88fdc123`, etiqueta
[0788c56](https://github.com/has207/xenia-edge/releases/tag/0788c56).
`engine/edge-release.json` registra revisión, URLs, hashes y capacidades. No se
sigue automáticamente una rama mutable ni se modifica el binario de Edge.

- ZIP Windows oficial: SHA256
  `cdc9bc80ff5007ed367f8d16916899e219b7aa85c3fab6109404a39f667181b3`.
- AppImage Linux oficial: SHA256
  `bdb35f41b5acefcf9666b8c5accb85152e3c566b1f91ab308b4a78f85a02309f`.
- Licencia del blob Git original: SHA256
  `369ea6b0f7ba57544067e9d470ca82a927da787fb0a749b11cb55f1fd0ba47ae`.
  El checkout Windows puede convertir sus saltos de línea; se verifica el blob
  original para la descarga Linux, y el ZIP íntegro para Windows.

`scripts/setup-edge.ps1` y `scripts/setup-edge.py` descargan y verifican los paquetes
fijados y conservan su licencia y metadatos. Windows incluye también
`D3D12/D3D12Core.dll` de Agility SDK 1.619.3 y `D3D12/dxil.dll` de DXC
v1.9.2602.24, con los hashes exigidos por esa revisión de Edge y sus licencias.
La prueba inicial detectó el aviso de descarga cuando faltaba Agility; incluir
estos runtimes evita esa dependencia del primer arranque. No se distribuye el
SDK de depuración. Los scripts de construcción y
empaquetado incluyen ambos cores. El AppImage se inicia con
`APPIMAGE_EXTRACT_AND_RUN=1`, sin exigir un montaje FUSE. Las futuras actualizaciones
verifican que también se incluya Edge. Para cambiar su revisión, audita primero
código, argumentos, estructura de contenido y hashes oficiales; vuelve a ejecutar
la matriz de pruebas y documenta gameplay por título antes de recomendarlo.

## Evidencia y límites de la validación

- Compilación de la aplicación para Windows x64 (MSVC/Qt 6.8.3) y Linux x86_64
  (Ubuntu 26.04, Clang 21.1.8/Qt 6.10.2).
- Pruebas de preferencias, herencia por título, persistencia, bloqueo, aislamiento,
  copia sin sobrescritura y rechazo de rutas solapadas.
- Pruebas de servicios, biblioteca, voz e interfaz; revisión visual del selector
  global y del diálogo por juego.
- Prueba de lanzamiento con ejecutables instrumentados: helpers y Netplay al core
  actual; Edge con argumentos propios; rechazo de MultiP y de ejecutable ausente;
  GPU explícita sin duplicados.
- Windows: lanzamiento real de NEW RALLY-X (58410835) con perfil desechable.
  El registro confirma carga de `GAME:\default.xex`, creación del dispositivo
  Direct3D 12 y presentación de frames durante 15 segundos. Esto no acredita menú
  ni gameplay; la captura de ventana muestra la interfaz nativa y no valida por
  sí sola el contenido del swapchain.
- Windows: creación/listado de perfiles y ventana nativa del core actual; apertura,
  detección y cierre del Edge oficial desde PlayerServices.
- ASan y UBSan aplicados al componente de opciones e importación de contenido.
- Comprobación de paquetes y simulación del instalador de actualización 0.3.0 →
  0.4.0 en carpetas desechables, preservando archivos de datos de prueba.

Las pruebas instrumentadas no son partidas reales. La prueba de ventana de Edge
no demuestra que un título llegue al menú o permita completar una partida. No se
ha validado nuevo gameplay ni una partida online entre dos equipos en esta fase.
Las pruebas opcionales que requieren un catálogo privado de avatar o un servidor
HTTPS externo quedan omitidas. Rivals permanece sin solución demostrada.

El dossier inicial y los capítulos 09/10 conservan las hipótesis de Rivals y los
candidatos de mantenimiento del core. Esta integración permite probar Edge sin
portar esos cambios al motor activo. Registra por título revisión, Media ID/TU,
GPU, configuración, síntoma y resultado real, separando arranque, menú, gameplay
y multijugador funcional.
