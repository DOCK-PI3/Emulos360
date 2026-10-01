# Emulos360 en Ubuntu 26.04 (x86-64)

## Instalar o actualizar

Usa el archivo `Emulos360-MultiP-linux.tar.gz`: extraerlo **desde Ubuntu** conserva los permisos. Guarda el programa en tu carpeta personal; una unidad montada con `noexec` no permite ejecutar sus herramientas. Los juegos pueden permanecer en otra unidad.

En una terminal, dentro de la carpeta extraída:

```sh
bash install-dependencies.sh
./Emulos360
```

El instalador instala las bibliotecas y 7-Zip, repara los permisos del motor, 7z, ISO2GOD, Node y MongoDB, y comprueba sus bibliotecas. La aplicación se abre como usuario normal, sin `sudo`.

Si ya tienes una instalación con perfiles y partidas, cierra la aplicación y ejecuta desde la nueva carpeta:

```sh
bash update-linux.sh "/ruta/a/tu/Emulos360-anterior"
bash "/ruta/a/tu/Emulos360-anterior/install-dependencies.sh"
```

La actualización copia únicamente la aplicación y los recursos. Conserva la carpeta `data` de la instalación anterior, incluidos perfiles, partidas, ajustes y carátulas. La build nueva no contiene cuentas de Windows.

Los recursos originales de avatares importados por el propietario están en `assets/avatar-system`. El editor usa ese catálogo si no existe uno propio en `data/avatar-system`.

## Si el motor no abre un juego

Linux utiliza Vulkan y la ventana del motor se abre por separado mediante Xwayland. Instalar las bibliotecas Vulkan no sustituye el controlador de la tarjeta gráfica. Para NVIDIA, el controlador de Ubuntu debe incluir su componente Vulkan; AMD e Intel utilizan los controladores Mesa instalados por el script.

Un cierre con señal 9 significa que el proceso recibió SIGKILL: puede ser un cierre forzado o que el sistema lo haya terminado por memoria. El número por sí solo no identifica la causa del bloqueo previo.

Después de un intento fallido:

```sh
bash diagnose-linux.sh
```

Adjunta el archivo `diagnostico-linux-*.txt`, `data/core.log` y `data/core-console.log`. El último archivo también registra los mensajes que salen antes de que se inicialice el registro del motor. El diagnóstico no copia perfiles, partidas ni invitaciones privadas.
