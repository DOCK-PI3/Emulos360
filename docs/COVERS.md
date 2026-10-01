# Carátulas de Emulos360

El selector nativo se abre por juego y consulta hasta ocho opciones. Se identifica
el título con los ocho dígitos de su Title ID; no se envían rutas locales ni juegos.

| Grupo | Mercados e idiomas |
| --- | --- |
| Europa | España/es-ES, Reino Unido/en-GB, Francia/fr-FR, Alemania/de-DE |
| América | Estados Unidos/en-US, México/es-MX |
| Asia | Japón/ja-JP |
| Sin especificar | Archivo x360db, sin asumir una región para la imagen |

La región describe el mercado del recurso solicitado, no la región del paquete
GOD/XBLA. Varios mercados pueden devolver la misma ilustración. Una imagen inexistente
o un fallo de red se muestra sin sustituirla silenciosamente por otra región.

Fuentes:

- Xbox Marketplace, ruta pública histórica:
  `http://download.xbox.com/content/images/66acd000-77fe-1000-9115-d802{titleid}/{lcid}/boxartlg.jpg`.
  Ese servidor funciona por HTTP; su certificado HTTPS presentó un nombre
  incompatible en la consulta de implementación. No se desactiva la validación TLS.
- [x360db](https://github.com/xenia-manager/x360db/blob/main/docs/USAGE.md),
  portada archivada por Title ID sobre HTTPS.
- Referencia de mercados de [Aurora Asset Editor](https://github.com/XboxUnity/AuroraAssetEditor/blob/master/AuroraAssetEditor/Classes/XboxAssetDownloader.cs).
  Implementación C++ propia, sin copiar código de clientes externos.

Solo se descargan las opciones del juego abierto en el selector, con tres
solicitudes simultáneas como máximo. Cerrar cancela las descargas pendientes.
El usuario confirma la portada; no se cambia toda la biblioteca automáticamente.

Si no hay una portada adecuada, en **Biblioteca → Carátula → Elegir imagen del PC**
se puede seleccionar un PNG, JPEG o BMP. Emulos360 la convierte a PNG,
guarda una copia optimizada en `data/covers` y recuerda la elección por Title ID.
El archivo original puede moverse o borrarse después. Se aceptan archivos de
hasta 32 MiB con dimensiones entre 64 y 8192 píxeles por lado; un archivo
inválido no sustituye la carátula anterior.

Las imágenes se decodifican antes de guardarlas como PNG en `data/covers`.
Límites: 4 MiB por respuesta, dimensiones entre 64 y 4096 píxeles, 15 segundos
por petición. Se usa escritura atómica; la selección y procedencia quedan en
`data/settings.ini`. Las imágenes cacheadas se reutilizan sin conexión.
Los derechos de las ilustraciones pertenecen a sus respectivos titulares.

Disponibilidad consultada durante la implementación: Gears of War, mercados
español y japonés, y archivo x360db. No se presupone cobertura de todo el catálogo.
