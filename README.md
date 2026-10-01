# Emulos360

Biblioteca nativa en C++20 y Qt Quick/QML: portadas verticales, búsqueda, lectura
GOD, XBLA y juegos extraídos con `.xex`, descarga y selección de carátulas por región, ajustes persistentes y modos
escritorio/consola. La interfaz es nativa y no incrusta un navegador. La base es Xenia Canary Netplay
(`AdrianCassar/xenia-canary`, revisión `6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c`),
en `engine/xenia`. Los cambios propios se conservan en `patches/xenia-netplay/`.

Consulta [SPECDEV.md](SPECDEV.md) para las decisiones, propuestas técnicas y preguntas pendientes.
El estado actual, las comprobaciones y lo pendiente están en [docs/STATUS.md](docs/STATUS.md).

Para clonar el código y reconstruir el motor modificado:

```sh
git clone --recurse-submodules <URL_DEL_REPOSITORIO> Emulos360
cd Emulos360
bash scripts/prepare-engine.sh # En Windows: ./scripts/prepare-engine.ps1
```

La revisión de Xenia está fijada como submódulo. El preparador aplica los cambios
de Emulos360 de forma idempotente. Los datos personales, juegos y binarios de
compilación permanecen fuera de Git. Consulta [la guía de publicación](docs/PUBLISHING.md)
y [el diseño de futuras actualizaciones](docs/UPDATES.md).

La interfaz aloja la ventana del núcleo dentro de Qt en Windows y
comparte con él ajustes, perfiles y partidas. Incluye creación de perfiles
locales, avatares de imagen, copias/restauración de partidas y un selector de
clientes XEX. Incluye Netplay y System Link del motor, dos unidades de guardado
independientes y un dashboard Metro `.xex` para el modo consola. Una partida online entre dos equipos aún no está
verificada en esta instalación.

- **Partidas:** ver interno/externo, configurar la carpeta externa, copiar y restaurar.
  Dentro del juego se elige unidad cuando el título solicita almacenamiento.
- **Acerca de:** en Ajustes → Emulos360 se muestran las funciones creadas para
  esta aplicación y los principales proyectos integrados, con su función y enlace oficial.
- **Logros:** progreso y puntos del perfil seleccionado, con detalle por juego,
  búsqueda, filtros de desbloqueados/pendientes y logros secretos.
- **Comunidad:** consultar salas públicas, filtrar por juego/anfitrión, abrir el
  juego correspondiente, elegir modo de red y abrir el gestor Netplay para configurar
  servidor, interfaz, amigos y perfiles. Crear/unirse a partidas se hace en el juego.
- **Party de voz:** crear una sala en Emulos360 o unirse por descubrimiento LAN o
  IP, puerto y código. Voz Opus y mensajes por canales WebRTC cifrados, incluso
  mientras se juega. El anfitrión aloja la señalización; no hace falta instalar
  otro programa ni contratar un servidor. Para conexiones por Internet, consulta
  [las instrucciones de puertos y limitaciones](docs/VOICE_CHAT.md).
- **Sesión:** volver a biblioteca, consultar salas, controlar el juego y cerrar
  normalmente desde la barra Qt. Navegar por la interfaz no pausa el juego.
- **F11 / Modo consola:** dashboard Metro `.xex`; **Guía mantenida 2,5 s:** menú de sistema.
  Puente XInput en Windows y SDL2 en Linux para navegar con mando; teclado y ratón también disponibles.
- **Importar juegos:** selecciona una ISO o un 7z local. La conversión a GOD y la
  instalación de paquetes XBLA se hacen en segundo plano; al terminar se actualiza
  la biblioteca. Los archivos originales elegidos por el usuario se conservan.
- **Tienda:** explora el catálogo público de Vimm por letras A–Z (incluidas las
  páginas adicionales de cada letra) o busca juegos Xbox 360 y XBLA. La ficha
  se abre en el navegador predeterminado para su comprobación y descarga. Si el
  navegador guarda un 7z nuevo en Descargas con el nombre del juego, Emulos360 lo
  detecta, instala y elimina el archivo descargado tras comprobar la instalación.
  «Elegir descarga» permite importar archivos de otra carpeta y conserva ese original.

- [Compilar y abrir](docs/BUILD.md)
- [Arquitectura y próximos hitos](docs/ARCHITECTURE.md)
- [Ajustes del motor y consola](docs/SETTINGS.md)
- [Perfiles, avatares y partidas](docs/PLAYERS_AND_SAVES.md)
- [Iconos y catálogo de compatibilidad](docs/GAME_COMPATIBILITY.md)
- [Registro de cambios del fork](docs/FORK_CHANGES.md)
- [Dashboard Metro](XEXplugins/MetroDashboard/README.md)
- [Alternativas de multijugador para Gears](docs/NETPLAY_OPTIONS.md)
- [Party de voz sin servidor externo](docs/VOICE_CHAT.md)

```powershell
./scripts/build-engine.ps1
./XEXplugins/MetroDashboard/build.ps1
./scripts/build-ui.ps1 -Package
./out/Emulos360/Emulos360.exe --library 'D:\XBOX360_GAMES'
```

Selecciona un juego → **Carátula** → región → imagen → **Usar esta
carátula**. Con el mando, Y abre el selector y A ejecuta el juego. Dentro del
selector, la cruceta o el stick resaltan una caja, A la aplica y B cierra;
en el teclado equivalen a F6, flechas, Intro y Escape. El doble clic también
abre el selector. Las descargas se guardan en `data/covers`, fuera de los
paquetes de juego.

En la biblioteca, X en el mando (F8 en el teclado) o clic derecho sobre una
carátula abre la confirmación para eliminar el juego. También hay un botón
«Eliminar · X». Se intenta mover el juego a la Papelera; si no está disponible,
el diálogo advierte que los archivos se borrarán definitivamente. Para juegos
XEX se elimina su carpeta completa, salvo si contiene otros juegos detectados
o es la raíz de la biblioteca. Después se eliminan las carpetas superiores que
hayan quedado vacías, hasta llegar a la raíz de la biblioteca. En el diálogo,
usa izquierda/derecha para elegir, A para aceptar y B para cancelar.

Durante una partida integrada en Windows, **Pantalla completa** amplía la
ventana. F8 o pulsar el stick derecho oculta o muestra la franja de controles
para que el juego ocupe toda la pantalla. F11 vuelve al modo ventana.

La compilación normal no construye ni ejecuta pruebas. No se repiten pruebas del
motor original sin cambios. Detalles de los proveedores en [docs/COVERS.md](docs/COVERS.md).

El empaquetado incluye `iso2god` 1.8.0 (licencia MIT) y 7-Zip de la instalación
local, junto con sus avisos de licencia en `out/Emulos360/tools`. La importación
elimina los archivos temporales de extracción y conversión tras instalar y
comprobar el paquete. La búsqueda del catálogo funciona desde Emulos360. La
descarga empieza en el navegador al pulsar **Descargar** en la ficha de Vimm;
Emulos360 detecta después el 7z completo en la carpeta Descargas del sistema.
Los navegadores con otra carpeta de descarga requieren «Elegir descarga». No se
reutilizan cookies del navegador ni se evita la comprobación del sitio.
