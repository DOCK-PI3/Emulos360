# Serie del core principal

Base fijada: `6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c` (Canary Netplay).
Aplicar en este orden con `scripts/prepare-engine.ps1` o `prepare-engine.sh`:

1. `0001-emulos360.patch`: extensiones propias existentes; conservado intacto.
2. `0002-core-maintenance-0.6.0.patch`: ocho correcciones seleccionadas,
   adaptaciones de límites y tests de CPU/hilos/UTF-8.

[Alcance, revisiones originales, pruebas y límites](../../docs/releases/0.6.0.md).
Las atribuciones se conservan en los parches individuales bajo
`patches/xenia-upstream-candidates/`; la serie agregada no implica actualizar
toda la base ni eliminar los cambios propios. Las comprobaciones de contexto
no equivalen a gameplay ni validación de todas las sesiones Netplay.

Los parches individuales son referencias upstream. Las adaptaciones locales
(fd_set/QoS y cast de handle) pueden cambiar su contexto inverso: comprobar
presencia semántica y el agregado 0002, no concluir ausencia por un raw check
fallido. El agregado pasa reverse-check exacto sobre las fuentes actuales y
forward-check sobre el baseline conservado.
