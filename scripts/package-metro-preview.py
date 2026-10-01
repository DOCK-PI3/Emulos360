"""Create a portable Metro preview without copying the user's profiles or saves."""

from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
import hashlib

project = Path(__file__).resolve().parent.parent
source = project / "out" / "Emulos360"
destination = project / "out" / "Emulos360-Metro-Correcciones-2026-09-24.zip"
required = (
    "Emulos360.exe",
    "engine/Emulos360-core.exe",
    "XEXplugins/MetroDashboard/default.xex",
    "XEXplugins/MetroDashboard/font.png",
)
for name in required:
    if not (source / name).is_file():
        raise SystemExit(f"Falta {name}; ejecuta scripts/build-ui.ps1 -Package")

readme = """Emulos360 - Dashboard Metro (vista previa)

Extrae la carpeta Emulos360 y ejecuta Emulos360.exe. F11 o el boton
Modo consola abre el dashboard .xex. En un juego, A lo ejecuta, Y abre el
selector de caratulas y B vuelve. LB/RB cambia de seccion, LT/RT pasa
paginas y mantener Guia 2,5 s abre Sistema. En la biblioteca del PC,
F6/Intro/Escape equivalen a Y/A/B. Dentro del selector, la cruceta o el
stick resalta una caratula, A la aplica y B cierra.

Esta revision separa las pestañas, las etiquetas de las salas y los textos
del dashboard; al cerrarlo, la biblioteca vuelve maximizada. La apariencia
del cuello y la cabeza del avatar en Doritos sigue en investigacion.

Este ZIP no contiene perfiles, partidas, juegos ni ajustes privados.
Para probar con tus datos, usa la carpeta out/Emulos360 del proyecto, que
conserva data, o copia tu carpeta data a la nueva Emulos360 antes de abrirla.
El menu Apagar Windows solicita una segunda confirmacion.
"""

with ZipFile(destination, "w", ZIP_DEFLATED, compresslevel=6) as archive:
    archive.writestr("Emulos360/LEEME-METRO.txt", readme)
    for file in sorted(source.rglob("*")):
        if not file.is_file():
            continue
        relative = file.relative_to(source)
        if relative.parts[0].lower() == "data" or file.suffix.lower() == ".log":
            continue
        archive.write(file, Path("Emulos360") / relative)

with ZipFile(destination) as archive:
    bad = archive.testzip()
    if bad:
        raise SystemExit(f"ZIP dañado: {bad}")
    names = set(archive.namelist())
    for name in required:
        if f"Emulos360/{name}" not in names:
            raise SystemExit(f"Falta en el ZIP: {name}")
    if any(name.startswith("Emulos360/data/") for name in names):
        raise SystemExit("El ZIP incluiría datos privados")

hash_value = hashlib.sha256(destination.read_bytes()).hexdigest()
print(f"{destination}\n{destination.stat().st_size:,} bytes\nSHA-256 {hash_value}")
