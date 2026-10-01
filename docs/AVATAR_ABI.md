# Evidencia del manifiesto original y límites de la integración

Actualización 2026-09-21. Código propio: `emulos_avatar_{manifest,assets,scene,animation}.h`,
`AvatarStudio` y cambios acotados en el puente de perfiles y `xam_avatar.cc/.h`.
No se copian fuentes, librerías ni recursos del SDK al repositorio. El recurso
privado del esqueleto se importa por separado para esta build local.

## Evidencia local

El SDK instalado es Xbox 360 XDK 21256.3. El editor extraído del backup del usuario
importa el sistema 2.0.17559.0. Son versiones distintas: la inspección del SDK
no acredita por sí sola la ejecución completa de ese editor.

| Contrato | Evidencia | Uso implementado |
| --- | --- | --- |
| Metadatos opacos de 1000 bytes | `include/xbox/xavatartypes.h`, contrato público XAVATAR_METADATA | Estructura opaca con tamaño comprobado; se retiran campos supuestos |
| Cuerpo en recurso de 16 bytes a offset 0x120 | Constructor de ManifestReader y GetBodyType, `avatarmanifest.obj` de xavatar2.lib; inspección de código PowerPC y relocaciones | Lectura acotada, sin convertir GUID con el endian del host |
| Identidad masculina | `AVATARASSETPACK_ASSET_MALEBODY`: `0000000200000001c1c8f109a19cb2e0` | Devuelve 1 |
| Identidad femenina | `AVATARASSETPACK_ASSET_FEMALEBODY`: `0000000200010002c1c8f109a19cb2e0` | Devuelve 2; desconocido/nulo devuelve 0 |
| Sin avatar en el perfil | `XAVATAR_E_USER_HAS_NO_AVATAR`, xavatar.h público: 0x80990001 | La lectura rechaza ajustes vacíos/truncados, limpia el buffer y comunica el error |
| Escritura del manifiesto | Ruta existente XamAvatarSetManifest → UserTracker/GPD | Valida índice/puntero e inicializa por completo el descriptor antes de guardar |

Los offsets del lector son evidencia de implementación privada, no una promesa
del contrato público. El lector de cuerpo conserva la representación original y
no asigna un cuerpo inventado a recursos desconocidos. El ensamblador 0.16 sí
interpreta los campos documentados a continuación; conserva el resto al editar.

Hashes SHA-256 de objetos privados inspeccionados, guardados solo en `/local/`:

- `init.obj`: C7C4D6012BB1B2A49C9931DD219DD56B8AE1CCC314237687D8B2C8F63A73E604
- `avatarmanifest.obj`: E2A6C93280B7630AFCDB02407042E5C2DF667E5CFEBEDC39B8F23921AFEE5571
- `texture.obj`: 4E6F7A3532C5B2F398634D1B88FDD978529EA91B74437FDEF8F05493890CFF1D

## Lo que todavía impide el sistema 3D completo

La inspección de XAvatarInitialize confirma que los cinco callbacks entregados a
XamAvatarInitialize corresponden a texturas: tile, untile, dimensiones, tamaño
y offset de mip. **No constituyen un cargador completo de modelos** que pueda
ejecutarse simplemente desde XAM. El primer argumento privado es una versión del
contrato; no es el sistema de coordenadas, como sugiere el nombre actual del stub.
No se ha cambiado su firma ni activado globalmente ese stub.

TOC/STRB, LZX, mallas y poses ya se decodifican y dibujan en el editor Qt.
La build de prueba 0.18 implementa los buffers CPU/GPU para ABI4, coordenadas
izquierdas; los objetos privados de animación siguen pendientes. El renderizado
Qt y la entrega de buffers no acreditan por sí solos el resultado en una partida.

## Entrega experimental a Doritos (2026-09-23)

### Corrección de posición homogénea posterior a 0.18

Las capturas de la carrera aportadas por el usuario confirman que 0.18 mantiene
J1 invisible. El contenedor privado del esqueleto guarda XYZ; el serializador
invitado dejaba W=0 en `BindPose.Position` y `BindPose.Local.Position`.
`XAVATAR_SKELETON_BINDPOSE_JOINT` usa XMVECTOR, no XMFLOAT3. El consumidor del SDK
(`Source/Samples/Common/AtgAvatarRenderer.cpp`, `RebuildJoints`) asigna ese vector
completo a la fila de traslación y calcula la inversa de la matriz de reposo.
Con W=0 la matriz es singular. La corrección emite W=1 para ambas posiciones.
La prueba nueva falla con el serializador 0.18 en la articulación 0 y pasa
con ambos cuerpos tras el cambio, también con AddressSanitizer. El resultado
visual se confirmó el 2026-09-23 a las 19:54: J1 aparece en USA 1: Safety
First de Doritos con un perfil de prueba, ropa y sombra; se observa
la animación de entrada. El motor probado y empaquetado tienen SHA-256
`4f01a0021d98c0288e65145568c7727a2bfcdbbf03d2165260da7bbbfdf3ed12`.

### Alcance de la carga

- Título observado: 58410A71. Su llamada real a XamAvatarInitialize entrega
  versión 4, coordenadas 0, procesador 5. Se rechazan contratos distintos.
- Layout público contrastado con `xavatar.h` de XDK 21256.3: ASSETS 20 bytes,
  SKELETON 8, JOINT 96/alineación 16, COMPONENT_INFO 32, MODEL 52, BATCH 512,
  TEXTURE 44. Enteros, flotantes y direcciones son big endian de 32 bits.
- CPU y GPU pertenecen al título. Se comprueba acceso al rango, desbordamiento,
  solapamiento y capacidad (1 MiB / 8 MiB). El montaje se hace primero en memoria
  del host y sólo se copia después de validar el resultado completo.
- Texturas BC1/2/3 lineales, filas alineadas a 256 y capas a 4096; sólo nivel base.
  Las BC1 vacías usan el selector transparente. No se entregan carryables.
- El catálogo y el esqueleto son datos inmutables compartidos; cada operación
  diferida conserva su referencia. Initialize/Shutdown sustituyen el caché;
  también se comprueban propietario KernelState y título antes de usarlo.
- MetadataRandom: argumentos verificados en `avatarmetadata.obj`, relocación
  al ordinal 1515 y llamada real de Doritos (máscara 3, cantidad 5). Genera
  manifiestos válidos con cuerpo y ropa del catálogo compatible. No modifica
  perfiles. La enumeración de perfiles desconectados todavía no está implementada.
- La biblioteca activa la ruta al iniciar Doritos si existen los dos recursos
  originales. Otros títulos mantienen su configuración. El interruptor manual
  de diagnóstico es `emulos_avatar_guest_assets`.
- Primera ejecución aislada: inicialización y diez entregas de modelos completas.
  El aviso sobre cinco manifiestos aleatorios vacíos motivó la implementación de
  MetadataRandom. La verificación posterior se registra en las notas de la build.
- Pruebas con recursos originales y AddressSanitizer: geometría, texturas,
  límites, punteros/alineación y rechazo de buffers inválidos. Ningún recurso
  privado se añade al parche de código.

## Formatos implementados en 0.16

Evidencia: catálogos extraídos del paquete del HDD, contratos públicos XDK y
lectura de los serializadores/decodificadores de las herramientas instaladas.
Las notas, desensamblados y recursos de comparación permanecen en `local/`.

- TOC v2: cabecera 48 bytes, registros 268 bytes, 18 idiomas. TOC v1: 40,
  248 y 13 respectivamente. Tamaños y offsets se validan antes de leer.
  V2 contiene 1282 registros, de los cuales 1103 tienen recurso utilizable.
- STRB: cabecera 32 bytes, namespace del formato y registros alineados a 4.
  Modelos en bloque 3, texturas en 2, sustituciones geométricas en 4,
  animaciones en 1. No se confunde STRB con STFS.
- Cada fragmento LZX lleva tamaño comprimido, offset de destino y tamaño
  descomprimido, little endian; ventana 32768 e independencia entre fragmentos.
  La salida está limitada a 16 MiB y no permite solapamientos.
- Mallas: posiciones cuantizadas en retícula, enteros de anchura variable,
  UV half-float, índices de 16 bits, soldaduras y sustituciones por recurso GUID.
  Normal empaquetada 11/11/10, pesos/articulaciones en bytes de mayor a menor.
- Texturas: BC1/BC2/BC3, bytes de cada palabra de 16 bits intercambiados.
  Qt recibe RGBA expandido porque la carga BC directa por TextureData no funciona
  en el backend D3D11 observado. Los recursos originales conservan todos sus
  niveles/capas; la vista actual usa la primera capa.
- Manifiesto: versión en 0; peso/altura 4/8; rasgos geométricos 12/28/44;
  seis texturas con ubicación a partir de 60, stride 32; nueve colores ARGB
  en 252; componentes desde 288, stride 32. No se inventan GUID de componentes.
- Animación: cabecera de 10 DWORD little endian; flujo de poses por fotograma.
  Contextos de articulación en orden inverso, relleno hasta 72; cada pose tiene
  posición, logaritmo de cuaternión y escala. Las poses son deltas respecto a la
  pose local de reposo. La vista interpola rotaciones y deforma con cuatro pesos.
- `EMUSKEL1` es un contenedor **propio**, no un formato Xbox: contador y registros
  de 72 bytes con padre, transformada global de reposo y transformada local.
  El importador limita la jerarquía a 71 articulaciones conocidas.

La inspección de `AnimationData_c::Load` confirma que XamAvatarLoadAnimation
recibe un objeto privado previamente construido y un overlapped, no simplemente
un buffer donde copiar los fotogramas. En el SDK inspeccionado ese objeto incluye
contextos de descompresión y memoria proporcionada por el título. Sustituirlo por
las poses de Qt o devolver éxito sin inicializarlo no implementaría la ABI.

### 0.18.2: guest coordinate-system correction (2026-09-24)

Doritos requests ABI 4, coordinate system 0 (left-handed). The imported
EMUSKEL1 rig and decoded model streams are right-handed. Previously the guest
serializer copied both unchanged, reversing the anatomical forward direction
relative to the title animation. Conversion now happens only in guest staging:
Z is reflected in bind/local positions and mesh positions, quaternion X/Y are
negated, packed HEND3N normal Z is negated with SNORM endpoint saturation, and
triangle indices 1/2 are exchanged. Weights, joint indices, UVs and the editor's
source assets are unchanged. This is a whole-coordinate-system correction,
not a head-specific rotation.

Evidence: installed XDK 21256.3 xavatar.h defines LeftHanded=0; private managed
EmbeddedSkeleton.GetEmbeddedSkeleton exposes LeftHanded=0, RightHanded=1 and
Natal=2. All 71 reflected joints compare exactly to its left-handed Natal rig.
Private parser inspection: UnpackVertices calls FlipCoordinateSystem for LH;
Vector3dDataUnpacker.InvertCoordinateSystem negates MinZ and Delta2; LH triangle
unpacking exchanges the second and third indices and uses converted normals.
These private tool details are local reference evidence, not a new public ABI.
No SDK assets or code are added to the update package.

Validation: new regression fails on the previous serializer; fixed Release and
AddressSanitizer resource suites pass for male and female scenes. Checks cover
all emitted joint positions/rotations, every vertex position/normal and triangle
winding, alongside existing homogeneous-W, pointer and rejection tests.
In-game appearance is pending user validation; no visual test was performed
for 0.18.2, at the user's request.
