# Arquitectura — actualización 2026-09-22

La base actual es `AdrianCassar/xenia-canary@6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c`,
rama local `codex/netplay-integration`. Qt y núcleo se comunican mediante proceso
propio y el puente de perfiles. Los cambios actuales se preservan en
`patches/xenia-netplay/0001-emulos360.patch`. El dashboard es Qt nativo; la red y
los menús de amigos del núcleo reutilizan Netplay. Véase [STATUS.md](STATUS.md).

Las secciones siguientes conservan la evaluación inicial y sus hitos históricos.

## Decisión técnica

Usar Xenia Canary como base inicial de evaluación del fork. Revisión fijada:
`6f9840568bb7dc9f5a9adb94801a99b18a7b2a60`, rama local `los/main`.

El repositorio superior contiene la aplicación Qt y la documentación; `engine/xenia`
es un submódulo con el historial y las dependencias de la base. No se ha publicado
ningún fork remoto. Los cambios funcionales propios del motor se identifican en
`docs/FORK_CHANGES.md` y se preservan en `patches/xenia/`.

## Comparación de bases

| Base | Evidencia consultada | Decisión |
| --- | --- | --- |
| Xenia principal | Código y documentación oficial; proyecto original | Referencia para comparar comportamiento y cambios |
| Xenia Canary | Código, ruta SVOD/GOD, documentación de compilación y reportes específicos de Gears | Base inicial por su desarrollo experimental y ecosistema de parches; requiere pruebas locales |

No se infiere superioridad universal de los reportes de compatibilidad. No se
activan parches de juegos ni opciones de velocidad sin identificar primero la
versión del ejecutable invitado y medir el comportamiento base.

Fuentes: [Xenia](https://github.com/xenia-project/xenia),
[Canary](https://github.com/xenia-canary/xenia-canary),
[reporte de Judgment](https://github.com/xenia-project/game-compatibility/issues/585),
[parches de Gears](https://github.com/xenia-canary/game-patches).

## Límites entre componentes

- `app/library.*`: descubrimiento GOD y XBLA en solo lectura. Lee una cabecera
  limitada, valida `00007000`/SVOD para GOD y `000D0000`/STFS para XBLA, obtiene
  Title ID y nombre, y comprueba los fragmentos cuando el formato los requiere.
  No comprueba hashes ni garantiza que un juego arranque.
- `app/controller.*`: estado de aplicación, escaneo en un worker, publicación del
  resultado en el hilo Qt y preferencias INI.
- `app/qml`: presentación nativa Qt Quick. Los modos escritorio y consola
  comparten el mismo controlador. Sin WebView ni motores HTML.
- `engine/xenia`: CPU, GPU, kernel invitado, audio y VFS de la base del fork.
  Se compila por separado para establecer una referencia antes de integrar Qt.

La capa `PlayerServices` inicia el núcleo Emulos360 del paquete en un proceso
propio, le transmite configuración y rutas, gestiona su ciclo de vida y bloquea
los cambios concurrentes. Su ventana de juego sigue separada de Qt. El puente
nativo de perfiles llama al gestor de Xenia mediante comandos transitorios;
la capa Qt no duplica el cifrado del archivo Account. `EngineSettings` gestiona
el TOML y XConfig; `save_store` gestiona copias fuera de la biblioteca de juegos.
El comando transitorio `import-content` inspecciona e instala perfiles o partidas
mediante las funciones del núcleo; rechaza enlaces, tipos ajenos y sobrescrituras.

## Stack de esta fase

C++20, Qt 6.8.3 Quick/QML y CMake para la interfaz. Rust queda aplazado hasta que
exista un componente que justifique una frontera FFI. No reemplazar SDL2 de la
base con SDL3 solo por preferencia: verificar primero las dependencias heredadas.
El backend de Qt es independiente del backend de emulación.

## Próximos hitos

Prioridad del usuario, 2026-09-17: marca Emulos360, diseño y carátulas regionales.
No repetir pruebas del motor upstream sin cambios. La biblioteca cuenta ya con
selector de carátulas y caché local; los hitos de emulación siguientes están
aplazados mientras se trabaja en presentación.

1. Compilación de referencia completada y herramientas documentadas.
2. Carga inicial de Gears 1 observada en el log; pendiente validar imagen, audio y una escena controlada.
3. Adaptador de sesión Qt/motor implementado mediante proceso propio; integración de superficie pendiente.
4. Integrar mando y estados de sesión; el selector de carátulas ya está implementado.
5. Medir campañas de los cuatro títulos; Gears 3 ya está presente, pero su
   campaña completa y rendimiento aún no están acreditados.
6. Repetir compilación y pruebas en Linux.

## Procedencia

Conservar `engine/xenia/LICENSE` y los avisos de sus dependencias. La licencia de
los archivos propios sigue pendiente; el nombre definitivo es Emulos360. El paquete local
de prueba no es una distribución pública. Qt se utiliza mediante bibliotecas
dinámicas. Antes de distribuir, preparar los avisos y materiales que correspondan
a las dependencias elegidas.
