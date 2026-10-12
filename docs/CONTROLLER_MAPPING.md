# Mapeo completo del mando

Implementación del 11 de octubre de 2026. Ajustes → Mandos → **Mapear mando**.

La ventana detecta dispositivos USB/Bluetooth mediante SDL2 y el tipo anunciado
por el driver: Xbox 360, Xbox One, PS3, PS4, PS5 o genérico. Un adaptador, Steam
Input o un driver virtual puede ocultar el modelo físico: el selector de imagen
permite corregir la representación sin alterar la detección ni el mapa.
Los dibujos son vectoriales propios, integrados en QML y escalables.

## Uso

1. Conectar el mando y elegirlo en la lista si hay varios.
2. Probar botones, cruceta, clics y desplazamientos de ambos sticks y gatillos.
   Los controles se iluminan; los sticks se desplazan y los gatillos muestran
   intensidad. El texto «En directo» ofrece la misma información.
3. Para cambiar una asignación, pulsar su control en el dibujo o su fila en la
   lista y accionar el control físico deseado. Las cuatro direcciones de cada
   stick se asignan individualmente desde la lista.
4. **Asistente paso a paso** recorre los 25 controles de Xbox 360: 15 botones
   (incluida Guía), dos gatillos y ocho direcciones analógicas. Espera a soltar
   los controles entre pasos. **Omitir** conserva la asignación previa;
   **Detener** permite continuar manualmente. Los botones adicionales del mando
   físico pueden asignarse a cualquiera de estas funciones.
5. **Guardar mapa** aplica los cambios al iniciar la próxima sesión. Cerrar
   sin guardar los descarta. **Restaurar mapa** y después Guardar recupera el
   mapa estándar y elimina la personalización de ese modelo.

Una asignación física que ya se utiliza intercambia las funciones implicadas.
El archivo distingue modelos por GUID SDL; dos mandos con el mismo GUID
comparten el mapa. Antes de cambiar de dispositivo hay que guardar o descartar
el borrador; desconectarlo descarta los cambios sin guardar. Al desconectar el seleccionado no se sustituye automáticamente
por otro mando conectado. Guía/PS puede estar reservado por el sistema operativo;
el asistente permite omitirlo. La recepción física de PS3 depende del driver
instalado; no se instala ningún driver desde esta función.

La ventana se abre con ratón/teclado. Mientras está abierta se suspende la
navegación del launcher por mando para que A/B/cruceta no activen la interfaz.
No puede abrirse durante una sesión de juego. Escape o Cerrar salen del diálogo.

## Integración con el motor existente

El mapa se escribe atómicamente en
`data/controllers/gamecontrollerdb.txt`, separado del TOML de ajustes del core.
Se conservan las entradas de otros modelos. Sin entradas personalizadas no se
añaden argumentos de entrada al lanzamiento.

Cuando existen mapas personalizados, los lanzamientos de juego/dashboard reciben
`--mappings_file=<ruta absoluta>` y `--hid=any`. La fábrica `CreateAll` del fork
Netplay actual omite XInput en `any` e incluye SDL y teclado; esto permite usar
el mapa sin duplicar los mandos ni retirar la entrada por teclado. Las operaciones
del helper de perfiles/logros no reciben estos argumentos. No se recompila ni
modifica el core, el contenido, los perfiles ni los guardados.

El formato procede de la [API oficial de mapeos SDL2](https://wiki.libsdl.org/SDL2/SDL_GameControllerAddMapping)
y de la implementación local de `SDL_gamecontroller.c` (SDL 2.32.2). Se preservan
medios ejes con signo, ejes completos de gatillos y las máscaras cardinales de
la cruceta. `SDLInputDriver::LoadGameControllerDB` del core carga este formato.
En Windows la interfaz enlaza SDL2 estáticamente desde la dependencia ya presente
en el submódulo. Los empaquetadores incluyen su licencia. En Linux usa SDL2 del
sistema (mínimo 2.0.14).

## Verificación y límites

`gamepad_mapper_tests` utiliza un joystick virtual SDL para verificar captura,
botones, cruceta, ejes positivos/negativos, gatillos, espera de liberación,
cancelación, desconexión, persistencia y restauración. El mapa producido también
se carga con `SDL_GameControllerAddMapping` y se comprueba la entrada normalizada
que recibe un consumidor SDL. Las pruebas de interfaz cargan y renderizan las
cinco familias y un tamaño reducido. La prueba de lanzamiento usa el helper
simulado para comprobar argumentos y conservar configuración/Netplay/partidas.

Estas pruebas no son gameplay ni una prueba física de cinco modelos diferentes.
El propietario debe probar su mando real y una sesión de juego en su instalación.
El core estable y la instalación personal permanecen sin sustituir. No se hace
commit, push ni publicación.

Resultado local Windows: build Release y arranque correctos; CTest **5/5**,
mapeador con AddressSanitizer **6 casos aprobados**, sin errores de memoria.
La suite UI aprueba 20 casos y omite 5 integraciones opcionales que requieren
servidor, perfil de prueba o catálogos en vivo. Las capturas con fuentes Windows se revisaron para las cinco
familias, 760×800 y el mínimo de la aplicación (960×640). El contenido permite
desplazamiento en ventanas pequeñas y mantiene Guardar/Cerrar accesibles.

Build portable de revisión: `out/Emulos360-Mandos/Emulos360.exe`. Usa datos propios
y contiene el mismo core que `out/Emulos360-MultiP`, comprobado por SHA-256.
Evidencia y capturas locales: `local/gamepad-evidence.json`,
`local/gamepad-captures`, `local/gamepad-final-ctest.log` y
`local/gamepad-asan-results-final.txt`. Esta build es de revisión; no sustituye
la instalación personal ni los paquetes de la release pendiente.
