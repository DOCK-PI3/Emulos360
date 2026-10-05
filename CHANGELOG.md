# Cambios

## 0.5.0 — 5 de octubre de 2026 (preparada para publicación)

- Retirada de Nexia360 y Xenia Edge; Xenia Canary es el único motor.
- Se mantienen GPU global/por juego, perfiles, partidas, avatares y Netplay/MultiP.
- Paquetes e instaladores de actualización sin requisitos de cores externos.
- Se conserva la reparación del actualizador Linux y los datos históricos.
- Ubuntu: se instala y comprueba `libqt6concurrent6`, cuya ausencia impedía
  arrancar la nueva interfaz tras actualizar (código 127).


## 0.4.0 — 3 de octubre de 2026

- Segundo core opcional Canary Edge, con revisión oficial fijada y verificación
  SHA256, incluido en Windows y Linux.
- Selección de core y GPU global o por juego desde Ajustes y Biblioteca.
- Almacenamiento independiente de Edge; copia inicial de perfil y contenido sin
  sobrescribir las partidas actuales ni el progreso posterior de Edge.
- Se conserva el binario del core actual y la ruta Netplay/MultiP existente.
- Actualización desde 0.3.0 con conservación de datos. Edge se ejecuta en su ventana
  nativa; sus partidas y logros no se sincronizan con el core actual.

## 0.3.0 — 1 de octubre de 2026

- Actualizador desde GitHub Releases, comprobación SHA256, instalación con
  restauración ante errores y conservación de datos personales.

## Base anterior (0.2.0)

- Biblioteca en cuadrícula y carrusel, carátulas manuales e indicadores de
  compatibilidad.
- Instalación, listado y eliminación de contenido adicional y actualizaciones
  por juego.
- Perfiles, partidas, logros, avatares, Netplay y salas privadas MultiP.
- Preparación de fuentes y paquetes separados para Windows y Linux.

Las notas de cada versión publicada se añadirán también a su GitHub Release.
