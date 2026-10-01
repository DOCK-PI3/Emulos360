#!/usr/bin/env python3
"""Apply a verified Emulos360 Linux release after its parent process exits."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile
import time


def wait_for_exit(pid: int) -> None:
    if pid <= 0:
        return
    for _ in range(120):
        try:
            os.kill(pid, 0)
        except ProcessLookupError:
            return
        time.sleep(1)
    raise RuntimeError("Emulos360 no se cerró en dos minutos.")


def checked_archive(archive: Path, staging: Path) -> Path:
    prefix = "Emulos360-MultiP-linux/"
    with tarfile.open(archive, "r:gz") as package:
        members = package.getmembers()
        if len(members) > 100_000:
            raise RuntimeError("El paquete contiene demasiados archivos.")
        total = 0
        for member in members:
            name = member.name.rstrip("/")
            if name == prefix.rstrip("/"):
                continue
            if not name.startswith(prefix):
                raise RuntimeError("El paquete contiene rutas fuera de Emulos360.")
            parts = Path(name[len(prefix):]).parts
            if not parts or any(part in (".", "..") for part in parts) or parts[0] == "data":
                raise RuntimeError("El paquete contiene una ruta no permitida.")
            if not (member.isfile() or member.isdir()):
                raise RuntimeError("El paquete contiene un enlace o archivo especial.")
            total += member.size
            if total > 4 * 1024**3:
                raise RuntimeError("El paquete descomprimido es demasiado grande.")
        package.extractall(staging, filter="data")
    source = staging / prefix.rstrip("/")
    for required in ("Emulos360", "engine/Emulos360-core", "updates/apply-update-linux.py"):
        if not (source / required).is_file():
            raise RuntimeError(f"Falta {required} en la actualización.")
    return source


def launch(install: Path, data: Path) -> subprocess.Popen:
    return subprocess.Popen(
        [str(install / "Emulos360"), "--data-dir", str(data)],
        cwd=install, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL, start_new_session=True,
    )


def main() -> int:
    if len(sys.argv) not in (7, 8) or (len(sys.argv) == 8 and sys.argv[7] != "--no-launch"):
        print("Uso: apply-update-linux.py INSTALACION ARCHIVO PID DATOS NOVEDADES TEMP [--no-launch]", file=sys.stderr)
        return 2
    install, archive, pid, data, news, temporary = map(Path, (sys.argv[1], sys.argv[2],
                                                                sys.argv[3], sys.argv[4],
                                                                sys.argv[5], sys.argv[6]))
    install = install.resolve(strict=True)
    data = data.resolve(strict=True)
    archive = archive.resolve(strict=True)
    news = news.resolve(strict=True)
    temporary = temporary.resolve(strict=True)
    if not (install / "Emulos360").is_file() or install.is_symlink():
        raise RuntimeError("La carpeta de instalación no es válida.")
    wait_for_exit(int(str(pid)))
    work = Path(tempfile.mkdtemp(prefix=".emulos360-update-", dir=install.parent))
    extracted = work / "extracted"
    extracted.mkdir()
    backup = work / "backup"
    backup.mkdir()
    installed = []
    saved = []
    try:
        source = checked_archive(archive, extracted)
        for item in source.iterdir():
            destination = install / item.name
            if item.is_symlink() or destination.is_symlink():
                raise RuntimeError("Se detectó un enlace en la instalación.")
            if destination.exists():
                destination.rename(backup / item.name)
                saved.append(item.name)
            item.rename(destination)
            installed.append(item.name)
        shutil.copy2(news, data / "update-news.json")
        if "--no-launch" not in sys.argv:
            process = launch(install, data)
            time.sleep(3)
            if process.poll() is not None:
                raise RuntimeError("La nueva versión se cerró durante el arranque.")
        shutil.rmtree(work, ignore_errors=True)
        shutil.rmtree(temporary, ignore_errors=True)
        return 0
    except Exception as exc:
        rollback_error = ""
        try:
            for name in reversed(installed):
                destination = install / name
                if destination.is_dir():
                    shutil.rmtree(destination)
                elif destination.exists():
                    destination.unlink()
            for name in reversed(saved):
                (backup / name).rename(install / name)
        except Exception as rollback:
            rollback_error = f"; también falló la restauración: {rollback}. Copia: {backup}"
        message = f"No se pudo instalar la actualización: {exc}{rollback_error}"
        (data / "update-news.json").unlink(missing_ok=True)
        (data / "update-error.txt").write_text(message, encoding="utf-8")
        print(message, file=sys.stderr)
        if not rollback_error:
            shutil.rmtree(work, ignore_errors=True)
            if "--no-launch" not in sys.argv:
                try:
                    launch(install, data)
                except OSError:
                    pass
        return 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        print(f"No se pudo iniciar el actualizador: {error}", file=sys.stderr)
        sys.exit(1)
