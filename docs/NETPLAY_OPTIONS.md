# Opciones de multijugador para Emulos360

Actualizado: 2026-09-20. **Netplay integrado en el código y compilado** sobre
`AdrianCassar/xenia-canary@6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c`, rama
`netplay_canary_experimental`. Se eligió esta revisión próxima a la base reciente
de Canary; no es el tag estable v6.0.0. La conexión de una partida entre dos equipos
sigue sin verificarse en esta instalación.

En Emulos360, **Comunidad** permite seleccionar Offline, System Link o Netplay,
guardar el modo y abrir el gestor nativo del núcleo. Se mantiene el servidor
público del proyecto como valor inicial y se admiten servidores personalizados.
**Ajustes → Red** contiene las opciones persistentes; servidor/interfaz/amigos
también se gestionan desde los menús de Netplay. No hay WebView ni interfaz web.

El perfil local debe habilitarse para Netplay mediante el menú contextual del
gestor de perfiles del núcleo (**Convert to Xbox Live-Enabled Profile**). No se
convierte automáticamente ningún perfil existente. El XUID local y los guardados
permanecen asociados al perfil; el servicio comunitario usa su identidad online.
Cerrar el gestor antes de iniciar un juego desde la biblioteca evita dos procesos
usando a la vez los mismos perfiles y partidas.

Crear/buscar/unirse a una sala se realiza en los menús multijugador del título;
la lista de amigos está en **Netplay → Manager → Friends**. Emulos360 incluye un
navegador Qt de sesiones anunciadas con filtros y apertura de juegos instalados;
no sustituye la unión/creación del menú del juego. La NAT entre pares no se ha
verificado. Véase `QT_NETPLAY_INTEGRATION.md`.
No se han instalado VPN, controladores ni cambiado reglas del router/firewall.

## Recomendación

Se incorporó **Xenia Canary Netplay**, de AdrianCassar. Ya
implementa funciones de multijugador y usa un servicio propio para sesiones.
Publica un listado de sesiones activas y documenta descubrimiento de sesiones
de amigos en juegos que lo permiten. Es la alternativa más alineada con el
objetivo del usuario; esta recomendación es una valoración del proyecto.
Fuentes: [README](https://github.com/AdrianCassar/xenia-canary/blob/netplay_canary_experimental/README.md),
[configuración](https://github.com/AdrianCassar/xenia-canary/wiki/Config-Setup).

| Método | Cómo se crean y encuentran partidas | Requisitos y límites |
| --- | --- | --- |
| Netplay con servicio comunitario | Menús multijugador del juego; servicio de sesiones y funciones de amigos según el título | Núcleo Netplay y versiones compatibles del juego; depende del servicio y de la conectividad entre jugadores |
| System Link en LAN o red virtual | Un jugador aloja una partida y los demás la encuentran en el menú System Link | Núcleo con soporte System Link; misma red LAN o red virtual compatible |
| XLink Kai + Xenia Netplay | Reunirse en una arena y utilizar las sesiones System Link del juego | Configuración adicional de captura/puente; soporte experimental en Windows |

La wiki de Netplay explica el uso de redes virtuales —Radmin VPN, Hamachi o
Lanemu— y la elección de su interfaz en el emulador. La infraestructura virtual
transporta el tráfico; la implementación de System Link ahora la aporta Netplay.
[Systemlink](https://github.com/AdrianCassar/xenia-canary/wiki/Systemlink).

Team XLink documenta Xenia contra Xenia y advierte que, en su integración
actual, no funciona Xenia contra Xbox 360 física. Requiere pasos adicionales
con NPCAP y un puente de loopback. Por eso no es la primera opción propuesta.
[Guía oficial Xenia/XLink Kai](https://www.teamxlink.co.uk/wiki/Xenia_Support).

## Gears of War

La tabla oficial del proyecto consultada indica:

| Juego | Partidas públicas Netplay | System Link |
| --- | --- | --- |
| Gears of War | Funcionando según el proyecto | Funciona sin servidor |
| Gears of War 2 | Funcionando según el proyecto | Funciona sin servidor |
| Gears of War 3 | Funcionando según el proyecto | Funciona sin servidor; considerado más estable |
| Gears of War: Judgment | Probado localmente; no marcado como público verificado | Funciona sin servidor |

Para Gears 3 la wiki señala además la versión base como la más estable para
matchmaking y cooperativo. No se aplica ningún downgrade ni parche a los juegos
del usuario a partir de esta investigación. El proyecto pide igualar Media ID y
versión entre participantes y advierte de diferencias entre ediciones, regiones,
actualizaciones y DLC. Estos reportes no garantizan campañas completas ni el
rendimiento en el PC del usuario.
[Compatibilidad](https://github.com/AdrianCassar/xenia-canary/wiki/Netplay-Compatibility).

## Plan original de integración (referencia)

1. Fijar una revisión de Netplay y comparar sus cambios con la base actual antes
   de incorporar código. Preservar los parches y datos de Emulos360.
2. Añadir configuración nativa Qt de modo de red, interfaz, servicio y perfiles
   que requiera esa revisión. Mantener copias de los perfiles actuales.
3. Conectar la biblioteca con el modo online. El listado de salas en Qt podrá
   consultar el servicio, pero un botón no puede prometer unirse directamente
   si el título requiere pasar por su propio menú.
4. Revisar conectividad/NAT según la guía de Netplay antes de tocar router o
   firewall. No se han instalado controladores, VPN ni reglas de red.
5. Hacer una comprobación concreta de las modificaciones propias con dos
   participantes y un título, cuando corresponda, sin repetir suites upstream.

El backend público tiene código independiente: [Xenia Web Services](https://github.com/AdrianCassar/Xenia-WebServices).
Está diseñado específicamente para Xenia y no es una API oficial de Microsoft.
Usar ese servicio remoto no exige una interfaz web en Emulos360: la aplicación
puede seguir siendo C++/Qt. Autoalojar el backend sería una decisión separada,
pues su implementación utiliza Node.js/TypeScript y MongoDB.

XBGuard queda como paquete de consola auditado. Ni importarlo ni declarar un
perfil local como conectado sustituye la implementación real de multijugador.
