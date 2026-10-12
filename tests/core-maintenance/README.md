# Contratos de mantenimiento del core

El generador copia sin reescritura el validador fd_set, los tipos QoS y las
secciones de copia QoS/UPnP de `engine/xenia/src/xenia/kernel/xam/xam_net.cc`.
Usa los wrappers big endian y la capacidad de sockets reales del core.
Las fixtures sustituyen únicamente asignación, object table y UPnP; el banco
no enlaza los exports completos, no abre sockets reales ni valida gameplay.
`fragment-evidence.json` conserva la fuente y los hashes utilizados.

Casos: puntero nulo, conjuntos vacíos, todos los tamaños hasta el máximo,
contador excesivo, handle inválido/sentinel, UPnP ausente/activo/inactivo,
puerto asignado/mapeado, longitudes QoS 0/1/17/65535/65536/65537, asignación
fallida, respuesta HTTP fallida y datos ausentes. 204 assertions.

Windows, con MSVC configurado:

```powershell
cmake -S tests/core-maintenance -B local/core-maintenance-tests -G "Ninja Multi-Config"
cmake --build local/core-maintenance-tests --config Release
ctest --test-dir local/core-maintenance-tests -C Release --output-on-failure
```

Linux, usando ASan/UBSan:

```sh
cmake -S tests/core-maintenance -B /var/tmp/emulos-core-maintenance-tests -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DMAINTENANCE_SANITIZERS=ON
cmake --build /var/tmp/emulos-core-maintenance-tests
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir /var/tmp/emulos-core-maintenance-tests --output-on-failure
```

Los tests de CPU/JIT, UTF-8 y hilos se conservan en el parche 0002 y se
compilan en los targets reales `xenia-cpu-tests` / `xenia-base-tests`.
No son reemplazados por estas fixtures. [Validación de la entrega](../../docs/releases/0.6.0.md).
