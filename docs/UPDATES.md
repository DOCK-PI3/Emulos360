# Actualizaciones desde GitHub: diseño pendiente

La comprobación automática aún no está implementada. La versión de la
aplicación se define una sola vez en `CMakeLists.txt`, se muestra en **Acerca
de** y responde a `Emulos360 --version`. El futuro publicador usará etiquetas
`v<versión>` y adjuntará a cada GitHub Release un paquete Windows, un paquete
Linux, sumas SHA-256 y notas de cambios.

Flujo previsto:

1. Al arrancar, consultar la última versión estable de GitHub en segundo plano,
   con tiempo límite y sin impedir el uso sin Internet.
2. Comparar versiones y mostrar un diálogo con las novedades. El usuario elige
   **Actualizar** o **Más tarde**; no se descarga nada por adelantado.
3. Descargar el paquete de su sistema en un área temporal y comprobar su tamaño
   y SHA-256 publicado. Rechazar archivos incompletos o de otra plataforma.
4. Cerrar el programa, sustituir solo archivos de aplicación mediante un
   proceso auxiliar y conservar `data/`, perfiles, partidas, juegos y ajustes.
   Si falla la sustitución, restaurar la versión anterior.
5. Reiniciar Emulos360 y mostrar las novedades de la versión instalada una sola
   vez. La opción de actualización y los diagnósticos estarán en Ajustes.

Windows necesita un ayudante externo para reemplazar el ejecutable después de
cerrarlo. En Linux el paquete puede instalarse en una carpeta del usuario, pero
debe respetar permisos y montajes `noexec`. El servidor MultiP, si está
instalado, requiere detenerse y reiniciarse durante la sustitución. La fuente
de las notas será el cuerpo del GitHub Release, con una copia local incluida en
el paquete para poder mostrarlas después de reiniciar sin conexión.

El paso siguiente será acordar el nombre y la URL del repositorio, definir el
formato de los paquetes de Release y probar una actualización entre dos
versiones de prueba con datos personales simulados antes de activarla para
usuarios reales.
