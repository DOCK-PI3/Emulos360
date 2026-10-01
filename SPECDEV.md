# SPECDEV — Emulos360

Versión: 0.17 · Fecha: 2026-09-22 · Estado: biblioteca GOD/XBLA y recuperación de perfiles y partidas integrados; servicios de avatar dentro de juegos pendientes.

Estado actual y límites verificados: **[docs/STATUS.md](docs/STATUS.md)**.
Los apartados fechados de prototipos anteriores son un registro histórico.

## Prioridad actual confirmada

- Nombre definitivo: **Emulos360**.
- Integrar Xenia Canary Netplay: base cambiada a `6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c`, manteniendo las extensiones propias. Detalles: `docs/NETPLAY_OPTIONS.md`.
- Selector de almacenamiento interno/externo con rutas independientes, metadatos y copias por unidad.
- Dashboard nativo de consola inspirado en Xbox 360, con guía y navegación XInput en Windows.
- Reunir los ajustes persistentes de Xenia en categorías dentro de la interfaz.
- Crear perfiles, personalizar avatares y gestionar guardado/carga de partidas.
- Preparar acceso a clientes XEX comunitarios; identificar servicio, ejecutable y requisitos antes de prometer autenticación.
- Documentar cada implementación propia y conservar por separado los parches del motor: `docs/FORK_CHANGES.md`.
- Priorizar el diseño nativo y las carátulas descargables con opciones por región.
- No repetir pruebas del motor upstream sin cambios. Limitar la verificación a
  cambios propios cuando lo justifiquen; no lanzar suites o juegos por rutina.
- Compilar y empaquetar sin pruebas por defecto. La compatibilidad de un título
  no se deduce de que upstream haya sido probado por otras personas.
- Portadas con vista previa, filtro por región, procedencia y caché local.
  Variantes por mercado/idioma no garantizan ilustraciones diferentes.

## 1. Decisiones confirmadas

- Crear un proyecto nuevo en esta carpeta.
- Empezar con un emulador multiplataforma como aplicación.
- Evaluar y reutilizar componentes de proyectos existentes.
- Crear un fork propio de un emulador existente, integrar la interfaz Qt Quick y modificar el motor para trabajar en compatibilidad y rendimiento. Base inicial de evaluación: Xenia Canary, fijada en el repositorio local.
- Dar prioridad a Windows x64 y abordar Linux después.
- Interfaz nativa, sin tecnologías web: excluir Electron, Tauri/WebView y HTML/CSS como implementación de la interfaz.
- Interfaz elegida por el usuario: Qt 6 Quick con C++ y QML; SDK local Qt 6.8.3.
- Dos modos de interfaz confirmados: escritorio para ratón y teclado, y consola a pantalla completa para mando; ambos comparten biblioteca y configuración.
- Prioridad de compatibilidad: toda la saga Gears of War publicada para Xbox 360 (Gears of War, Gears of War 2, Gears of War 3 y Gears of War: Judgment).
- Primera modalidad prioritaria: campaña individual de los cuatro títulos; validar gráficos, audio, estabilidad y guardado.
- Biblioteca de pruebas declarada: `D:\XBOX360_GAMES`, disco duro de 2 TB, juegos en formato GOD. Gears of War 3 pendiente de descarga por el usuario desde su backup privado.
- Priorizar rapidez de respuesta y aspecto moderno, aceptando consumo de RAM y GPU cuando aporte fluidez. Aspiración visual: superar la interfaz de Xenia.
- Definir las especificaciones junto al usuario antes de implementar el motor.
- Preparar skills para los lenguajes y para el diseño de interfaz.

## 2. Visión y alcance

Construir un emulador de Xbox 360 con una interfaz propia y una arquitectura que permita evolucionar en compatibilidad y rendimiento. Un sistema dedicado que lo integre puede estudiarse después.

«Emulación a full» es la aspiración, no un criterio de aceptación definido ni una garantía de compatibilidad universal. Debemos concretar juegos prioritarios, hardware de referencia, fidelidad, velocidad y estabilidad. Cada resultado se vinculará a una versión, configuración y prueba reproducible.

El primer alcance no incluye construir un kernel de sistema operativo. El soporte de dashboard, periféricos especiales, red y servicios de Xbox está pendiente de definición y no se presume incluido en el primer prototipo.

## 3. Decisiones abiertas

| Decisión | Propuesta inicial | Estado |
| --- | --- | --- |
| Motor | Xenia Canary Netplay, revisión 6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c | Integrado con Qt mediante núcleo propio, con extensiones documentadas |
| Plataformas | Windows x64 primero; Linux después | Confirmado |
| Hardware de referencia | Radeon RX 6600 XT, 8 GB VRAM; 16 GB RAM; el registro del equipo indica Intel Core i5-11400F a 2,60 GHz | GPU y RAM declaradas; CPU identificada localmente |
| Lenguajes | C++20 y QML en el prototipo; conservar el C++ del motor; Rust aplazado hasta justificar un componente | Implementado en la interfaz |
| Gráficos | Evaluar Vulkan; Direct3D 12 si aporta una ventaja comprobada en Windows | Propuesta |
| Ventana, mandos y audio del host | Evaluar SDL3 | Propuesta |
| Interfaz | Qt 6.8.3 Quick con C++ y QML, sin Qt WebEngine ni WebView | Prototipo compilado y renderizado en RX 6600 XT |
| Nombre y licencia | Emulos360; conservar licencias y atribuciones upstream | Nombre confirmado; licencia propia pendiente |
| Juegos prioritarios | Campaña individual de Gears of War, Gears of War 2, Gears of War 3 y Gears of War: Judgment, versiones Xbox 360 | Títulos y modalidad confirmados; métricas pendientes |
| Recursos | Concretar experiencia, tiempo disponible y colaboradores | Pendiente |

No quedan elegidas versiones de compiladores, bibliotecas ni APIs por instalar sus skills.

## 4. Estrategia del motor

### Decisión confirmada: fork propio

Partir del código de un emulador existente, integrar la interfaz Qt Quick y desarrollar cambios propios en el motor. La evaluación inicial eligió Xenia Canary como base de trabajo, conservando Xenia principal como referencia. La revisión fija y los motivos están documentados en docs/ARCHITECTURE.md. No se ha creado un repositorio remoto ni se presume compatibilidad de los cuatro Gears por elegir esta base.

Propuesta de mantenimiento: conservar la procedencia y los avisos del código reutilizado, registrar la revisión de partida, mantener identificables los cambios propios y facilitar la incorporación de correcciones de la base. Validar primero una compilación de referencia antes de modificar el comportamiento del motor.

Se mantiene un motor derivado de Netplay con extensiones propias y una interfaz Qt
en otro proceso. El puente propio reutiliza sus perfiles y la capa de contenido;
incrustar la superficie gráfica del juego en Qt sigue pendiente.

## 5. Arquitectura propuesta para evaluar

Separar interfaz, servicios del host y emulación mediante contratos pequeños y comprobables:

- Aplicación: biblioteca de juegos, configuración, mandos, inicio y cierre de sesión, mensajes de error.
- Núcleo: ciclo de ejecución y coordinación de subsistemas.
- CPU y memoria: estado del procesador invitado, acceso a memoria y traducción de instrucciones; comparar intérprete y JIT según la base elegida.
- Cargador y servicios invitados: carga de ejecutables y comportamiento requerido por los programas.
- GPU: interpretación de comandos y shaders invitados; traducción al backend gráfico del host.
- Audio: comportamiento invitado y salida de audio del host.
- Entrada y almacenamiento: mandos, archivos y partidas.
- Diagnóstico: registros, trazas, capturas y métricas reproducibles.

Esta división es conceptual. Si se reutiliza un motor, respetar su arquitectura antes de plantear una reescritura. SDL3 ayuda con servicios del host; no implementa la emulación. Vulkan y Direct3D son APIs gráficas, no lenguajes. Rust y C++ no deben mezclarse sin una frontera justificada y reglas de propiedad de memoria.

## 6. Hitos y criterios de salida propuestos

### Material de prueba disponible

La biblioteca está en `D:\XBOX360_GAMES`, en un disco duro de 2 TB. El lector
actual identifica 16 paquetes GOD y 14 XBLA. Los cuatro Gears de Xbox 360 están
presentes; esto acredita inventario, no campañas completas ni compatibilidad.

Ruta comprobada mediante listado de directorios: D:\XBOX360_GAMES. La ruta inicialmente indicada incluía una barra adicional entre XBOX360 y _GAMES. Dentro de gow1-2y_judgement se identificaron cabeceras con Title IDs 4D5307D5, 4D53082D y 4D530A26. Se comprobó la presencia básica de fragmentos; integridad completa, edición y actualización siguen sin validarse.

Requisito de evaluación: comprobar soporte y carga del formato GOD en la base elegida. Mantener los juegos fuera del repositorio; usar la biblioteca como fuente y almacenar diagnósticos y datos generados en rutas separadas. No modificar ni convertir los paquetes originales como parte del inventario.

### Validación por hitos

Suite prioritaria de juegos: Gears of War, Gears of War 2, Gears of War 3 y Gears of War: Judgment en sus versiones Xbox 360. Esta selección establece objetivos de evaluación, no compatibilidad demostrada. Registrar edición, actualización y contenido adicional en cada prueba. Modalidad inicial confirmada: campaña individual, verificando gráficos, audio, estabilidad y guardado/carga. Cooperativo local y funciones de red quedan fuera del primer alcance; su inclusión posterior no está decidida. Las entregas para otras plataformas no quedan incluidas en el alcance del emulador Xbox 360.

1. **Especificación acordada:** motor, primera plataforma, hardware, juegos, licencia y alcance de interfaz decididos.
2. **Viabilidad técnica:** compilar y medir la revisión base elegida para el fork antes de introducir cambios propios. Documentar resultados y límites; distinguir pruebas realizadas de reportes externos.
3. **Aplicación mínima:** ventana, configuración persistente, detección de mando y registros de errores; comprobar inicio y cierre limpios. Esto aún no demuestra emulación.
4. **Primera ejecución integrada:** completar una prueba invitada controlada con resultado reproducible. Definir la prueba concreta tras elegir el motor.
5. **Compatibilidad inicial:** evaluar los títulos elegidos y registrar arranque, gráficos, audio, entrada, guardado, fallos y tiempos de cuadro.
6. **Portabilidad:** repetir pruebas acordadas en la segunda plataforma y documentar diferencias.

No fijar FPS ni fechas sin una medición inicial. La velocidad objetivo debe respetar la velocidad original del título; medir también estabilidad de tiempos de cuadro, uso de memoria y sesiones sin fallos.

## 7. Interfaz: requisitos para conversar

Propuesta: biblioteca, añadir rutas, búsqueda, estado de compatibilidad, configuración gráfica, selección de mando y acceso a diagnósticos. Diseñar estados vacíos, errores de carga y navegación con teclado y mando.

Modos confirmados por el usuario:

- Escritorio: biblioteca con carátulas, barra lateral y ajustes, orientada a ratón y teclado.
- Consola: pantalla completa, elementos grandes y navegación con mando.
- Ambos comparten biblioteca y configuración; no son aplicaciones independientes.

Pendiente definir la identidad visual y el mecanismo para cambiar entre modos.

Requisito confirmado: implementación nativa sin navegador ni WebView. Modernidad y fluidez tienen prioridad frente al mínimo consumo absoluto. Evitar trabajo gráfico de la biblioteca mientras esté oculta durante una sesión de juego.

Elección confirmada: Qt 6 Quick con C++ y QML. Las otras opciones quedan como alternativas evaluadas, no como dependencias a instalar.

Propuesta de implementación: QML para presentación y transiciones; C++ para modelos y servicios de interfaz. Mantener exploración de archivos, carga de carátulas y tareas largas fuera del hilo de interfaz. El backend gráfico de Qt y el del emulador se decidirán según las necesidades de integración; no se presupone que deban coincidir.

Comparación conservada como referencia:

| Opción | Encaje propuesto | Coste o límite |
| --- | --- | --- |
| Qt 6 Quick + C++/QML | Primera recomendación para biblioteca visual, transiciones y controles personalizados con renderizado acelerado | Más infraestructura; integrar cuidadosamente el motor y su salida gráfica. QML es declarativo y puede usar expresiones JavaScript, pero no requiere un motor web |
| Slint + Rust o C++ | Alternativa nativa orientada a eficiencia y diseño declarativo personalizado | Validar controles, navegación con mando e integración gráfica mediante un prototipo |
| Dear ImGui + C++ + backend SDL3/gráfico | Integración directa en el renderizado, herramientas y overlays | Un acabado de producto muy personalizado requiere más trabajo; limitaciones de accesibilidad e internacionalización |

La elección no constituye un benchmark: medir arranque, latencia, tiempos de cuadro, memoria y carga de GPU del prototipo. La GPU declarada no establece por sí sola la compatibilidad o velocidad de emulación.

## 8. Skills preparadas

Instaladas en el perfil de Codex; disponibles a partir del siguiente turno. No están copiadas dentro del repositorio y no son dependencias del emulador.

| Skill | Fuente | Uso previsto |
| --- | --- | --- |
| rust-engineer | https://github.com/jeffallan/claude-skills/tree/main/skills/rust-engineer | Código Rust, memoria, errores y validación |
| cpp-pro | https://github.com/jeffallan/claude-skills/tree/main/skills/cpp-pro | C++ e integración con componentes nativos |
| qt-qml | https://github.com/TheQtCompanyRnD/agent-skills/tree/main/skills/qt-qml | Desarrollo QML para la interfaz elegida |
| qt-ui-design | https://github.com/TheQtCompanyRnD/agent-skills/tree/main/skills/qt-ui-design | Diseño de interfaz Qt/QML; aplicar únicamente su alcance nativo |
| frontend-design | https://github.com/anthropics/skills/tree/main/skills/frontend-design | Instalada previamente; las skills Qt son la referencia específica para esta interfaz. No usar tecnologías web |

Existe además xbox360-xdk-system en el perfil, orientada a desarrollo nativo XDK. No equivale a una skill completa para emular Xbox 360 en un PC; solo se evaluará si el trabajo concreto corresponde a su alcance.

Las skills adicionales se seleccionarán al confirmar las tecnologías. Evitar instalar lenguajes o frameworks que no se vayan a utilizar. Los ejemplos de una skill no fijan las versiones del proyecto.

## 9. Referencias iniciales

- Xenia, repositorio del proyecto: https://github.com/xenia-project/xenia
- SDL3, documentación oficial: https://wiki.libsdl.org/SDL3/FrontPage
- Qt Quick: https://doc.qt.io/qt-6/qtquick-index.html
- Renderizado de Qt Quick: https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph.html
- Slint: https://slint.dev/
- Dear ImGui: https://github.com/ocornut/imgui
- Directorio usado para localizar skills: https://skills.sh/

Estas referencias documentan las opciones evaluadas inicialmente; la base actual
integrada es Netplay y su revisión se identifica al principio del documento.

## 10. Próxima conversación

La prioridad es cerrar las limitaciones de [docs/STATUS.md](docs/STATUS.md): carga
real de las partidas recuperadas, sesión Netplay entre participantes, mando físico
y servicios XAM necesarios para mostrar avatares dentro de los juegos.

## 11. Primer avance de implementación — histórico, 2026-09-15

El usuario autorizó comenzar el desarrollo. Se prepararon el repositorio local,
el submódulo Xenia Canary con revisión fija y rama local `los/main`, el SDK Qt
local y la aplicación C++/QML. Nombre de trabajo: LOS 360; no es nombre definitivo.

Funciones implementadas: elegir carpeta, escanear GOD en segundo plano, buscar
por nombre o Title ID, seleccionar juego, alternar escritorio/pantalla completa
y guardar preferencias. Interfaz provisional en español, oscura y con acento
verde. Sin carátulas todavía. Mando físico e integración del motor pendientes.

Comprobaciones realizadas:

- Inventario de la ruta completa: 13 paquetes GOD detectados; no implica que
  otras carpetas sean inválidas, pues el lector solo reconoce GOD bajo 00007000.
- Gears 1: 40 fragmentos; Gears 2: 40; Judgment: 45. Presencia básica comprobada,
  sin validar hashes completos. Gears 3 no se encontró en el inventario GOD.
- Pruebas del lector y de la UI superadas, incluida una corrección de la
  sincronización entre fin del trabajo y publicación de resultados.
- Pruebas del lector con AddressSanitizer superadas. UBSan no ejecutado con MSVC.
- Renderizado de la UI comprobado tanto offscreen como con la RX 6600 XT.
- La biblioteca no ejecuta juegos todavía; no se ha demostrado compatibilidad
  ni mejora de rendimiento en emulación.
- Xenia Canary compilado en Release. En una prueba independiente, el motor base
  mapeó los 40 fragmentos de Gears 1 y lanzó su default.xex utilizando la RX 6600 XT.
  Prueba cerrada; imagen, audio y campaña jugable aún sin validar.

Construcción, arquitectura y limitaciones: [BUILD.md](docs/BUILD.md) y
[ARCHITECTURE.md](docs/ARCHITECTURE.md). El detalle de pruebas y estado de la
compilación del motor se mantiene en [VALIDATION.md](docs/VALIDATION.md).

## 12. Emulos360 — 2026-09-17

Icono definitivo elegido por el usuario: opción 2, **Orbit 360** (órbita verde
y E central). Recursos PNG e ICO compartidos por interfaz y motor.

Nombre definitivo aplicado a ventana, marca de la interfaz y ejecutable
`Emulos360.exe`. El fork genera `Emulos360-core.exe` y muestra Emulos360 en
su ventana, conservando los créditos y las licencias del motor original.

Biblioteca rediseñada con barra lateral, portadas verticales y selector nativo
de carátulas por región. Descargas asíncronas con caché local y selección
persistente. Fuentes, disponibilidad y límites en [COVERS.md](docs/COVERS.md).

Compilación normal sin suites de pruebas. Los resultados históricos anteriores
no deben reinterpretarse como pruebas realizadas de nuevo ni como compatibilidad
garantizada. Integración Qt/motor y mando físico continúan pendientes.
