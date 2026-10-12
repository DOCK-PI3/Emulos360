# Candidatos upstream preparados

> **Estado actualizado para 0.6.0:** CPU Extract, mutex/barrera Vulkan,
> bind/fd_set/QoS, reanudación POSIX y upper_ascii se incorporan a las fuentes
> mediante `patches/xenia-netplay/0002-core-maintenance-0.6.0.patch`.
> [Alcance, pruebas y gameplay pendiente](../../docs/releases/0.6.0.md).
> Los estados siguientes son históricos; viewport/fetch siguen sin integrar.

Estos parches conservan las atribuciones de Canary. Los cuatro candidatos
gráficos contienen sus diffs completos; el candidato CPU recoge únicamente
el hunk de implementación oficial, sin importar el conjunto amplio de tests.
**No forman parte del parche activo de Emulos360 ni los aplican los scripts de
compilación.** La preparación respeta la prioridad del usuario de mantener la
compatibilidad de los juegos existentes.

Revisión local comprobada: `6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c`.
Comprobación inicial: 2 de octubre de 2026. Los cuatro gráficos pasan `git apply --check` sobre el árbol
local; la comprobación inversa no encaja. Es evidencia de contexto, no de
gameplay ni de ausencia absoluta de soluciones equivalentes.

| Parche | Revisión upstream | Validación adicional |
| --- | --- | --- |
| [vulkan-layout-lock.patch](vulkan-layout-lock.patch) | `1b242658e535fbd12645e4d2c12693771075b738` | Archivo modificado compilado en copia aislada con MSVC Release |
| [vulkan-upload-barrier.patch](vulkan-upload-barrier.patch) | `c8f214e2a9b0209ca8a0ad6cb3dfb155fe545ae2` | Archivo modificado compilado en copia aislada con MSVC Release |
| [d3d12-invalid-vertex.patch](d3d12-invalid-vertex.patch) | `b083312b8b18e07e6e410b82104191f126722794` | Inspección estática; pendiente de compilación y ejecución |
| [negative-viewport.patch](negative-viewport.patch) | `6579cc2a4c8812a101c46fdae2e21df34a8521fd` | Inspección estática; pendiente de compilación y ejecución |
| [cpu-extract-fold.patch](cpu-extract-fold.patch) | `5ce7c336df060d043b0a2351889ec8789322d035` | 6 de octubre: error reproducido; 4199424 extracciones aprobadas en Windows/Linux, ASan/UBSan Linux, JIT real y batería CPU 247/247 en Windows |

En la comprobación inicial de los cuatro gráficos no se enlazó un core completo
ni se ejecutaron juegos, pruebas GPU o sanitizers.
Los hashes, alcance y criterios de aceptación están en
[el seguimiento del core](../../docs/research/xenia-canary/10-SEGUIMIENTO-Y-MEJORAS-DEL-CORE.md)
y [su evidencia](../../docs/research/xenia-canary/core-seguimiento-evidencia-20261002.json).

La validación CPU posterior se documenta aparte en
[el capítulo 13](../../docs/research/xenia-canary/13-MEJORAS-CPU-Y-VALIDACION.md).
Para reproducirla: `scripts/prepare-core-extract-candidate.ps1`, los bancos
de `tests/core-semantics/` y `scripts/build-core-extract-candidate.ps1`.
La copia conserva las fuentes y cambios propios actuales, exceptuando el
arreglo CPU y fuentes de tests. Ninguno de los cinco candidatos está integrado
al motor estable; gameplay y Netplay siguen pendientes de validación.

Una nueva revisión exige repetir los checks. Para una comprobación que no aplica
el primer parche, desde la raíz del proyecto:

```powershell
git -C engine/xenia apply --check ../../patches/xenia-upstream-candidates/vulkan-layout-lock.patch
```

Antes de integrar, completar las pruebas de ejecución y conservar el cambio en
el flujo reproducible `patches/xenia-netplay/0001-emulos360.patch`. La aplicación
en una copia de investigación no actualiza el motor del producto.

## Candidatos adicionales — 12 de octubre de 2026

Los siguientes cinco parches conservan el diff oficial completo, incluidos
los tests upstream de POSIX y ASCII. Pasan individualmente git apply --check
en la base local con sus cambios privados; la comprobación inversa falla.
No están aplicados, compilados ni conectados a los scripts del producto.

| Parche | Revisión | Límite principal pendiente |
| --- | --- | --- |
| [netplay-bind-null-upnp.patch](netplay-bind-null-upnp.patch) | `c7f5f15e1232a5f1aac77c40e2a171e12faec3f8` | UPnP ausente y puertos automáticos/mapeados |
| [netplay-fd-set-bounds.patch](netplay-fd-set-bounds.patch) | `493d2d32ddb0d4b3d7073b1e4cc98da49305e28c` | Límite permitido frente al assert estricto de host_set::Load |
| [netplay-qos-copy-size.patch](netplay-qos-copy-size.patch) | `4a904011fb1a18f30a2bb77e3477b3510e35d063` | Payload >65535 aún se estrecha a uint16; validar contrato y fallo de asignación |
| [posix-created-suspended-resume.patch](posix-created-suspended-resume.patch) | `1c81b867473806b2845a1946b2d185c9d888a319` | Compilación/pruebas Linux, estrés y regreso a suspensión |
| [utf8-upper-ascii.patch](utf8-upper-ascii.patch) | `19c23eb1123f891d44f6548429758b612fe0ae7e` | Tests descargados sin ejecutar; no hay efecto de gameplay identificado |

[Informe y dependencias](../../docs/research/xenia-canary/14-SEGUIMIENTO-20261012.md)
y [evidencia](../../docs/research/xenia-canary/core-seguimiento-evidencia-20261012.json).
Estos checks no prueban compatibilidad, rendimiento ni Netplay. Los cinco
candidatos previos mantienen su estado de validación histórica.
