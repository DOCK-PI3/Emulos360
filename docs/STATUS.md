# Estado de Emulos360 — 2026-09-22

Este documento conserva el estado de la entrega de esa fecha. Las versiones
posteriores y la preparación para Linux se documentan en [BUILD.md](BUILD.md),
[LINUX.md](LINUX.md) y [FORK_CHANGES.md](FORK_CHANGES.md).

Paquete: `out/Emulos360/Emulos360.exe`. Motor incluido: Netplay
`6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c` más el parche propio de
`patches/xenia-netplay/0001-emulos360.patch`.

| Petición | Estado real | Qué queda |
| --- | --- | --- |
| Nombre Emulos360 e icono opción 2 | Aplicados en interfaz y núcleo | Ningún bloqueo conocido |
| Interfaz moderna sin web | C++20/Qt 6.8.3 Quick/QML | Pulir diseño según uso real |
| Biblioteca GOD/XBLA | 16 GOD y 14 XBLA detectados (30 en total); originales en solo lectura | Detección no demuestra que cada título sea jugable |
| Carátulas por región | Descarga, variantes, selección y caché implementados | Disponibilidad según proveedor |
| Ajustes del motor | 282 opciones coinciden con el binario, más 22 de consola | Traducción completa de descripciones técnicas |
| Perfiles | Persistencia corregida tras el proceso auxiliar; activación Netplay desde Qt con copia previa | Varios perfiles requieren selección explícita |
| Recuperar perfil y partidas | Perfil original importado y seleccionado; seis partidas en interno y siete en externo; Diablo III conserva ambas versiones | Probar la carga dentro de cada juego; versión, actualización y DLC deben coincidir. Ver PROFILE_RECOVERY.md |
| Avatares | Editor 3D nativo con modelos originales, animación de 71 articulaciones, rasgos, ropa, accesorios y guardado real por perfil | Renderizado dentro de juegos/XAM y editor XEX sin completar; proporciones, posicionamiento facial y expresiones animadas pendientes; ver AVATAR_SYSTEM.md |
| Guardado como consola | Selector interno/externo, rutas independientes y desconexión comprobados mediante ContentManager real | Comprobar guardar/continuar dentro de una campaña; algunos títulos recuerdan la unidad |
| Copias de partidas | Copia/restauración por unidad, hashes y cabeceras; conserva versión anterior | No son estados instantáneos; un corte durante renombrados puede requerir recuperación manual |
| Netplay | Modo de red y perfiles integrados; conversión y persistencia verificadas | Partida entre dos equipos y NAT sin acreditar |
| Crear/buscar/unirse a partidas | Consulta HTTPS real desde Qt: cuatro salas en la comprobación; sin bloqueo por ajustes pendientes | Creación/unión desde los menús del juego; compatibilidad concreta y partida real entre equipos |
| XBGuard HDD/USB | Inspección e importación completa con manifiesto y orden de plugins | Ejecución/autenticación: falta compatibilidad DashLaunch; Netplay no elimina ese bloqueo |
| Modo consola tipo Xbox 360 | Dashboard de mosaicos, guía, secciones y carátulas; ventana del motor alojada en Qt en Windows | Apartados interiores reutilizan las pantallas actuales; proceso del motor separado por diseño |
| Navegación con mando | Puente XInput Windows: cruceta/stick, A/B, LB/RB, Start/Back | Comprobación con mando físico; otros sistemas de entrada |
| Saga Gears | Los cuatro títulos están detectados; Gears of War llegó a «Press Start» y las dos partidas recuperadas de Gears 3 están instaladas en interno | Campañas completas, carga de esos guardados y rendimiento sin acreditar |
| Multiplataforma | Windows x64 compilado | Linux y distribución por plataforma |
| Documentación y separación del fork | Código Qt separado, parche contra Netplay e instrucciones actualizadas | Sin publicación de repositorio remoto ni release |
| Sistema operativo dedicado | Aplazado por decisión de empezar con el emulador | Fuera de esta entrega |

## Comprobaciones propias

Entrega 0.17: el inventario real de `D:\XBOX360_GAMES` devuelve 16 GOD y
14 XBLA, 30 títulos en total. El lector XBLA/STFS y los casos GOD focalizados
superaron 9 resultados QtTest. El comando de importación instaló primero un
perfil de prueba y un guardado de Gears 3 en un entorno desechable; después se
usó para la build activa. Los trece guardados quedaron repartidos entre interno
y externo, el perfil aparece en el núcleo, está habilitado para Netplay y su XUID
es el jugador 1. No se inició ningún título para estas comprobaciones.

Entrega 0.16: decodificación de los catálogos originales, modelos y animaciones;
renderizado real en Qt/D3D11 y recorrido de edición/guardado/reapertura del
manifiesto. Recursos privados importados en la build local. La animación del
editor Qt no acredita la implementación de XamAvatarLoadAnimation.

Entrega 0.15: comprobadas creación/listado/selección y persistencia del perfil,
activación de identidad Netplay con copia previa e identidad local conservada,
consulta pública desde Qt y arranque concreto de Gears of War. Detalles en
`VALIDATION.md`. No son pruebas generales de compatibilidad del motor.

Entrega anterior 0.14: ventana nativa dentro de Qt comprobada con el motor real vacío
(padre/WS_CHILD, tamaño, visibilidad y cierre normal). Navegador de salas comprobado
con respuestas controladas; servidor público HTTP 200 sin salas en esa consulta.
Ver `QT_NETPLAY_INTEGRATION.md` y `AVATAR_SYSTEM.md` para alcance y límites.

- Compilación Release del núcleo y Qt; motor actualizado incluido en el paquete.
- `check-storage-bridge.py`: crear, enumerar y reabrir contenido real en ambas
  unidades; desconexión sin redirigir las partidas.
- `check-profile-bridge.py`: crear Account nativo y releerlo entre procesos.
- `storageDevicesAndMetadata` y `saveBackupRestoreAndCorruption`: aislamiento,
  persistencia de modo de red, metadatos y rechazo de copias alteradas. Dos casos
  superados, cuatro resultados contando inicio/cierre de QtTest.
- `check-netplay-catalog.py`: 282 claves exportadas y 282 en la UI, sin diferencias.
- Capturas revisadas de dashboard, Partidas y Comunidad; registros QML vacíos.
- Parche verificado con `git apply --reverse --check`.

Se comprobó únicamente el arranque comunicado como averiado, sin repetir suites
generales de upstream. No se afirma una sesión online exitosa sin observarla.

## Uso inmediato

1. Abrir el paquete. El perfil importado queda seleccionado como jugador 1. F11 alterna
   escritorio y consola.
2. En Partidas, **Configurar externo…** elige una carpeta de USB/HDD real. Sin esa
   elección, el externo es una carpeta virtual separada.
3. En Comunidad, guardar el modo Netplay. El perfil activo local se habilita con
   copia previa; también existe **Habilitar Netplay** en Perfiles.
4. Consultar las salas, abrir el juego y usar sus menús multijugador o de guardado.
5. En Perfiles y avatares, **Avatar 3D** abre el editor original y **Guardar en el
   perfil** persiste sus cambios. `out/Emulos360/Ver-avatar-3D.cmd` abre una vista
   previa sin perfil ni guardado. La compatibilidad dentro de juegos sigue pendiente.

La recuperación e instalación solicitadas se documentan en
`PROFILE_RECOVERY.md`; las copias maestras permanecen en `local/recuperacion-*`.
