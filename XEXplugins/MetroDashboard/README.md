# Dashboard Metro para Emulos360

`dist/default.xex` es un título Xbox 360 PowerPC original, construido con el
XDK. Xenia lo ejecuta como la pantalla de consola en el PC; no reutiliza las
pantallas Qt de la biblioteca ni de ajustes. La disposición de mosaicos, barra
superior y fondo gris toma como referencia la captura Metro facilitada por el
usuario. La tienda y sus anuncios se sustituyen por Netplay.

Desde Emulos360, **Modo consola** o **F11** abre el título. La biblioteca y las
salas se dibujan dentro del `.xex`. A abre un mosaico o ejecuta el juego,
Y abre el selector de carátulas del juego seleccionado y vuelve al dashboard
al cerrarlo; B vuelve, la cruceta o
el stick navegan, los gatillos pasan las páginas de juegos o salas y los
botones laterales cambian de sección. Al cerrar un juego, Emulos360 vuelve a
arrancar el dashboard. Mantener Guía 2,5 segundos abre el menú de sistema;
**Apagar Windows** exige una segunda confirmación y solo entonces pide al PC
que se apague. El menú propio del juego sigue funcionando mientras se juega.

El puente PC/guest copia el `.xex` y `font.png` a `data/metro-session`. Xenia
monta *solo esa copia temporal* con escritura para intercambiar
`snapshot.txt`, `request.txt`, `ack.txt` y `guide.txt`. Los juegos de la
biblioteca mantienen el montaje normal de solo lectura. El anfitrión valida
identificador de sesión, número de petición e índice antes de actuar.

Compilar:

```powershell
./XEXplugins/MetroDashboard/build.ps1
./scripts/build-ui.ps1 -Package
```

El XDK local se indica con `XEDK`. `build.ps1` compila con `/W4 /WX` y deja
un informe de `imagexex` en `build/xex-audit.txt`. El dashboard retail real de
la consola está en la flash `Sfc:\dash.xex`; la partición FATX `G:` aportada
incluye Aurora y datos de usuario, pero no ese módulo ni sus recursos visuales.
