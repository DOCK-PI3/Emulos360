# Avatares originales Xbox 360: editor nativo y estado del núcleo

La petición es usar los avatares originales, incluido su funcionamiento dentro
de los juegos. Una imagen 2D o un personaje propio dibujado en Qt no cumple esa
petición. **La petición completa todavía no está terminada:** desde la build 0.16
se incluyen modelos y animaciones originales en el editor Qt, pero su renderizado
dentro de los juegos sigue pendiente.

## Editor disponible desde la build 0.16

Perfiles y avatares → **Avatar 3D** → personalizar → **Guardar en el perfil**.
Permite elegir cuerpo, cabello, ropa, calzado, accesorios, mentón, nariz, orejas,
boca, ojos, cejas, barba, maquillaje y rasgos de piel, con nueve colores.
También permite previsualizar las animaciones compatibles con ese cuerpo y girar
el personaje. Se guardan las referencias originales y los colores en el GPD;
la animación elegida es una preferencia temporal de previsualización.

El renderizado es Qt Quick3D con geometría, texturas y animaciones decodificadas
del catálogo original. No es una imagen 2D ni un personaje de sustitución.
Los catálogos proceden del HDD aportado. El esqueleto de 71 articulaciones de esta
build privada se importa de las herramientas XDK 21256.3 instaladas en el equipo,
en `avatar-skeleton.bin`. No se atribuye ese archivo al HDD. El emulador no carga
DLL del SDK durante su ejecución. Sin el esqueleto se conserva la vista estática.

El manifiesto se respalda antes de escribirlo. La lectura posterior remonta el
perfil para evitar el tamaño de GPD antiguo almacenado en la caché del VFS.
Los datos del perfil seleccionado, las partidas y la identidad se conservan.

Pendientes del editor: altura/peso, desplazamiento/escala de rasgos faciales,
colores de ropa por paletas, carryables y expresiones faciales por capas de
animación. No se presenta la pantalla como réplica completa de AvatarEditor.xex.

## Recursos del usuario

Se inspeccionó en modo `rb`, sin montar ni escribir el origen:
`D:\XBOX360_GAMES\xbox360hdd - 55,89 GB Backup.bin` (60 011 642 880 bytes).

`scripts/inspect-xbox-hdd.py` reconoce las particiones FATX SystemAuxiliary,
SystemExtended y Compatibility. Valida geometría, rangos y ciclos antes de leer.
No recorre perfiles ni sectores de seguridad. La extracción opcional usa una
carpeta nueva dentro del workspace, rechaza sobrescrituras y registra SHA-256.
Offsets/formato: referencias comunitarias de
[Velocity FatxConstants](https://github.com/hetelek/Velocity/blob/master/XboxInternals/Fatx/FatxConstants.h)
y [FatxDrive](https://github.com/hetelek/Velocity/blob/master/XboxInternals/Fatx/FatxDrive.cpp).
Estas referencias no son contratos oficiales XDK.

Resultados privados, excluidos de Git por `/local/`:

- `local/hdd-system-inventory.json`: 151 archivos SystemAuxiliary, 21
  SystemExtended y 6 Compatibility.
- `local/hdd-avatar-resources.json`: extracción seleccionada y hashes.
- `local/avatar-system-from-hdd/SystemAuxiliary/20449700/AvatarEditor.xex`:
  XEX2 de aplicación, importa `xam.xex` y `xboxkrnl.exe`, versión 2.0.17559.0.
- Recursos XZP del editor, minicreador y manifiesto del sistema.
- Paquetes PIRS FFFE07DF: AvatarAsset, ParityPack, DashPack y NuiPack.
- Directorio STFS de AvatarAsset: `AvatarAssetPack.toc` (14 662 168 bytes) y
  `AvatarAssetPackLegacyV1.toc` (1 252 000 bytes).

Los archivos del usuario no se añaden al parche, al ejecutable ni a una
distribución pública. En la revisión de recuperación posterior se extrajeron
tres contenedores de perfil a `local/hdd-profile-identification/` para leer sus
cuentas con el núcleo. No se importaron a la build ni se copiaron partidas:
el gamertag solicitado no aparece. Inventario privado en esa misma carpeta.

## Estado del núcleo en la build de prueba 0.18

Actualización 0.15: lectura del tipo de cuerpo original y validación de lectura/
escritura del manifiesto corregidas. Se retira la estructura de campos supuestos
y se conserva el blob opaco de 1000 bytes. Evidencia y alcance en `AVATAR_ABI.md`.
La actualización 0.16 añade su decodificación y renderizado en el editor nativo.
La tabla siguiente describe exclusivamente las llamadas del núcleo invitado.

En `engine/xenia/src/xenia/kernel/xam/xam_avatar.cc`:

| Función | Implementación actual | Pendiente |
| --- | --- | --- |
| XamAvatarInitialize | Carga catálogo y esqueleto para ABI4/coordenadas 0 | Otros contratos y coordenadas |
| XamAvatarGetAssetsResultSize | Capacidades CPU/GPU validadas | Ajuste fino por máscara |
| XamAvatarGetAssets | Entrega modelos y texturas lineales, síncrona/asíncrona | Carryables, mipmaps y otros títulos |
| XamAvatarLoadAnimation | Stub | Carga y representación de animaciones |
| XamAvatarGetMetadataRandom | Manifiestos válidos con recursos compatibles del catálogo | Más variedad de rasgos |
| Enumeración / personalización | Stubs o implementación parcial | Editor original y enumeración completa |
| Manifest local / SetManifest | Conexión parcial a ajustes del perfil | Validar formato completo y uso por títulos |

`xam_avatar.h` conserva `X_AVATAR_METADATA` como datos opacos. La nueva ruta
de modelos devuelve éxito sólo después de construir y copiar los recursos.
No se activó `allow_avatar_initialization` de forma global. Tampoco se presume que
cargar AvatarEditor.xex implemente las funciones XAM que ese ejecutable importa.

## Trabajo necesario para completar la petición

1. Completar los objetos privados de animación para juegos que los soliciten.
2. Completar enumeración, carryables y contratos de otros títulos.
3. Ampliar la validación a otras carreras y juegos. En 0.18.1 se confirma
   J1 visible y animado en USA 1: Safety First de Doritos; se corrigió W=0
   en las posiciones del esqueleto (detalle en AVATAR_ABI.md).

El HDD aportado resuelve la disponibilidad de recursos; no resuelve esos
contratos ni implementa el subsistema de emulación. La build de prueba solicitada
permite comprobar la nueva entrega de modelos; no representa soporte completo
de todas las funciones de avatares Xbox 360.

### Prueba 0.18.2 — orientación en juegos

Se corrige la entrega de recursos derechos al juego que solicita coordenadas
izquierdas: se reflejan malla y esqueleto coherentemente, incluyendo normales
y orientación de triángulos. El editor conserva sus recursos derechos.
Regresión y AddressSanitizer correctos para ambos cuerpos; el usuario hará la
verificación visual de cabeza, carrera, salto y victoria en Doritos. No marcar
esta versión como validada visualmente hasta recibir su resultado.
