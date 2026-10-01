# Party de voz de Emulos360

La sección **Comunidad → Party de voz** permite crear una sala dentro de Emulos360. El anfitrión ejecuta la señalización en su propio PC; los demás jugadores se conectan a él por LAN o por IP pública y puerto. La voz se comprime con Opus (mono, 48 kHz, tramas de 20 ms), y voz y mensajes viajan por canales de datos WebRTC cifrados. La sala continúa activa al abrir un juego en el motor integrado. Es independiente de las salas Netplay de Xenia: no sincroniza el juego.

## En la red local

1. En el PC anfitrión, abre **Comunidad**, indica un nombre y pulsa **Crear sala**. El puerto inicial es `46580`.
2. En el otro PC, espera a que aparezca la sala en **Salas detectadas en tu red local** o escribe su IP local. La sala detectada rellena la IP y el puerto.
3. Escribe el código de ocho cifras que muestra el anfitrión y pulsa **Unirse**. La lista de participantes y los mensajes confirman la conexión. Cada participante puede silenciar su micrófono.

El descubrimiento local usa UDP `46579` y depende de que los PC estén en la misma red que permite broadcast. Si una red bloquea el descubrimiento, la IP local funciona manualmente. El cortafuegos de Windows puede solicitar permiso para Emulos360 al crear o unirse a una sala; hay que permitirlo en la red deseada.

## Por Internet

El anfitrión comparte su IP pública, el puerto TCP elegido y el código de sala. En su router debe redirigir el puerto **TCP elegido** (por defecto `46580`) y el rango **UDP `46581–46600`** a la IP local del PC anfitrión. También debe permitir esos puertos en el cortafuegos del PC. El invitado introduce la IP pública, el puerto y el código en **Comunidad → Party de voz**.

No se usa STUN, TURN ni un servidor externo de señalización. Esta forma de conexión directa requiere que la IP y los puertos del anfitrión sean accesibles. Con CGNAT, doble NAT sin redirección en ambos routers o redes restrictivas, la conexión por Internet puede fallar. Una VPN privada que conecte las dos LAN puede servir como alternativa, pero Emulos360 no la instala ni la necesita para funcionar en LAN.

El código evita uniones accidentales, pero la señalización TCP que intercambia el código y los metadatos de conexión no tiene cifrado propio. Los canales WebRTC de voz y chat sí usan DTLS. Comparte el código solo con personas conocidas y no expongas esta sala como servicio público.

## Compilación

`scripts/build-ui.ps1` prepara versiones fijadas de libdatachannel, Mbed TLS y Opus mediante `scripts/setup-voice.ps1`, las enlaza con la interfaz y `-Package` copia sus avisos de licencia al paquete. Las dependencias se descargan una vez en `.tools/`; para esa preparación se necesita Git y acceso a GitHub. La aplicación resultante no ejecuta programas de voz externos.

Para comprobar el transporte local sin dispositivos de audio, ejecuta `scripts/build-ui.ps1 -Test`; la prueba `voice_party_tests` crea dos instancias, conecta WebRTC y comprueba el chat en ambos sentidos. La calidad del micrófono/altavoces y la conexión real entre dos PC requieren una prueba manual.
