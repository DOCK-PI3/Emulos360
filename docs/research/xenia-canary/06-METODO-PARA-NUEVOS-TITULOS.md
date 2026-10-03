# 06 — Procedimiento para habilitar y verificar un título nuevo

Este capítulo describe trabajo futuro. No se implementaron exports ni protocolos, no se lanzaron juegos y no se contactó con jugadores durante la investigación.

## 1. Crear una ficha que permita repetir el problema

Usar esta plantilla antes de modificar el motor:

```text
Título / Title ID / edición / región:
Media ID / hash del módulo XEX / TU / DLC:
Commit core + cambios propios / commit UI / backend:
Modo y operación exacta (buscar, crear, unir, iniciar, migrar...):
Estado offline y primera escena reproducible:
Host y cliente: OS, CPU, GPU/driver, config efectiva:
Perfiles distintos / modo de red / interfaz:
Topología: LAN, VPN o WAN; IPs/puertos observados:
Resultado esperado / resultado real / primer código de error:
Logs de ambos lados y petición backend correlacionada:
Parches aplicados y hash correspondiente:
Evidencia obtenida / hipótesis pendientes:
```

Seudonimizar identidad/IP si se comparte la ficha y evitar credenciales en logs. No es necesario publicar saves o perfiles completos para describir una llamada y su estructura.

## 2. Determinar qué tipo de online tiene ese modo

| Evidencia del título | Rama de trabajo |
| --- | --- |
| System Link y tráfico entre hosts | Descubrimiento LAN, XNet y sockets; comenzar sin servicios externos si el título lo permite |
| Sesiones XGI/XSession con datos del backend | Perfiles, propiedades, queries, QoS y dirección de peers |
| Tráfico a un servidor del editor o una API propia | Servicio específico: endpoints, mensajes, estado y autenticación necesaria |
| Servidor dedicado de gameplay | Investigar el protocolo y la simulación que realiza ese servidor |
| Modalidad híbrida | Separar login/metadata de lobby y tráfico de la partida |
| No avanza offline o falla el renderer | Resolver y documentar ese bloqueo antes de atribuirlo al multijugador |

No inferir arquitectura únicamente de género o engine. Confirmar imports y secuencia runtime, distinguiendo un endpoint configurado de una dependencia realmente utilizada.

## 3. Preparar un baseline controlado

Para futuras pruebas, usar dos PCs con direcciones diferentes y perfiles separados. La [guía de pruebas Netplay](https://github.com/AdrianCassar/xenia-canary/wiki/Testing-Procedure) recomienda entornos distintos para evitar colisiones de puertos y confusión de sesiones. Dos procesos en el mismo PC no son sustituto de peers independientes.

Fijar binario, TU, contenido, backend y ajustes. Empezar en LAN antes de incorporar NAT/WAN. Para comparar con upstream, mantener una build upstream exacta y una derivada con el mismo juego; la comparación debe aislar los cambios propios. Una VM requiere soporte GPU suficiente y dirección propia; no usar sus resultados de rendimiento como equivalentes a bare metal.

No cambiar router/VPN/firewall durante el diagnóstico sin registrar la modificación. Si el modo necesita servidor local, emplear un servicio aislado con su estado propio, no alterar o vaciar datos activos del usuario. Estas acciones no forman parte de la investigación actual.

## 4. Dibujar la máquina de estados observada

Escribir el recorrido de ambos lados: sign-in → flags/contextos → create/search → QoS → join → handshake → carga → juego → leave/end. Añadir timestamps, evento/overlapped, session ID y resultado de cada operación. La secuencia se deriva del título, no de una plantilla asumida universal.

Localizar el **primer desacuerdo**. Un error de timeout al final puede haber sido causado por una propiedad mal ordenada varios segundos antes. Correlacionar:

- Llamada guest: import, ordinal o XGI message, argumentos y retorno.
- Estado core: perfil, objeto sesión/socket, buffers y tarea.
- Petición API: ruta, código HTTP y filtros relevantes.
- Paquete host: origen/destino, protocolo, longitud y momento.
- Observación del peer: recepción, cambio de estado o rechazo.

Un ping correcto solo prueba conectividad de ese ping. Una respuesta HTTP solo prueba el servicio. Una búsqueda con resultados solo prueba una etapa del protocolo.

## 5. Investigar la API o ABI que falla

Fuente local primaria: `kernel/xam/xam_net.cc`, `kernel/xam/apps/xlivebase_app.cc`, `kernel/xsession.*`, `kernel/xsocket.*` y `kernel/xnet.h`. Si el export existe, inspeccionar su implementación; etiquetas `kImplemented`, `kSketchy`, TODOs y retorno exitoso no determinan fidelidad de contrato.

Para una función nueva o estructura privada:

| Campo | Evidencia requerida para implementar |
| --- | --- |
| Módulo, ordinal y versión | Tabla/import del binario objetivo y fuente correspondiente a esa versión |
| Prototipo y argumentos | Call sites y usos de retorno, headers públicos aplicables u otra evidencia independiente |
| PowerPC ABI | Registros de argumentos, stack, retornos, valores de 64 bits y objetos apuntados |
| Estructura guest | Tamaño, offsets, packing, endian y rango válido de cada campo |
| Resultado | Distinción entre X_RESULT/HRESULT/NTSTATUS/errores Winsock y estado pending |
| Asincronía | Buffer, evento, callback/APC, contexto y orden de publicación/completado |
| Lifetime | Propietario hasta completion, cancelación, descarga del título y shutdown |

Según la guía `xbox360-xdk-system`, código comunitario y reverse engineering se etiquetan como evidencia privada; una librería de importación no basta para validar comportamiento. No usar una firma supuesta porque su nombre sea similar a Win32.

### Anclas de layouts comprobadas en la base

Estas son assertions de la **implementación local**, no layouts oficiales universales. Verificar revisión y offsets antes de reutilizarlos.

| Estructura | Tamaño en bytes | Archivo |
| --- | --- | --- |
| `XNKID` | `0x08` | `kernel/xnet.h` |
| `XNKEY` | `0x10` | `kernel/xnet.h` |
| `XNADDR` | `0x24` | `kernel/xnet.h` |
| `XSESSION_INFO` | `0x3C` | `kernel/xnet.h` |
| `XSESSION_SEARCHRESULT` | `0x5C` | `kernel/xnet.h` |
| `XGI_SESSION_SEARCH` | `0x20` | `kernel/xsession.h` |
| `XGI_SESSION_CREATE` | `0x1C` | `kernel/xsession.h` |
| `XWSABUF` | `0x08` | `kernel/xsocket.h` |
| `XWSAOVERLAPPED` | `0x14` | `kernel/xsocket.h` |

Fuente: [xnet.h](https://github.com/AdrianCassar/xenia-canary/blob/6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c/src/xenia/kernel/xnet.h), [xsession.h](https://github.com/AdrianCassar/xenia-canary/blob/6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c/src/xenia/kernel/xsession.h), y el archivo `xsocket.h` local inventariado, que contiene cambios propios además de la base.

## 6. Resolver por contrato y subsistema

### Si no aparecen sesiones

Revisar estado de perfil, modo y flags, anuncio de sesión, Title/Media ID y versión, contextos game mode/type, slots, query ID, propiedades y filtros. Para System Link, verificar broadcast en la interfaz correcta. Para Netplay, comprobar respuesta de API y serialización guest. No borrar los filtros para que aparezca cualquier sala.

### Si aparece el lobby y no se entra

Revisar session ID/classificación XNKID, MAC, IP local/online y puerto, flags de plataforma, prejoin, QoS payload, clave registrada y errores de bind/connect. Ver si paquetes de negociación llegan y si se completa el overlapped. Comparar el primer paquete o estado rechazado con una sesión que funciona.

### Si se entra y no empieza la partida

Revisar transición start, slots/lista de miembros, disponibilidad del mapa/DLC, consistencia TU y handshake de carga. Recoger datos CPU/GPU si un peer deja de enviar porque se bloqueó. No equiparar timeout de carga con NAT.

### Si se juega y luego se desincroniza o desconecta

Investigar pérdida/reordenación, MTU/fragmentación, colas, timeouts específicos, estados de retransmisión y ritmo de simulación. Identificar si el juego admite diferentes frame rates antes de activar desbloqueos. Capturar stacks y vida de sockets/tareas cuando un peer abandona.

### Si depende de un servicio específico

Hacer un catálogo de endpoints/mensajes, datos y transiciones: DNS → transporte → login → metadata → matchmaking/game server. Localizar en el juego o fuentes públicas la estructura de cada request/response, y comprobar su interpretación. Una respuesta ficticia sin campos correctos puede avanzar al menú y bloquear después.

Una implementación mínima verificable cubre el modo elegido y documenta los demás como pendientes. Si el servidor original también simula gameplay, habrá que implementar esa responsabilidad, no solo respuestas HTTP. No se asume compatibilidad con autenticación/infraestructura oficial por implementar una API comunitaria.

## 7. Parches por juego cuando sean necesarios

Usarlos solo cuando la causa específica y su compromiso estén identificados. Vincularlos a hash/XEX/TU y comprobar bytes originales; investigar el contrato general antes de duplicar writes para muchos títulos. Registrar si desactivan un efecto, alteran FPS, saltan una ruta de discovery o corrigen un dato.

Por ejemplo, la wiki referencia parches System Link para algunos Call of Duty. Eso prueba que hay soluciones por versión, no que pueda copiarse un branch NOP a otro juego. La extracción/revisión de esos parches concretos sería un siguiente análisis separado; no se han auditado aquí sus direcciones.

## 8. Demostrar compatibilidad por etapas

| Nivel | Evidencia mínima |
| --- | --- |
| E0 | Código/hipótesis, sin ejecución |
| E1 | APIs y contratos probados de forma aislada |
| E2 | Lobby descubierto con dos peers |
| E3 | Unión y comienzo de partida |
| E4 | Sesión de duración definida, reconexión/salida y modos seleccionados |
| E5 | Repetición en otra topología/build y regresión de otros títulos afectados |

La documentación actual de Emulos360 no se eleva a E4 por el listado upstream. Registrar nivel por versión/mode/backend; no asignar una marca general al título si solo se probó un lobby LAN.

Para completar la ficha: cambios mínimos y causa, evidencia de contrato, logs/paquetes, prueba host/client intercambiados, duración, salida ordenada, caso negativo e impacto en rutas comunes. Esa información permitirá a otra sesión de trabajo continuar sin reconstruir suposiciones.
