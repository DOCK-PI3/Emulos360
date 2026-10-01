# Recuperación de perfiles y partidas desde HDD y USB

Las herramientas `scripts/extract-hdd-profile.py` y
`scripts/recover-usb-saves.py` leen copias de unidades Xbox 360 y escriben
contenedores en una carpeta nueva. No cambian el Account ni reasignan partidas
a otro perfil. Las rutas de origen, los identificadores y los manifiestos de
una recuperación personal se mantienen fuera de este repositorio.

## Identificar el perfil

Un contenedor puede mostrar un identificador como nombre visible. La identidad
se debe comprobar con el Account leído por el núcleo mediante el comando
`emulos_profile_command=list`. La carpeta usada para identificar la copia debe
estar separada de la carpeta de juego. Crear un perfil nuevo con el mismo
gamertag produce otro XUID y no recupera las partidas ni los logros anteriores.

`extract-hdd-profile.py` recibe `--accounts` con el TOML de identificación,
`--gamertag` y `--destination`. Sin coincidencia única no extrae partidas; sin
destino solo informa del tamaño. `recover-usb-saves.py` valida tipo de
contenido, Title ID y XUID frente a la ruta. Ambos conservan los originales en
solo lectura y comprueban SHA-256 al copiar.

## Importar en Emulos360

El comando interno `emulos_profile_command=import-content` inspecciona el
contenedor XContent, admite perfiles y partidas, rechaza enlaces y no
sobrescribe archivos ya instalados. Conviene hacer una copia de `data/engine`
antes de importar. El perfil recuperado se selecciona en **Perfiles y avatares**;
los juegos pueden solicitar después **Disco interno** o **Disco externo**.

La importación conserva los archivos y sus cabeceras. La carga efectiva de cada
partida depende de que coincidan el juego, la versión, la actualización y el
contenido adicional. Estas copias no son estados instantáneos de CPU/GPU/RAM.
