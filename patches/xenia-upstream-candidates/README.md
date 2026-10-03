# Candidatos upstream preparados

Estos parches conservan exactamente los diffs y atribuciones de Canary.
**No forman parte del parche activo de Emulos360 ni los aplican los scripts de
compilación.** La preparación respeta la prioridad del usuario de mantener la
compatibilidad de los juegos existentes.

Revisión local comprobada: `6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c`.
Fecha: 2 de octubre de 2026. Los cuatro pasan `git apply --check` sobre el árbol
local; la comprobación inversa no encaja. Es evidencia de contexto, no de
gameplay ni de ausencia absoluta de soluciones equivalentes.

| Parche | Revisión upstream | Validación adicional |
| --- | --- | --- |
| [vulkan-layout-lock.patch](vulkan-layout-lock.patch) | `1b242658e535fbd12645e4d2c12693771075b738` | Archivo modificado compilado en copia aislada con MSVC Release |
| [vulkan-upload-barrier.patch](vulkan-upload-barrier.patch) | `c8f214e2a9b0209ca8a0ad6cb3dfb155fe545ae2` | Archivo modificado compilado en copia aislada con MSVC Release |
| [d3d12-invalid-vertex.patch](d3d12-invalid-vertex.patch) | `b083312b8b18e07e6e410b82104191f126722794` | Inspección estática; pendiente de compilación y ejecución |
| [negative-viewport.patch](negative-viewport.patch) | `6579cc2a4c8812a101c46fdae2e21df34a8521fd` | Inspección estática; pendiente de compilación y ejecución |

No se enlazó un core completo ni se ejecutaron juegos, pruebas GPU o sanitizers.
Los hashes, alcance y criterios de aceptación están en
[el seguimiento del core](../../docs/research/xenia-canary/10-SEGUIMIENTO-Y-MEJORAS-DEL-CORE.md)
y [su evidencia](../../docs/research/xenia-canary/core-seguimiento-evidencia-20261002.json).

Una nueva revisión exige repetir los checks. Para una comprobación que no aplica
el primer parche, desde la raíz del proyecto:

```powershell
git -C engine/xenia apply --check ../../patches/xenia-upstream-candidates/vulkan-layout-lock.patch
```

Antes de integrar, completar las pruebas de ejecución y conservar el cambio en
el flujo reproducible `patches/xenia-netplay/0001-emulos360.patch`. La aplicación
en una copia de investigación no actualiza el motor del producto.
