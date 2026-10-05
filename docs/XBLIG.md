# Juegos indie XBLIG: integración experimental

> **Historial de una integración retirada el 5 de octubre de 2026.**
> No es una función actual ni una prioridad futura. Véase
> [el funcionamiento con un único core](SINGLE_CORE.md).


> **Decisión del usuario, 5 de octubre de 2026:** retirar primero Nexia y después
> Edge al retomar el trabajo. XBLIG queda descartado. El objetivo es mejorar
> compatibilidad, estabilidad, rendimiento y gráficos del core principal,
> conservando juegos y Netplay funcionales. Este documento conserva el historial;
> no continuar ampliando estos proveedores. Ver [la próxima prioridad](NEXT_PRIORITIES.md).

Estado del 5 de octubre de 2026, posterior a la release pública 0.4.0.
La biblioteca y el importador reconocen Xbox Live Indie Games. En Windows x64
se utiliza el host XNA de Nexia360 como proveedor externo.
**El perfil común y el aislamiento de partidas están implementados y probados.
La ejecución de juegos, sus guardados reales y los avatares XNA requieren
validación por título. Avatar Bowling todavía falla antes de presentar frames.**
No se ha publicado otra release ni modificado la instalación del usuario.

## Detección y selección

Un XBLIG contiene ensamblados administrados, `GameInfo.xml` y recursos `.xnb`.
Su STFS suele usar `00000002` y el Title ID común `584E07D2`; esos valores solos
no distinguen un juego de un DLC. Se exige metadata, un VirtualTitleID válido
y un único ejecutable CLR. Se rechazan DLL como entrada, XML con DTD, metadata
ambigua, índices fuera del paquete, nombres inseguros y cadenas cíclicas.
El lector está acotado y abre el original solo en lectura. Comprueba estructura;
no acredita firma, todos los hashes STFS ni integridad de gameplay.

La identidad `XBLIG-<VirtualTitleID sin guiones>` se usa para carátulas y datos
por juego. Se aprovecha el icono embebido; no se consulta Marketplace ni se
atribuye la leyenda Canary usando el Title ID compartido. `Jugar` dirige estos
paquetes a Nexia antes de seleccionar Netplay/Edge. El selector de core XEX y
el gestor de DLC/TU nativos quedan deshabilitados para ellos. El importador
no reclasifica otros DLC ni sobrescribe paquetes existentes.

## Un perfil y partidas comunes

Por petición del usuario, **se comparten perfil y partidas solamente**.
Configuración, cachés, logs y contenido instalado mantienen sus rutas por core.
No se duplican ni sincronizan periódicamente los perfiles: las vistas de Edge
y Nexia apuntan al mismo `FFFE07D1` del perfil seleccionado en el almacenamiento
principal. Account, GPD y los demás datos del perfil quedan en ese directorio.
Windows usa enlaces de directorio NTFS; Linux usa enlaces simbólicos.
Para datos en Windows se necesita una unidad local NTFS.

Edge enlaza `<xuid>/<TitleID>/00000001` al almacenamiento principal. Respeta la
unidad de guardado seleccionada; si solo existe una partida en la otra unidad,
usa esa ubicación. Si existen partidas en ambas unidades, conserva la elección
de unidad; no mezcla automáticamente dos versiones. DLC/TU se siguen incorporando
a la copia independiente de Edge cuando faltan archivos. No se comparte su
configuración ni se reemplazan paquetes con el mismo nombre.

**Nexia tiene un contrato de guardado distinto**: `ResolveGameUserContentPath`
resuelve la partida de su fachada administrada dentro de
`<xuid>/FFFE07D1/00010000/<xuid>/584E07D2`. No basta enlazar `00000001` fuera del
perfil. Al preparar cada indie, Emulos360 conecta únicamente esa rama a
`cores/xblig/saves/XBLIG-<VirtualTitleID>/<xuid>/containers`. El otro puente,
`Nexia_XnaStorageRoot`, sí utiliza `<xuid>/584E07D2/00000001/PlayerN` o `AllPlayers`;
se enlaza su `00000001` a `saves/XBLIG-<VirtualTitleID>/<xuid>/native`.
Se cubren ambos contratos y sus layouts permanecen separados dentro del mismo
almacén por indie. Cambiar de indie cambia el enlace de la rama del perfil;
Account y los GPD continúan siendo comunes. El candado del almacenamiento principal
se mantiene durante preparación y sesión, evitando dos escritores desde Emulos360.
Las pruebas alternan dos indies y verifican que cada uno recupera su progreso.

Si hay una carpeta privada de una integración anterior, se conserva en
`.before-shared/<uuid>` con una nota de procedencia. Las partidas nativas que
faltan pueden migrarse una vez; si dos archivos del mismo nombre difieren,
se conservan ambos y se informa de sus rutas antes de iniciar la sesión.
Una antigua carpeta XNA común con progreso sin identificar no se atribuye al
primer indie: se conserva y pide identificar sus partidas. No se ha realizado
esta migración sobre datos reales de la instalación del usuario.

`Partidas de XBLIG` abre el almacén por VirtualTitleID. Un enlace y una escritura
de prueba comprueban almacenamiento, no demuestran que un juego complete su
propio ciclo de guardar/cerrar/cargar.

## Proveedor y separación del motor

Se usa el ejecutable oficial de [Nexia360](https://github.com/Nexia360/Nexia360).
No se han portado scheduler, gráficos, CLR ni XAM al core principal. Netplay,
MultiP, helpers y dashboard conservan el motor actual y sus argumentos.
Los ejecutables de Edge y Nexia también permanecen sin modificar.

| Elemento | Versión fijada |
| --- | --- |
| Release | `Nexia_v0.4.2` |
| Revisión | `faa3c915b0cb8e7036743b6d36d91270a29837c6` |
| Plataforma | Windows x64 |
| SHA-256 del ejecutable | `2d625237bc1c6af08fd44a894454326d9549c305b1aba264c9bf05ba63384a53` |
| SHA-256 de LICENSE | `3d58f25c15634b6ec01d1f133ef798209ae06626ab8d2227b6223d5a9f5113f4` |

`engine/xblig-release.json` y `scripts/setup-xblig.ps1` fijan descarga, hashes,
licencia BSD y avisos de Mono.Cecil 0.9.6, SDL2 2.32.10 y OpenAL Soft 1.24.3.
Se despliega el proveedor en una carpeta privada escribible, pues extrae su
host junto al ejecutable. El paquete original también se copia y verifica por
SHA-256 para que Nexia solo escriba en su copia de trabajo.

```text
data/cores/xblig/
  .session.lock
  runtime/
    Nexia360.exe, LICENSE
    xna/
      Nexia.Xna.Host.dll, runtimeconfig y dependencias del proveedor
      console/                   # runtime importado localmente
  saves/
    XBLIG-<VirtualTitleID>/<xuid>/
      native/                    # puente de StorageRoot, PlayerN/AllPlayers
      containers/                # fachada administrada por displayName
  games/
    XBLIG-<VirtualTitleID>/
      nexia360.config.toml
      title/<paquete>
      content/                   # vista del perfil común
      cache/
      core.log, core-console.log
```

Emulos360 supervisa la ventana nativa y el PID, y permite cerrar la sesión.
El candado XBLIG impide importar el runtime durante una sesión en otra instancia.
**No hay multijugador XBLIG integrado**: se fija `network_mode=0`, se desactiva
el actualizador de Nexia y se rechaza el lanzamiento durante MultiP privado.
En Linux se detectan/importan los juegos, pero se rechaza su ejecución porque
este proveedor está fijado para Windows. No se añade Wine implícitamente.

## Dependencias y runtime obtenido

Nexia necesita .NET 9, .NET Framework 3.5, DirectX legacy y XNA Framework 3.1 y
4.0 Refresh de Windows. El usuario autorizó instalar los dos XNA; ambos MSIs
oficiales de Microsoft tenían firma válida y SHA-256 fijado. Se instalaron con
resultado 0 y `/norestart`; no se solicitó ni realizó un reinicio. Los otros
componentes ya estaban presentes. `scripts/setup-xna-windows.ps1` descarga y
verifica solamente; `-Install` requiere una consola elevada y autorización.

Estos frameworks Windows **no contienen el runtime XNA de consola**. Se obtuvo
por separado el Title Update histórico del XNA Indie Player desde
[el archivo conservado en Digiex](https://digiex.net/threads/xna-title-update.10600/).
La descarga procede de ese archivo, no de un endpoint actual de Microsoft.

| Artefacto | SHA-256 |
| --- | --- |
| `IndyGameTitleUpdate.zip`, 20.550.784 bytes | `f03e8edf476e7bcc2e5c218eea3fcdfb510031ee8271411ef46b54c6fcffb213` |
| `tu32000100_00000000`, 28.303.360 bytes | `2e5a4f44cfd9135a940954474f67bd6100050844c0093ec73a70dfc7d239c249` |

El STFS LIVE identifica `584E07D2` y tipo `000B0000`. Contiene runtimes 3.0,
3.1 y 4.0. Se extrajeron los 16 `.dlx` de `Runtime/v4.0`, comprobando límites y
SHA-1 de los bloques leídos. La consistencia de hashes **no verifica la firma RSA
de Xbox**. La inspección CLR/IL confirma implementaciones reales, a diferencia
de las referencias del SDK con métodos vacíos. No se sustituyen por DLL Windows
ni se renombra material de referencia para superar la comprobación.

```powershell
# Descarga y extracción en .tools; no instala software ni ejecuta el STFS.
./scripts/setup-xna-console.ps1
# También puede importar en una build local ya preparada:
./scripts/setup-xna-console.ps1 -Executable '<Emulos360.exe>' -DataDirectory '<data>'
```

El script fija hashes, extrae únicamente el miembro esperado del ZIP y usa
`scripts/extract-xna-runtime.py` con Python 3. La interfaz permite importar
una carpeta extraída con `Ajustes → Core → Importar runtime XNA`; el nuevo CLI
`--import-xna-runtime <carpeta> --data-dir <data>` permite hacerlo sin abrir la UI.
La importación valida los 16 PE administrados antes de copiar. No modifica el
original ni se ejecuta durante una sesión. `Configurar XBLIG` abre los ajustes
nativos del proveedor.

Los archivos de consola quedan en caché/datos locales y **no entran en Git ni
en paquetes públicos**. El empaquetador excluye `data`; la preview local puede
contener su runtime importado para pruebas.

## Evidencias y trabajo pendiente

- Windows: 14 comprobaciones XBLIG y 6 de opciones/almacenamiento pasaron,
  incluidas escrituras compartidas, cambio de rama XNA, separación por indie,
  recuperación de carpetas anteriores y rechazo de versiones divergentes.
- ASan/UBSan Linux: 14 comprobaciones pasaron sobre las mismas muestras reales.
- Frontend Windows y Linux compilados; cinco suites CTest pasaron (cinco fixtures
  UI opcionales omitidos) y las rutas Netplay/Edge mantuvieron sus argumentos.
- Avatar Bowling, Avatar Karting y Avatar Warfare!: metadata, entrada CLR,
  GUID propio e icono detectados; los originales permanecen sin modificar.
- La biblioteca nativa conserva sus 84 títulos anteriores. SFG Beach Volleyball
  y Survival Games Season 1 también se reconocen como XBLIG.
- El Nexia real reconoce el perfil creado por el helper actual como `SharedXna`
  en slot 0 y llega a `XnaHost: hosting AvatarBowling.exe` usando .NET 9 y el
  runtime de consola. Esto acredita interoperabilidad de Account y arranque
  del host; no una partida. La captura observada permanece negra, con 0 frames.
- Avatar Bowling solicita XNA 3.1. Con la ruta oficial preparada para 4.0,
  la reescritura no resuelve `AvatarBone` y después falla `Graphics.Viewport`.
  El mero importado de `Runtime/v4.0` no añade soporte completo a XNA 3.1.
- Avatar Karting también carga el perfil y host, pero falla al resolver
  `GamerServicesDispatcher` en el ensamblado principal. Avatar Warfare carga
  bibliotecas 4.0, recursos LZX, shaders y audio y sigue ejecutándose tras 30 s;
  no aparece `title failed` en esa observación. La captura por HWND permanece
  negra y no valida por sí sola el swapchain D3D12. No se comprobó menú,
  control del personaje ni guardar/cargar.
- Una prueba desechable con los dos ensamblados 3.1 y referencias para Cecil
  avanzó más, pero falló al resolver `D3D`; otro ensayo cerró el proceso. Esos
  cambios se descartaron y no forman parte del lanzamiento de la aplicación.
- Gameplay, guardar/cargar dentro del juego, avatar renderizado y sesiones
  largas siguen pendientes. La ejecución XBLIG en Linux y MultiP también.

La siguiente corrección debe permanecer en el proveedor independiente: resolución
por versión, reescritura de referencias y contrato de exports/ABI 3.1. Antes de
integrarla, validar tipos y llamadas con juegos 3.1 y 4.0, luego menú, partida,
guardar/cerrar/reabrir y avatar del perfil. No modificar Netplay para resolver
estos fallos ni acreditar compatibilidad por un mensaje de hosting.

## Repetir comprobaciones

```powershell
./scripts/setup-xblig.ps1 -Destination build-dual-core/Release/engine/xblig
cmake --build build-dual-core --config Release --target xblig_tests core_options_tests los360 ui_tests netplay_integration_tests
$env:EMULOS_XBLIG_LIBRARY = '<carpeta con los paquetes Avatar>'
./build-dual-core/Release/xblig_tests.exe -o xblig-results.txt,txt
$env:EMULOS_TEST_STUB = '<core_launch_stub.exe>'
./build-dual-core/Release/netplay_integration_tests.exe dualCoreRoutesCommandsAndKeepsIndependentStorage -o routes.txt,txt
```

Incluir Qt/6.8.3/msvc2022_64/bin en PATH. La prueba optativa
`officialXbligHostLoadsCommonProfileAndConsoleRuntime` usa `EMULOS_TEST_XBLIG`,
`EMULOS_TEST_CORE`, `EMULOS_XBLIG_GAME`, `EMULOS_XNA_RUNTIME` y opcionalmente
`EMULOS_XBLIG_REPORT`. Crea su propio perfil y datos temporales. Verifica que
Nexia lee el Account común y que el host sigue abierto, y conserva logs/captura;
un resultado de esta prueba no implica frames ni gameplay.

La versión pública permanece 0.4.0. El usuario conserva commit, push, publicación
y pruebas de actualización. Consultar el [capítulo 12 y la evidencia fechada](research/xenia-canary/12-XBLIG-AVATAR.md)
antes de reutilizar las conclusiones.
