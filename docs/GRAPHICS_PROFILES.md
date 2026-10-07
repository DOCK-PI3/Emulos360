# Perfiles gráficos globales y por juego

Implementación: **5 de octubre de 2026**, interfaz del proyecto Emulos360.
Se utiliza el único core Xenia Canary Netplay existente. No se modifica ni
recompila el motor de emulación para añadir estos selectores.

## Uso

En **Ajustes → Gráficos** o **Ajustes → Pantalla**, seleccionar «Perfil gráfico
global». También aparece en Ajustes → Emulos360 → Gráficos. La selección se
guarda inmediatamente y se aplica al próximo arranque; cerrar antes el juego.

En **Biblioteca → seleccionar juego → Gráficos**, elegir un perfil y pulsar
Guardar. «Usar perfil global» hereda cambios posteriores del global. El botón
«Usar globales» elimina tanto el perfil como la GPU específicos del título.
La GPU puede seleccionarse independientemente del perfil.

| Perfil | Escala interna X/Y | Escalado/nitidez | CAS adicional | Pasadas FSR máximas |
| --- | --- | --- | --- | --- |
| Personalizado · ajustes del motor | Configuración existente | Configuración existente | Existente | Existentes |
| Predeterminado · original | 1 / 1 | Bilineal, sin nitidez añadida | 0 | 4 |
| 720p mejorado | 1 / 1 | CAS | 0,15 | 4 (sin efecto con CAS) |
| 1080p · FSR | 1 / 1 | FSR 1.0 | 0 | 1 |
| 1440p · equilibrado | 2 / 2 | CAS | 0,1 | 2 (sin efecto con CAS) |
| 4K · FSR | 2 / 2 | FSR 1.0 | 0 | 2 |
| 4K · calidad | 3 / 3 | CAS | 0 | 2 (sin efecto con CAS) |

Los perfiles de mejora solicitan FXAA normal; **solo el backend Direct3D 12
lo aplica en esta revisión**. Vulkan dispone de CAS/FSR, pero no implementa
esa etapa FXAA. Predeterminado usa los valores originales del catálogo:
antialiasing y escalador vacíos, CAS 0, FSR máximo 4, reducción de nitidez FSR
`0.20000000298023224` (0,2 representado como float) y dithering activado.
Los demás perfiles conservan esos dos últimos valores.

Las resoluciones son **referencias desde una imagen de 1280×720**. No se
cambia la resolución del escritorio ni se fuerza el tamaño del área de juego:
la presentación sigue la ventana/monitor. Un juego que renderiza a otra
resolución tendrá otro resultado. Canary permite escalas enteras; 1080p usa
escalado espacial de la imagen nativa, no una escala interna 1,5×. La GPU puede
limitar 2×/3× si no admite los recursos sparse/tiled necesarios. FSR solo
amplía cuando el destino es mayor que la imagen de origen; en otros casos
utiliza CAS. No es generación de fotogramas ni garantiza mejores FPS.

Son puntos de partida de calidad/coste, **sin optimización medida por juego**.
2× y 3× pueden alterar efectos o consumir demasiada memoria en ciertos títulos.
Volver a Predeterminado permite probar a 1× conservando sus ajustes específicos
de compatibilidad. Para recuperar la configuración manual previa, usar
Personalizado. Una instalación existente comienza en Personalizado.

## Alcance y persistencia

`data/cores.ini` almacena `graphics/defaultProfile` y
`games/<TITLE_ID>/graphicsProfile`. Un ID desconocido cae a Personalizado o al
global válido; no se aceptan títulos ni perfiles arbitrarios.

Los ocho ajustes de un perfil se envían como argumentos **solo al iniciar
juegos**, con precedencia sobre el TOML global y las configuraciones nativas
por título. No se sobrescriben esos archivos para instalar un perfil. El core
serializa los valores de configuración, no los overrides de línea de comandos
(inspección de `base/cvar.h` y `config.cc`). Una modificación explícita desde el
menú del propio core durante la sesión puede sustituir un override y guardarse
según sus reglas habituales.

Se preservan backend de renderizado, EDRAM, readback, oclusión, parches, CPU,
sincronización, perfiles, partidas, contenido y servicios Netplay. Las tareas
de perfiles/logros y el dashboard no reciben los argumentos del preset.
Los campos manuales controlados por el perfil muestran el valor seleccionado
y quedan bloqueados hasta elegir Personalizado; el resto sigue editable.

## Evidencia y límites

Las pruebas añadidas cubren persistencia, herencia, vuelta a los valores
originales, selección manual independiente por título, IDs inválidos,
bloqueo durante sesiones, los botones Guardar/Usar globales y los campos
controlados por perfil. El stub de arranque comprueba los argumentos efectivos,
las rutas de datos y Netplay privado, y conserva byte a byte TOML y Account de
fixtures temporales. No inicia un juego real ni demuestra calidad visual.

Validación completada el 5 de octubre de 2026:

- Interfaz Release compilada en Windows (Qt 6.8.3/MSVC) y Ubuntu 26.04
  (Qt 6.10.2/GCC).
- CTest **4/4** en ambas plataformas: preferencias, biblioteca, interfaz y voz.
  Los tres casos de interfaz específicos de perfiles pasan en ambas.
- CoreOptions: **10 casos pasando**, incluidos inicialización/finalización,
  en ambas plataformas y con **ASan/UBSan + detección de fugas** (Clang 21).
  Sin errores de sanitizadores ni avisos nuevos en ese código.
- UiTests: Windows 19 pasan/5 omitidos; Ubuntu 18 pasan/6 omitidos. Los casos
  omitidos preexistentes requieren recursos privados, entorno o plataforma.
- Capturas globales y por juego revisadas en Windows/Linux. Pruebas y capturas
  conservadas en `local/graphics-profiles-20261005/` (ignorado en Git).

Reproducción Windows: configurar CMake con Qt y `BUILD_TESTING=ON`, compilar
Release y ejecutar `ctest --test-dir build-graphics-profiles -C Release
--output-on-failure`. Ubuntu: la misma configuración en Ninja/Release y
`ctest --test-dir <build> --output-on-failure`. Para los sanitizadores, compilar
`tests/core_options_tests.cpp` + `app/core_options.cpp` con AUTOMOC, Qt
Core/Gui/Qml/Test y `-fsanitize=address,undefined -fno-omit-frame-pointer`.

Gameplay, consumo de VRAM y FPS por título no se han medido; corresponden a
pruebas posteriores en hardware real. No se crea ni publica una release en
esta tarea; la versión del proyecto continúa en 0.5.0.

## Referencias verificadas

- Código local `gpu/texture_cache.cc`: escalas enteras y límites de GPU.
- Código local `app/emulator_window.cc`, `ui/presenter.cc` y presentadores
  D3D12/Vulkan: controles y recorrido CAS/FSR; FXAA en el command processor D3D12.
- [Explicación de presentación de Xenia](https://xenia.jp/updates/2022/01/29/presenting-the-presentation-update-amd-fidelityfx-fsr-fxaa-cas-vrr.html):
  mecanismos y limitaciones de presentación; contrastada con el código actual.
- [AMD GPUOpen: FSR 1](https://gpuopen.com/fidelityfx-superresolution/): escalado
  espacial y compatibilidad de GPU, incluida NVIDIA. No se añade DLSS ni otra
  generación de FSR en esta integración.
