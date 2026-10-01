# XBGuard: auditoría del paquete e integración en Emulos360

Fecha: 2026-09-18. Base del motor:
`xenia-canary/xenia-canary@6f9840568bb7dc9f5a9adb94801a99b18a7b2a60`.
Paquete local aportado para la prueba, fuera de este repositorio.

## Resultado

**La importación del paquete está implementada; su ejecución y autenticación no.**
Los archivos proporcionados son plugins DLL para el entorno de una consola
modificada, no un cliente que pueda ejecutarse con `--target=xbGuard.xex`.
Esta base no implementa el arranque de DashLaunch. Cargar tres archivos como
títulos, activar `allow_plugins` o copiar `launch.ini` junto al emulador no
resuelve esa diferencia.

## Documentación consultada

- [Sitio oficial de XBGuard](https://xbguard.live/): describe el servicio para
  consolas RGH/JTAG/XDK y remite a las instrucciones incluidas en la descarga
  y a su soporte. No documenta una integración con Xenia ni una API de login
  para emuladores en la página consultada.
- Los comentarios y valores de los dos `launch.ini` proporcionados constituyen
  la guía local de instalación: configuración de launch.xex v3, dispositivos
  `Hdd:`/`Usb:` y slots `[Plugins]`.
- El código local fijado de Xenia, especialmente `patcher/plugin_loader.cc`,
  `kernel/util/xex2_info.h`, `kernel/xbdm/xbdm_module.cc`,
  `kernel/xam/xam_net.cc` y `cpu/ppc/ppc_emit_control.cc`.

No se atribuyen requisitos binarios concretos a funciones que no se han podido
observar: los XEX están cifrados y comprimidos. La lectura de cabeceras no
demuestra qué llamadas hace cada módulo ni sustituye una auditoría de ejecución.

## Inventario y diferencias HDD/USB

| Slot | HDD | USB | Tipo observado |
| --- | --- | --- | --- |
| 1 | `Hdd:\xbdm.xex` | `Usb:\xbdm.xex` | XEX2 DLL, flags `0xE` |
| 2 | `Hdd:\xbGuard.xex` | `Usb:\xbGuard.xex` | XEX2 DLL, flags `0xA` |
| 3 | `Hdd:\JRPC2.xex` | `Usb:\JRPC2.xex` | XEX2 DLL, flags `0xA` |
| 4–5 | Vacíos | Vacíos | Sin módulos configurados |

Los tres binarios son idénticos byte a byte entre las dos variantes. La única
diferencia textual de `launch.ini` son las tres rutas de la tabla. `Hdd:` y
`Usb:` son dispositivos invitados de Xbox, no letras de unidades de Windows.
Todos declaran importaciones de `xam.xex` y `xboxkrnl.exe`, cifrado tipo 1 y
compresión tipo 2 en sus cabeceras. No se ejecutaron para obtener estos datos.

| Archivo | Bytes | SHA-256 |
| --- | ---: | --- |
| `xbdm.xex` | 53248 | `94a9b2ac546e77d81227bf91d69d386e8018e50a9ae49b390b044cccf0f083b1` |
| `xbGuard.xex` | 57344 | `3d859555b87b0528f301ec7b2a76086b022ada8b3b7627616d401f5155127f04` |
| `JRPC2.xex` | 73728 | `b8e28591711e946966b5f7ff5d5a242bf50746c0e60466d8a2e55ce671651959` |
| `launch.ini` HDD | — | `7647a64cb965c3454f1d5e2d28788f63563f5ac665fb701057c04efeacecbed1` |
| `launch.ini` USB | — | `94af339dfd5cc5749da330d4bc507e85f3fa10ac3083900ee2fc0525b4c449d7` |

Los hashes identifican los archivos locales revisados; no certifican su
procedencia ni que correspondan a la versión más reciente del proveedor.

## Cambios propios implementados

`app/community_package.*` inspecciona las cabeceras sin ejecutar módulos, lee
los slots de `launch.ini`, resuelve sus rutas dentro del paquete, detecta
dependencias ausentes y calcula SHA-256 de cada archivo. Admite la carpeta
contenedora `xbGuard` o una carpeta de variante directamente.

La importación conserva todos los archivos y subcarpetas de la variante en
`data/community/xbguard/<variante>-<identificador>/`. Mantiene `launch.ini`
íntegro, incluido su orden, y añade `emulos-package.json`. Compara inventario,
hashes y plugins tras copiar. Solo registra la nueva selección al completar la
operación; una importación fallida conserva la selección anterior. Los archivos
de Downloads permanecen intactos y los binarios no se añaden al repositorio.

Rechaza enlaces/junctions, rutas que salen del paquete, slots duplicados,
referencias HDD/USB incompatibles con la selección y cabeceras fuera de límites.
Límites: 32 MiB por archivo, 128 MiB por paquete, 1000 entradas y 16 niveles.
`emulos-package.json` es el nombre reservado al manifiesto propio. Una copia
fallida puede dejar una carpeta no seleccionada; no se ejecuta ni se activa.

`PlayerServices` persiste la selección y vuelve a inspeccionarla al iniciar.
El apartado Comunidad muestra la variante y el orden real de módulos, con el
estado explícito de ejecución no compatible. El lanzador de aplicaciones ahora
rechaza XEX2 con el indicador DLL para no tratarlos como títulos independientes.
No se aplican opciones DashLaunch a los ajustes de Xenia ni se cambia la red.

Ejemplos de importación e inspección sin arrancar XBGuard:

```powershell
./out/Emulos360/Emulos360.exe --inspect-xbguard 'C:\ruta\a\xbGuard' --xbguard-variant USB
./out/Emulos360/Emulos360.exe --import-xbguard 'C:\ruta\a\xbGuard' --xbguard-variant HDD
```

Estas operaciones producen JSON y terminan. Un resultado `complete: true`
significa **paquete íntegro**, no compatibilidad; `runtimeSupported` permanece
en `false`. En la interfaz: Comunidad → HDD/USB → Importar carpeta.

## Lo que falta para una sesión real

1. Un entorno de arranque y ciclo de vida compatible con DashLaunch, incluyendo
   montajes invitados y carga de DLL en el contexto correcto. El cargador actual
   de Xenia selecciona plugins por Title ID y hash mediante `plugins.toml`.
2. Resolver la interacción entre el `xbdm.xex` aportado y el módulo HLE XBDM
   que ya registra Xenia. Reemplazar el nombre del módulo no implementa su ABI.
3. Auditar imports, llamadas dinámicas, accesos a memoria y requisitos del kernel
   de los binarios. Hay llamadas de sistema no implementadas en el traductor
   PPC; no se puede prometer compatibilidad de plugins de consola arbitrarios.
4. Implementar las funciones de red y autenticación realmente requeridas, con
   documentación del proveedor o un cliente compatible con emuladores. En esta
   base `NetDll_XNetDnsLookup` es un stub que devuelve un resultado DNS de error,
   y otras funciones de red también están incompletas. Esto no afirma que sea
   la única ruta DNS ni que XBGuard importe directamente esa función.
5. Verificar la sesión contra el servicio con una identidad autorizada y
   observar un resultado real. Crear un perfil local no autentica ante XBGuard.

No hay una modificación pequeña de configuración que complete estos puntos.
Este cambio no presenta una importación de archivos como un login funcional.
