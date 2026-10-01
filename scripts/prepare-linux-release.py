#!/usr/bin/env python3
"""Name the Linux archive for the GitHub release and write its checksum."""
import hashlib
from pathlib import Path
import re
import shutil

root = Path(__file__).resolve().parents[1]
project = (root / "CMakeLists.txt").read_text(encoding="utf-8")
match = re.search(r"project\(Emulos360 VERSION (\d+\.\d+\.\d+)", project)
if not match:
    raise SystemExit("No se pudo leer la versión del proyecto.")
version = match.group(1)
source = root / "out/Emulos360-MultiP-linux.tar.gz"
if not source.is_file():
    raise SystemExit("Primero genera la build MultiP Linux.")
release = root / "out" / f"release-v{version}"
release.mkdir(parents=True, exist_ok=True)
target = release / f"Emulos360-v{version}-linux-x86_64.tar.gz"
if target.exists():
    raise SystemExit(f"Ya existe {target}; no se sobrescribe una release anterior.")
shutil.copy2(source, target)
with target.open("rb") as archive:
    digest = hashlib.file_digest(archive, "sha256").hexdigest()
target.with_name(target.name + ".sha256").write_text(
    f"{digest}  {target.name}\n", encoding="ascii")
print(f"Release Linux: {target} ({target.stat().st_size / 1048576:.1f} MiB)")
