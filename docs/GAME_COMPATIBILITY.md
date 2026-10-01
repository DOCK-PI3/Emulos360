# Iconos de compatibilidad

La tienda muestra un icono y el estado junto a cada título. La biblioteca lo
muestra al pie de la carátula, junto al tamaño y el formato. Al pasar el ratón
se ve la explicación, los fallos reportados y la fecha del informe disponible.
La leyenda está en **Ajustes → Emulos360 → Compatibilidad**.

| Icono | Estado | Qué indica |
| --- | --- | --- |
| ✓ verde | Jugable | Jugado de principio a fin con pocos fallos o ninguno. |
| ▶ azul | En juego | Permite jugar; terminarlo no está confirmado y puede tener fallos. |
| ⏻ amarillo | Solo arranca | No llega a una partida: introducción, menús o carga. |
| ✕ rojo | No jugable | Bloqueos o fallos graves impiden jugar. |
| ? gris | Sin datos | Informe sin estado conocido, juego ausente o edición ambigua. |

Fuente: [catálogo de Xenia Manager](https://xenia-manager.github.io/compatibility),
que recopila [informes comunitarios de Xenia Canary](https://github.com/xenia-canary/game-compatibility/issues).
Snapshot incluido consultado el 29 de septiembre de 2026, obtenido de
`https://xenia-manager.github.io/database/data/game-compatibility/canary.json`.
Los iconos y el código de integración son propios de Emulos360.

La biblioteca identifica los juegos por Title ID. La tienda, cuyo catálogo no
proporciona Title ID, compara nombres normalizados; no mezcla demos, betas o
ediciones ambiguas. Los informes duplicados de la misma edición usan la fecha
más reciente. Una actualización fallida conserva los estados disponibles.

El catálogo incluido funciona sin conexión. Se comprueba una actualización al
iniciar si la copia local tiene más de un día; también se puede solicitar desde
la leyenda. Se conserva en `data/compatibility/canary.json` y se reemplaza solo
después de validar la descarga. No se hacen consultas por cada juego.

Estos informes describen Xenia Canary, no una prueba en tu equipo con Emulos360.
El resultado puede variar por GPU, sistema operativo, versión del motor, parches
y ajustes. «En juego» no equivale a una campaña completa comprobada.

## Pruebas locales de septiembre de 2026

- Los cuatro registros pedidos por el usuario se conservaron en
  `.tools/game-logs/2026-09-29/`: Crysis 3, Borderlands 2 y Black Ops III
  están en `retest/`; Crash Course 2 es `04-crash-course-2-core.log`.
  Los cuatro registran `network_mode = 2` al arrancar.
- **Doritos Crash Course 2 (5841124F):** la prueba con modo 2 ya se hizo.
  El registro muestra inicialización del avatar, cero elementos en
  `XTitleServerCreateEnumerator` y solicitudes `XamGetServiceEndpoint` y
  `XamGetToken`. No hay evidencia de partida jugable. No volver a proponer
  el modo Netplay como si fuera una prueba pendiente.
- El intento del 30 de septiembre en `out/Emulos360/data/core.log` registra
  `network_mode = 0` y acaba en el aviso de Xbox LIVE de la captura del usuario.
  Esto no invalida la prueba anterior con modo 2.
- El usuario confirmó el 30 de septiembre que Borderlands 2 y The Pre-Sequel
  avanzan y permiten jugar al elegir System Link / VPN; The Pre-Sequel conserva
  algunos fallos gráficos. La conclusión anterior de que ambos estaban
  bloqueados al cargar quedó superada por estas pruebas.
