# Pruebas de semántica del core

Banco de pruebas independiente para `Value::Extract` del Xenia Canary Netplay
fijado en `6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c`. No arranca juegos ni accede
a perfiles, partidas, contenido o configuración del emulador.

Desde la raíz del proyecto:

```powershell
./scripts/prepare-core-extract-candidate.ps1
./tests/core-semantics/run-windows.ps1
wsl -d Ubuntu -- bash /mnt/c/Users/mabe8/Documents/GitHub/Emulos360/tests/core-semantics/run-linux.sh
./scripts/build-core-extract-candidate.ps1 -BuildApp
```

El script de preparación solo copia y parchea
`local/core-maintenance-20261006/candidate/src/xenia/cpu/hir/value.cc`.
Comprueba la revisión, contexto y hash de partida; una copia existente con
cambios ajenos no se sobrescribe. Los scripts de tests requieren las herramientas
ya instaladas: MSVC/CMake/Ninja en Windows y Clang/CMake/Ninja en Ubuntu.

## Qué comprueba

- Cinco vectores conocidos y 4096 vectores con semilla fija.
- Extracción de todos los elementos válidos de 8/16/32/64 bits: 123030 casos.
- Los 256 valores posibles de un índice I8, en los cuatro tamaños: 4199424 casos.
- Un oráculo independiente que interpreta los bytes/shorts de palabras del
  invitado mediante desplazamientos; no vuelve a escribir la función probada.
- Fallo esperado del baseline con índices válidos, y lectura fuera del array
  detectada por UBSan en el baseline Linux. El candidato debe pasar ambos tests.
- ASan/UBSan y detección de leaks en Linux, con parada al primer error.

El baseline se prueba para reproducir el defecto; su fallo es esperado y nunca
se confunde con una aprobación. En Windows no se ejecuta el baseline con índices
fuera del array. Linux lo ejecuta únicamente bajo sanitizers.

## Alcance de compilación

Linux compila el `value.cc` real completo y enlaza sus métodos usados, eliminando
las secciones sin usar. Windows también compila el archivo completo, pero MSVC
exige resolver referencias a miembros no utilizados antes de eliminarlos. Para
el ejecutable de pruebas se extrae **sin modificar su cuerpo** `Value::Extract`
de ese mismo archivo y se compila con el header real. No se simulan HIRBuilder,
kernel, memoria o GPU para satisfacer referencias ajenas a esta función.

Las fuentes nuevas se compilan con warnings y el test con warnings como errores.
El archivo upstream completo conserva excepciones explícitas para sus warnings
históricos. La corrección no añade warnings al código nuevo.

Estos dos bancos de componente no ejecutan el JIT completo. I64 sirve de control
del contrato HIR y no afirma soporte de EXTRACT_I64 en el backend x64.

## Regresión dentro del JIT real

`build-core-extract-candidate.ps1` copia el fork actual (incluidos sus cambios
propios) a `local/core-maintenance-20261006/candidate`, comprueba hashes de las
fuentes y compila `xenia-cpu-tests` con MSVC Release. La copia conserva el parche
de `Extract`; los bytecodes de shaders se regeneran con el pipeline nativo.
No aplica candidatos al árbol activo. Requiere las dependencias nativas ya
preparadas y glslang/SPIRV-Tools disponibles en `.tools/` o `PATH`.

El test `extract_fold_contract_test.cc` genera **84 funciones x64 reales** con
HIR constante y dinámico. Compara resultados de 8/16/32 bits con el oráculo y
entre sí: **252 assertions**. A continuación se ejecuta toda la batería de CPU:
**247 casos, 1077 assertions**, aprobados el 6 de octubre de 2026. No se ejecutan
los índices inválidos dentro del JIT; esa comprobación corresponde a UBSan.

`emulos_app_flags_test.cc` registra el mismo flag transitorio y default que
`emulator_window.cc`, pues el ejecutable de tests no enlaza esa ventana y XAM
declara la variable. Utiliza el registro real de cvars. No sustituye ninguna
función de emulación. Solo entra en el ejecutable de tests.

Para generar `version.h`, el script crea un archivo `.git` temporal en la copia
que permite leer el HEAD del core. No ejecuta mutaciones Git en la copia y borra
solo ese archivo al terminar, incluso si falla. No borra los metadatos originales.
Los resultados, inventario y logs quedan en `local/core-maintenance-20261006/`.

`-BuildApp` enlaza además `xenia-app`, un ejecutable experimental completo en
`jit-windows-build/bin/Windows/Release/Emulos360-core.exe`. No lo instala ni lo
incluye en una release. Es una build del estado actual del fork, que contiene
cambios previos del proyecto; su hash y manifiesto identifican el candidato.

**Límites:** JIT completo validado en Windows; componente con ASan/UBSan en Linux.
No se valida gameplay ni Netplay y no se atribuye un aumento de FPS. La aceptación
para el producto requiere las comprobaciones del
[capítulo 13](../../docs/research/xenia-canary/13-MEJORAS-CPU-Y-VALIDACION.md).
