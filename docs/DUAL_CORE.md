# Segundo core: Canary Edge (0.4.0)

> **Historial de una integración retirada el 5 de octubre de 2026.**
> No es una función actual ni una prioridad futura. Véase
> [el funcionamiento con un único core](SINGLE_CORE.md).


> **Decisión del usuario, 5 de octubre de 2026:** retirar primero Nexia y después
> Edge al retomar el trabajo. XBLIG queda descartado. El objetivo es mejorar
> compatibilidad, estabilidad, rendimiento y gráficos del core principal,
> conservando juegos y Netplay funcionales. Este documento conserva el historial;
> no continuar ampliando estos proveedores. Ver [la próxima prioridad](NEXT_PRIORITIES.md).

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
core elegido. La interfaz de Logros y Partidas consulta ahora el almacenamiento común.
Los cambios de perfil y partidas de Edge usan esa misma ubicación; la persistencia
de un juego concreto requiere comprobar guardar y cargar con ambos cores.

## Perfil y partidas compartidos — 5 de octubre de 2026

Por petición del usuario se comparte el perfil seleccionado y las partidas.
DLC/TU, configuración y cachés continúan siendo independientes.

| Ruta | Contenido |
| --- | --- |
| `cores.ini` | Core global, GPU por core y excepciones por Title ID |
| `cores/edge/xenia-edge.config.toml` | Ajustes nativos de Edge |
| `cores/edge/content/` | Vista del perfil y partidas comunes; DLC/TU propios |
| `cores/edge/cache/` | Cachés de Edge |
| `cores/edge/core.log`, `core-console.log` | Diagnóstico de Edge |

Al preparar Edge se conectan el directorio `FFFE07D1` del perfil y el `00000001`
del título con enlaces de directorio. No se copian ni sincronizan periódicamente.
Se respeta la unidad de partidas elegida; si solo existe progreso en la otra
unidad, se utiliza esa ubicación. Los datos y helpers principales quedan
bloqueados durante preparación y sesión.

Las carpetas de una integración anterior se conservan en `.before-shared`.
Se recuperan partidas que faltan; cuando existen versiones diferentes de un
archivo se conservan ambas y se muestran sus rutas antes de iniciar Edge.
No se sobrescribe automáticamente progreso. Los DLC/TU que faltan mantienen
la copia independiente anterior; los paquetes con el mismo nombre no se sustituyen.
Una unidad externa desconectada no se recrea. Windows requiere datos en un disco
local NTFS; Linux utiliza enlaces simbólicos. No se ha migrado la instalación real
del usuario en estas pruebas.

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

## Prueba del usuario — 3 de octubre de 2026

El usuario confirma que la actualización a esta entrega y el reinicio de la
aplicación finalizaron correctamente en su instalación real. Indica una demora
moderada y una carpeta `data` de varios GB. No se midieron tiempos, fases ni
integridad de los datos en esta comprobación, por lo que no se establece una
causa verificada de la demora.

Las pruebas de juegos y de ambos cores en su instalación siguen pendientes;
el usuario comunicará cualquier error. Este reporte acredita actualización y
reinicio según su observación, sin ampliar la compatibilidad de gameplay.

## Ruta independiente para XBLIG — 5 de octubre de 2026

Los paquetes administrados XNA no usan el selector Netplay/Edge de los juegos XEX. Se detectan antes de esa selección y se dirigen al proveedor experimental Nexia, con paquete y ajustes privados por VirtualTitleID, perfil común y partidas separadas por indie. No modifica estos dos cores ni sus argumentos. La primera integración ejecuta solo en Windows, offline; gameplay, avatares XNA y multijugador pendientes. Consultar [XBLIG.md](XBLIG.md) para dependencias, almacenamiento y pruebas.
