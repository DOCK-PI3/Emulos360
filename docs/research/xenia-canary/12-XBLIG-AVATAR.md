# 12 — Juegos Avatar XBLIG: diagnóstico e integración

> **Historial de una integración retirada el 5 de octubre de 2026.**
> No es una función actual ni una prioridad futura. Véase
> [el funcionamiento con un único core](../../SINGLE_CORE.md).


> **Decisión vigente del usuario, 5 de octubre de 2026:** XBLIG queda descartado.
> Retirar Nexia y después Edge al retomar; centrar mejoras en el core principal.
> Este capítulo es histórico y sirve para preservar datos durante la retirada,
> no para continuar soporte XBLIG. Ver [la prioridad actual](../../NEXT_PRIORITIES.md).
**Registro histórico:** las secciones iniciales documentan el diagnóstico del
4 de octubre. La integración autorizada después se describe al final y en
[XBLIG.md](../../XBLIG.md). Detectar un paquete y abrir el proveedor siguen sin
acreditar gameplay ni funcionamiento del avatar.

Fecha: **4 de octubre de 2026**. Petición: revisar tres juegos recién descargados
que el usuario considera XBLA y que desea usar para probar el avatar. Alcance:
lectura de sus archivos, inspección estática del lector de biblioteca e inventario
con la aplicación instalada. No se ejecutaron juegos ni se modificaron archivos,
configuración, perfiles o cores.

## Diagnóstico confirmado

Los tres son **Xbox Live Indie Games (XBLIG)**:

| Juego | Ensamblado encontrado dentro del STFS | Tamaño del paquete |
| --- | --- | --- |
| Avatar Bowling | `AvatarBowling.exe` | 21 987 328 bytes |
| Avatar Karting | `AvatarKarting.exe` | 49 565 696 bytes |
| Avatar Warfare! | `AvatarWarfare.exe` | 59 789 312 bytes |

Ya están bajo `D:/XBOX360_GAMES`. Sus paquetes llevan firma de formato `LIVE`,
Title ID de contenedor `584E07D2` y tipo de contenido `0x00000002`, dentro de
`584E07D2/00000002/`. El STFS contiene `GameInfo.xml`, ensamblados `.exe`/`.dll`
y recursos `.xnb`. Los tres `.exe` inspeccionados tienen cabecera PE y directorio
CLR (RVA 8200, tamaño 72): son código administrado del entorno XNA/.NET.
La inspección parcial no valida integridad completa del paquete ni todas sus
entradas y no atribuye un Title ID de juego individual a partir del contenedor.

`app/library.cpp` confirma dos filtros independientes:

1. `scanLibrary` solo trata como paquetes de juego las carpetas `00007000`
   (GOD) y `000D0000` (XBLA).
2. `parsePackageHeader` solo devuelve un paquete GOD para tipo `0x7000` con
   volumen SVOD válido, o XBLA para tipo `0xD0000` con volumen STFS.

Por tanto, estos contenedores de tipo `0x2` se omiten. El inventario de la aplicación
instalada devuelve **cero juegos y ningún error** en cada una de las tres carpetas.
La causa no es el prefijo «Avatar», un error demostrado del avatar ni un rechazo
ocurrido durante emulación: el juego todavía no se llega a lanzar.

El importador de paquetes también reutiliza la clasificación GOD/XBLA. Cambiar el
nombre de la carpeta no cambia la cabecera ni aporta soporte XNA. No reclasificar
globalmente todo `00000002` como juego: ese tipo también representa DLC normal.

## Lo necesario para ejecutarlos

La integración 0.4.0 no incluye un lanzador administrado XNA/.NET para estos
ensamblados. Los dos cores distribuidos siguen la ruta de módulos XEX; registrar
un tipo de contenido XNA o leer un STFS no equivale a ejecutar su programa.
Mostrar una carátula en la biblioteca tampoco resolvería esa capa.

Para soportarlos haría falta identificar específicamente paquetes XBLIG (sin
confundir DLC), extraer y resolver identidad/versión desde su metadata, proporcionar
un runtime/lanzador compatible con XNA de Xbox 360, conectar perfiles, contenido,
guardados y APIs de avatar, y validar arranque y gameplay. No usar el Title ID común
`584E07D2` como identidad única de los tres juegos para cachés u opciones por juego.

La falta de detección no sirve como prueba de funcionamiento o fallo de nuestros
avatares. Las rutas de avatar nativo/XAM y de XNA requieren validación separada.
Consultar [el estado del avatar](../../AVATAR_SYSTEM.md) y
[los contratos y pruebas previas](../../AVATAR_ABI.md).

## Referencia upstream descubierta, pendiente de auditoría

El [README de Nexia360](https://github.com/Nexia360/Nexia360), consultado hoy,
anuncia un host administrado para juegos XNA/XBLIG, instalación del runtime y
renderizado de avatares. Es una referencia potencialmente útil para el trabajo
anterior, **no una compatibilidad probada en Emulos360**. No se verificó una revisión
Git ni se inspeccionó su implementación en este diagnóstico; antes de reutilizarla
hay que fijar revisión, auditar dependencias/contratos y comprobar resultados.
No se instaló ese fork ni se incorporaron cambios al motor actual o a Edge.

El [reporte histórico de Xenia sobre Miner Dig Deep](https://github.com/xenia-project/game-compatibility/issues/1101)
usa el mismo Title ID de contenedor y documenta un bloqueo XNA. Es evidencia
histórica de esa revisión, sin justificar una afirmación universal sobre todos
los forks actuales. La nueva referencia debe investigarse con evidencia actual.

Datos de la comprobación: [inventario y límites](xblig-avatar-evidencia-20261004.json).

## Integración autorizada — 5 de octubre de 2026

La ruta XBLIG ya reconoce `GameInfo.xml`, `VirtualTitleID` y el ejecutable CLR;
no reclasifica globalmente DLC. Biblioteca, importador, carátulas y dashboard
usan una identidad propia por indie. Los tres paquetes Avatar originales
analizados vuelven a ser detectables y sus hashes permanecen iguales tras la
lectura. En el inventario actual de D: aparecen además SFG Beach Volleyball y
Survival Games Season 1, conservando los 84 títulos nativos anteriores.

Se fija el proveedor Windows de Nexia360 `Nexia_v0.4.2`, revisión
`faa3c915b0cb8e7036743b6d36d91270a29837c6`, con hashes del ejecutable y licencias.
La inspección de sus fuentes confirma un host .NET 9, reescritura de ensamblados,
resolución de runtime de consola y puentes nativos de gráficos, entrada, audio,
perfiles y avatar. Es un subsistema completo; no se ha transplantado al core
Netplay. Su fuente inspeccionada incluye `xna_launcher`, `xna_host`,
`xna_dependencies`, `xna_runtime_install`, `xna_avatar` y el bootstrap administrado.

La aplicación despliega el ejecutable en `data/cores/xblig/runtime`, copia el
paquete original para el proveedor y asigna a cada VirtualTitleID su configuración,
cache, logs y `content_root` propios. Esto evita mezclar las partidas del Title ID
compartido. El perfil principal se copia, nunca se sincroniza hacia atrás. Los
juegos se mantienen offline y las salas Netplay/MultiP permanecen deshabilitadas.
Netplay, Edge y los helpers nativos conservan sus rutas y argumentos anteriores;
no se modifica el código ni el binario activo de `engine/xenia`.

La prueba directa del ejecutable oficial encontró `584E07D1/AvatarBowling.exe`
dentro del STFS y desplegó su host. Se detuvo antes de CLR porque el equipo no
cumple los requisitos XNA Framework 3.1/4.0 de Windows. No arrancó gameplay.
La ventana de configuración se comprueba por separado con el proveedor real;
su resultado final está en la evidencia enlazada abajo.

### Búsqueda autorizada en el disco original

El usuario montó Content en X: y SystemExt en Q:. Se inspeccionaron en lectura
archivos, nombres, cabeceras de actualizaciones y tablas STFS de paquetes de
sistema. X: no contiene el Title ID `584E07D2` ni el conjunto `.dlx`; Q: contiene
`32000100/Xna_TitleLauncher.xex`, pero no el runtime `Runtime/v4.0`. Los cuatro
paquetes de sistema inspeccionados de Q: contienen assets/animaciones de avatar
y datos Kinect, sin ese runtime. No se escribieron, movieron o borraron archivos
en el disco original. No hay razón para copiar particiones completas.

La extracción estática del instalador oficial Microsoft XNA Game Studio 4.0
aportó **referencias de compilación de Xbox 360 con métodos vacíos**, verificadas
por IL. No se instalaron globalmente ni se renombraron a `MXF.*.dlx`. Las DLL
XNA de Windows del SDK local tampoco implementan el runtime administrado de
consola. Estas referencias no resuelven el bloqueo.

### Estado de validación

Frontend Windows y Linux compilados; pruebas del lector/importador y de
separación de datos, interfaz y rutas nativas ejecutadas. Se incluyen pruebas
ASan/UBSan en Linux y paquetes Avatar reales. El proveedor de ejecución sigue
limitado a Windows. No hay evidencia de gameplay, avatar XNA, persistencia real
de partidas ni multijugador en Emulos360. La siguiente etapa depende de una copia
propia del runtime original y de instalar las dependencias Windows que faltan.

Consultar [la guía de integración](../../XBLIG.md), el manifiesto
`engine/xblig-release.json` y [la evidencia de implementación](xblig-integracion-evidencia-20261005.json).
No reemplazar esta distinción por el anuncio de compatibilidad de Nexia ni por
una clasificación de Canary que usa el Title ID compartido.

## Corrección del modelo de datos y dependencias — 5 de octubre de 2026

El usuario rechazó copiar perfiles y pidió compartir perfil y partidas solamente.
La implementación actual usa el mismo Account/GPD para Netplay, Edge y Nexia.
Las partidas nativas de Edge apuntan al almacenamiento principal; DLC/TU y ajustes
conservan sus copias independientes. Los perfiles antiguos se archivan y dos
versiones distintas de una partida no se sobrescriben automáticamente.

La inspección de `xna_os.cc`, `xna_exports_impl.cc` y `content_manager.cc` del
proveedor fijado revela un contrato distinto para XNA: el guardado reside en
`<xuid>/FFFE07D1/00010000/<xuid>/584E07D2`, dentro del perfil, no en la carpeta
nativa `00000001`. Por eso se conecta esa rama a un almacén por VirtualTitleID
al preparar cada indie, manteniendo el candado común durante la sesión.
Las pruebas alternan dos juegos y recuperan sus guardados de prueba sin mezclarlos.
Las conclusiones anteriores sobre perfiles copiados quedan sustituidas por esta
implementación; se conservan arriba como historial.

Los MSIs Microsoft XNA Framework 3.1 y 4.0 Refresh, con firmas válidas y hashes
fijados, se instalaron tras autorización explícita del usuario; ambos terminaron
con código 0 y sin reinicio. El disco original ya no es necesario para obtener
el runtime: se descargó el Title Update histórico conservado en Digiex y se
extrajeron los 16 ensamblados reales `Runtime/v4.0`. Se verificaron hashes de
bloques STFS y límites, sin acreditar firma RSA Xbox. La guía enlaza procedencia,
hashes y scripts reproducibles. No se incluyen estos binarios en Git/releases.

La prueba real lee correctamente el perfil creado por el helper actual, entra
a CLR y anuncia hosting de Avatar Bowling. Falla antes de dibujar: Avatar Bowling
solicita XNA 3.1, la reescritura no resuelve AvatarBone y luego falta Graphics.Viewport
en la resolución 4.0. Una prueba desechable 3.1 avanzó, pero falló en D3D/terminó
el proceso; no se incorporó ese ensayo al lanzamiento. La ventana negra y los
0 frames no acreditan menú, avatar ni gameplay. El bloqueo restante corresponde
al proveedor XNA independiente, no a Xbox Live ni a la detección de la biblioteca.

Consultar [la guía actualizada](../../XBLIG.md) y la evidencia JSON; no reutilizar
la falta de dependencias Windows o runtime como diagnóstico actual.

El puente `Nexia_XnaStorageRoot` se comprobó por separado: utiliza la ruta
nativa `00000001/PlayerN` o `AllPlayers`, mientras la fachada `ResolveStorageContainer`
usa la rama del perfil. Se enlazan ambos contratos a subcarpetas `native` y
`containers` del mismo almacén por VirtualTitleID; las pruebas cubren ambos.
Avatar Karting falla en GamerServicesDispatcher; Warfare carga recursos, shaders
y audio y permanece activo tras 30 segundos. La captura por HWND es negra y no
valida el swapchain; menú, gameplay y guardar/cargar siguen sin comprobarse.
