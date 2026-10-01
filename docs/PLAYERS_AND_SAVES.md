# Perfiles, avatares, comunidad y partidas

## Uso

1. En **Perfiles y avatares**, pulsa **Crear perfil** con un gamertag de 1–15
   caracteres: letras, números y espacios, empezando por una letra. Para leer
   perfiles existentes en la carpeta de contenido, pulsa **Actualizar perfiles**.
2. Pulsa **Seleccionar**. Este perfil ocupará el puesto del jugador 1 al abrir
   un juego. El XUID vincula sus partidas; cambiar de perfil no las transfiere.
3. **Imagen de perfil** permite elegir tono de piel, ropa y estilo, o importar una imagen.
   Es la imagen local de Emulos360. El botón separado del editor 3D abre el XEX
   que el usuario seleccione; los avatares originales dentro de juegos siguen
   pendientes. Recursos y bloqueo técnico: [AVATAR_SYSTEM.md](AVATAR_SYSTEM.md).
4. En **Biblioteca**, selecciona un juego GOD o XBLA y pulsa **Jugar**. El juego se abre en la
   ventana del núcleo alojada dentro de Qt en Windows, con los ajustes guardados.
5. Guarda o continúa desde el menú del juego. Después de cerrar el motor,
   **Partidas → Actualizar → Crear copia** conserva los guardados de ese título
   y perfil. **Copias de seguridad → Restaurar** recupera una copia sin borrar
   definitivamente la versión anterior.
6. En **Comunidad**, importa el paquete completo de XBGuard eligiendo HDD o USB.
   Se muestran los módulos y su orden, pero este servicio todavía no puede
   ejecutarse con la base actual. Véase [XBGUARD.md](XBGUARD.md).
   Para otras aplicaciones independientes, selecciona un `.xex` y pulsa **Abrir aplicación**.
   El estado de autenticación se comprueba dentro del cliente; Emulos360 no
   muestra un inicio de sesión exitoso por el mero hecho de abrir un proceso.

## Logros

Los logros se guardan en el perfil seleccionado cuando el juego los concede.
Los avisos en pantalla están desactivados por defecto: con la sesión cerrada,
abre **Ajustes**, busca **Notificaciones de logros**, activa la opción y pulsa
**Guardar** antes de volver a abrir el juego (`UI.show_achievement_notification`).
Esta opción controla el aviso, no el registro del logro, y no vuelve a anunciar
los que ya estaban desbloqueados.

Para consultarlos desde la biblioteca, entra en **Logros**, disponible en el menú
lateral de escritorio y en el dashboard de consola. Muestra el progreso del perfil
seleccionado, los puntos y el detalle por juego, con búsqueda, filtros de
desbloqueados/pendientes, fechas e iconos. Los secretos permanecen ocultos hasta
desbloquearlos o activar **Mostrar secretos**. **Actualizar** vuelve a leer el
perfil; al cerrar una sesión se actualiza automáticamente la pantalla visible.
Mientras el juego sigue abierto se conserva la última lectura disponible.
Si falta el GPD de un juego, conserva su resumen e indica que no hay detalle;
un historial ilegible se informa como error, no como cero logros.

También puedes abrir un juego con tu perfil y utilizar el menú del motor
**Profile → Show Profile Menu**. Haz clic derecho sobre el perfil conectado,
elige **Show Played Titles** y selecciona un juego para ver sus logros y puntos.
La consulta usa el lector GPD del motor sin iniciar sesión ni modificar los logros.

## Archivos locales predeterminados

Desde 0.13, **Partidas** permite gestionar las unidades interna y externa por
separado. La elección en esta pantalla solo cambia la unidad que estás viendo.
El juego abre su propio selector con **Disco interno / Disco externo** cuando
solicita almacenamiento; los títulos que recuerdan una unidad pueden no preguntarlo
en cada guardado. El interno conserva los guardados anteriores.

El externo usa `data/engine/external-content` por defecto: es un disco virtual,
no implica que haya un USB físico. **Configurar externo…** permite elegir una
carpeta de tu USB/HDD. Cambiarla no mueve ni borra partidas. Si esa carpeta no está
conectada, la unidad no se ofrece en el selector y no se redirige al interno.
Las dos carpetas deben ser independientes. Los perfiles permanecen en el interno.

Las copias nuevas contienen guardado y cabeceras `Headers/00000001`, con hashes.
Las copias externas quedan en `data/save-backups/external`; las internas conservan
su ubicación anterior. Las versiones previas se mantienen junto al destino al
restaurar. No se implementan estados instantáneos de CPU/GPU/RAM.

Para Netplay: abre **Comunidad → Abrir gestor Netplay**. En el gestor de perfiles
del núcleo, el menú del perfil ofrece **Convert to Xbox Live-Enabled Profile**.
Es el formato de perfil que utiliza la red comunitaria, no un acceso a los servicios
de Microsoft. La conversión se realiza con las funciones originales de Netplay;
no se convierten automáticamente los perfiles del usuario. Configura servidor y
red en **Netplay → Settings** y amigos en **Netplay → Manager → Friends**.

| Ruta relativa al frontend | Contenido |
| --- | --- |
| `data/engine/xenia-canary.config.toml` | Opciones persistentes del motor |
| `data/engine/xconfig.settings` | Ajustes binarios de consola |
| `data/engine/content/<XUID>/FFFE07D1/00010000/` | Perfil real de Xenia |
| `data/engine/content/<XUID>/<TitleID>/00000001/` | Partidas nativas |
| `data/avatars/` y `data/players.ini` | Imágenes locales y preferencias de clientes |
| `data/save-backups/<id>/` | Manifiesto y copia de las partidas |

Si se configura `Storage.storage_root` o `Storage.content_root`, se usan las
rutas efectivas de Ajustes. `--data-dir` permite un entorno local separado.
Para usar perfiles/partidas de otra instalación, elige su carpeta de contenido
en Ajustes → Almacenamiento y después actualiza los perfiles. No hay migración
automática que sobrescriba perfiles existentes.

Los guardados internos funcionan en la medida en que cada juego funciona en
Xenia. Esta integración no demuestra la compatibilidad de las campañas de Gears.
El inicio de sesión de XBGuard sigue pendiente del soporte del motor descrito
en XBGUARD.md. Los avatares 3D completos también siguen pendientes.
