# Prioridades del core principal

Decisión y solicitud de retirada: **5 de octubre de 2026**.
Proyecto activo: `C:/Users/mabe8/Documents/GitHub/Emulos360`.

Retirada de Nexia360/Xenia Edge **terminada y verificada** en el proyecto.
[Funcionamiento, build local y conservación de datos](SINGLE_CORE.md).

1. **Mejorar Xenia Canary**, nuestro único core: compatibilidad de juegos,
   estabilidad, rendimiento y correcciones de texturas/personajes, preservando
   títulos y Netplay funcionales. No añadir otros cores ni retomar XBLIG.
2. **Trabajo de mejora iniciado el 6 de octubre, sin esperar a los lunes:**
   estudiar contratos y preparar correcciones pequeñas con pruebas. La primera
   mejora CPU y sus límites están en [el capítulo 13](research/xenia-canary/13-MEJORAS-CPU-Y-VALIDACION.md).
   El seguimiento de los lunes complementa este trabajo: solo Canary oficial y el fork Netplay;
   comparar revisiones antes de repetir trabajo. Preparar candidatos pequeños
   con causa comprobable. No integrar al estable sin validar regresiones.
3. **Publicación 0.5.0 autorizada para preparar builds:** resolver la transición
   del helper 0.4.0 antes de publicar un paquete sin Edge. El usuario hace commit,
   push, publicación y comprobaciones de juegos; no publicar por cuenta propia.

Los datos históricos y enlaces se conservan en su ubicación original; no borrar
`data/cores` ni desinstalar los frameworks XNA. La reparación previa del
actualizador Linux permanece. Commit, push, publicación y pruebas de juegos
continúan a cargo del usuario. No crear otra release por cuenta propia.

Consultar primero [el dossier](research/xenia-canary/README.md), el capítulo
correspondiente y [el seguimiento](research/xenia-canary/10-SEGUIMIENTO-Y-MEJORAS-DEL-CORE.md).
Reproducir un defecto concreto, registrar versión/TU, GPU, configuración y logs,
preparar un cambio acotado, medir y probar regresiones. Los candidatos
upstream preparados siguen sin estar integrados. Rivals sigue aplazado.
Los documentos Nexia/Edge/XBLIG se conservan únicamente como historial.

## Preparación 0.6.0 autorizada — 12 de octubre de 2026

El usuario confirma main y autoriza incorporar las mejoras acotadas viables y
crear builds Windows/Linux 0.6.0. La serie 0001 + 0002 conserva la base Netplay
fijada y las extensiones propias. [Selección y límites](releases/0.6.0.md).
Commit, push, publicación y controles finales de juegos siguen a cargo del
propietario. No se promete ausencia absoluta de regresiones ni FPS nuevos.
