# Emulos360 MultiP — servidor privado

Esta edición tiene sus propios ajustes, perfiles, partidas y base de datos bajo
`data/` junto a su ejecutable. La edición Windows se distribuye en
`out/Emulos360-MultiP`; la edición Linux, en `out/Emulos360-MultiP-linux`.
La biblioteca de juegos puede ser la misma. Crea un perfil Netplay o importa una
copia de tu perfil en esta edición para empezar. No se comparte automáticamente
el almacenamiento de partidas con la versión habitual.

## Abrir en Ubuntu 26.04

Extrae `Emulos360-MultiP-linux.tar.gz` desde Ubuntu x86_64 para conservar los permisos. Desde esa carpeta:

```sh
bash ./install-dependencies.sh
./Emulos360
```

El instalador solicita `sudo` para los paquetes del sistema. Incluye también
la preparación de Node y MongoDB portables dentro de la carpeta de esta edición.
Puedes comprobar la instalación con `bash ./install-dependencies.sh --check`,
o instalar y abrir con `bash ./install-dependencies.sh --launch` desde tu sesión
gráfica habitual.

## Crear un grupo

1. Abre **Multijugador → Crear servidor**.
2. Indica un nombre, la IP/dominio que utilizarán tus amigos y un puerto TCP
   (predeterminado: 36000). La dirección local preseleccionada sirve para LAN.
3. Para Internet, usa tu IP pública/dominio y permite ese puerto en el cortafuegos
   y en el router. Puedes solicitar UPnP con la casilla correspondiente; el panel
   informa si la asignación tuvo éxito. Esta opción solo abre el puerto del
   servicio; no decide los puertos que necesita cada juego.
4. Inicia el servidor y crea una invitación para cada persona. Copia el enlace y
   envíaselo. La duración se elige en horas, hasta 30 días.
5. Mantén Emulos360 abierto. Detener el servicio o cerrar la aplicación detiene
   también los procesos auxiliares que esta instancia ha iniciado.

La detección de IP pública consulta `api.ipify.org` al pulsar el botón. Si cambias
la dirección anunciada, reinicia el servidor y crea invitaciones con la nueva
dirección. La detección no acredita que tu operador permita conexiones entrantes.

## Entrar al grupo

1. Abre **Multijugador → Unirse**, escribe tu nombre y pega la invitación completa.
2. La invitación incluye dirección, puerto y huella del certificado del servidor.
   Puedes cambiar la IP/puerto cuando te conectes por LAN/VPN; se conserva la
   comprobación del certificado.
3. Abre el juego desde la biblioteca y crea/busca la partida en su menú
   multijugador. Todos deben estar conectados al mismo servidor y utilizar una
   versión, actualización y contenido compatibles.

El certificado se comprueba por su huella SHA-256 antes de enviar credenciales o
datos del juego. Las invitaciones permiten acceder al grupo: compártelas solo con
sus participantes. Al revocar una invitación, también se revocan los accesos
emitidos a partir de ella. La conexión directa de una partida en curso depende
del juego y no se puede expulsar universalmente desde el servicio de salas.

## Panel nativo y web

- El anfitrión ve peticiones, errores, memoria del servicio Node, espera del
  proceso, eventos, invitaciones y conexiones recientes al panel/API.
- Los participantes ven salas y su respuesta al servicio. El anfitrión puede
  ocultar la lista de participantes y el total agregado de jugadores.
- Los datos de la propia sala necesarios para jugar siguen visibles para sus
  miembros. Los menús administrativos comprueban el rol en el servidor.
- El panel web está en la misma dirección HTTPS y puerto. El botón del anfitrión
  abre su sesión administrativa por loopback para que funcione aunque el router
  no permita acceder a su propia IP pública; los amigos entran pegando una invitación.
  El navegador puede requerir aceptar el certificado generado localmente.
- La respuesta y variación medidas corresponden a la API de salas. No se
  presentan como ping entre jugadores. Las estadísticas de un juego dependen de
  sus llamadas a los leaderboards implementados por Xenia-WebServices.

## Router, CGNAT y compatibilidad

El servicio coordina perfiles, presencia, sesiones y estadísticas compatibles.
El tráfico de partida conserva el transporte de Xenia y del título. Consulta
los puertos del juego y UPnP en los ajustes de Xenia. Si tu operador usa CGNAT,
puedes utilizar una VPN privada con IPv4 entre amigos, indicando la IP de esa
VPN y seleccionando su interfaz en los ajustes de red de Xenia cuando proceda.

Esta edición no incorpora una retransmisión universal de paquetes de juego ni
servidores dedicados específicos que todavía no existen para un título. Esa
compatibilidad necesita implementación y pruebas por juego. Una consulta de
salas o una prueba de API no acredita una partida completa entre dos equipos.

## Arquitectura y mantenimiento

`server/private-service.cjs` gestiona una pasarela HTTPS autenticada, panel,
invitaciones, medición de API y procesos auxiliares. MongoDB y Xenia-WebServices
escuchan exclusivamente en loopback, con puertos elegidos al iniciar; se abre
únicamente el puerto HTTPS del servicio. El motor accede mediante una pasarela
local con ruta aleatoria; esta añade las credenciales y valida TLS para cada
conexión al anfitrión. La dirección privada se aplica como argumento transitorio,
sin sustituir el servidor público guardado en la configuración.

La lista de salas utiliza consultas en bloque a MongoDB y una caché de un segundo,
sin esperar a descargas externas de imágenes. La búsqueda de partidas upstream
aplica su límite después de filtrar resultados. La limpieza administrativa solo
elimina registros ya borrados o caducados según el TTL upstream de una hora.

Fuentes fijadas:

- Xenia-WebServices (MIT): commit `6abcc4397e0b6e0632307d9418ecae086cc95f48`.
- MongoDB Community: 8.0.32. Binario portable; licencia y origen en `licenses/`.
- Node.js: runtime incluido en el paquete Windows; licencia en `licenses/`.
- Dependencias de la pasarela fijadas en `server/package-lock.json`.
- Cambios upstream reproducibles en `server/prepare-upstream.cjs`.

No elimines `data/private-netplay/certificate.json` ni `access.json` si quieres
conservar el certificado y las invitaciones del grupo. Guarda esos archivos y
la base de datos en un lugar privado. Las invitaciones se almacenan con hash;
la clave administrativa y la clave TLS necesitan protección del usuario del SO.

## Compilar

Windows: prepara las fuentes/dependencias con `scripts/setup-multip-server.ps1`
y ejecuta `scripts/build-multip.ps1`. Linux: utiliza
`scripts/build-multip-linux.sh`; incluye su instalador de dependencias. Ambos
scripts generan salidas separadas de las ediciones habituales.
