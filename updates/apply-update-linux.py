#!/usr/bin/env python3
"""Apply a verified Emulos360 Linux release after its parent process exits."""
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


def launch(install: Path, data: Path, log: Path) -> subprocess.Popen:
    # Keep loader/Qt errors outside the directories replaced by the update.
    # A separate rollback log must not overwrite the failed version's output.
    with log.open("wb") as output:
        return subprocess.Popen(
            [str(install / "Emulos360"), "--data-dir", str(data)],
            cwd=install, stdin=subprocess.DEVNULL, stdout=output,
            stderr=output, start_new_session=True,
        )


def startup_failure(process: subprocess.Popen, log: Path) -> str:
    message = f"La nueva versión se cerró durante el arranque (código {process.returncode}). Registro: {log}"
    try:
        with log.open("rb") as output:
            output.seek(max(0, log.stat().st_size - 4096))
            detail = output.read(4096).decode("utf-8", errors="replace").strip()
        if detail:
            message += "\n\n" + detail
    except OSError:
        pass
    return message


def main() -> int:
    if len(sys.argv) not in (7, 8) or (len(sys.argv) == 8 and sys.argv[7] != "--no-launch"):
        print("Uso: apply-update-linux.py INSTALACION ARCHIVO PID DATOS NOVEDADES TEMP [--no-launch]", file=sys.stderr)
        return 2
    install, archive, pid, data, news, temporary = map(Path, sys.argv[1:7])
    data = data.resolve(strict=True)
    installed = []
    saved = []
    work = None
    process = None
    news_written = False
    phase = "preparando la actualización"
    try:
        if install.is_symlink():
            raise RuntimeError("La carpeta de instalación no puede ser un enlace.")
        install = install.resolve(strict=True)
        archive = archive.resolve(strict=True)
        news = news.resolve(strict=True)
        temporary = temporary.resolve(strict=True)
        if not (install / "Emulos360").is_file():
            raise RuntimeError("La carpeta de instalación no es válida.")
        # A custom --data-dir inside a replaced runtime directory would be lost.
        if data == install or (install in data.parents and data.relative_to(install).parts[0] != "data"):
            raise RuntimeError("La carpeta de datos debe estar en data o fuera de la instalación.")
        phase = "esperando el cierre de Emulos360"
        wait_for_exit(int(str(pid)))
        phase = "creando la carpeta temporal junto a la instalación"
        work = Path(tempfile.mkdtemp(prefix=".emulos360-update-", dir=install.parent))
        extracted = work / "extracted"
        extracted.mkdir()
        backup = work / "backup"
        backup.mkdir()
        phase = "descomprimiendo el paquete"
        source = checked_archive(archive, extracted)
        phase = "sustituyendo los archivos"
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
        news_written = True
        if "--no-launch" not in sys.argv:
            phase = "reiniciando Emulos360"
            startup_log = data / "update-startup.log"
            process = launch(install, data, startup_log)
            time.sleep(3)
            if process.poll() is not None:
                raise RuntimeError(startup_failure(process, startup_log))
        shutil.rmtree(work, ignore_errors=True)
        shutil.rmtree(temporary, ignore_errors=True)
        return 0
    except Exception as exc:
        rollback_error = ""
        try:
            if process is not None and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=10)
            for name in reversed(installed):
                destination = install / name
                if destination.is_dir():
                    shutil.rmtree(destination)
                elif destination.exists():
                    destination.unlink()
            for name in reversed(saved):
                (backup / name).rename(install / name)
        except Exception as rollback:
            rollback_error = f"; también falló la restauración: {rollback}. Copia: {work}"
        message = f"No se pudo instalar la actualización ({phase}): {exc}{rollback_error}"
        if news_written:
            (data / "update-news.json").unlink(missing_ok=True)
        try:
            (data / "update-error.txt").write_text(message, encoding="utf-8")
            # The application consumes update-error.txt when it shows the dialog.
            # Retain a second copy for diagnosis after that restart.
            (data / "update-last-error.txt").write_text(message, encoding="utf-8")
        except OSError as report_error:
            print(f"No se pudo guardar el error en {data}: {report_error}", file=sys.stderr)
        print(message, file=sys.stderr)
        if not rollback_error:
            if work is not None:
                shutil.rmtree(work, ignore_errors=True)
            if "--no-launch" not in sys.argv and (install / "Emulos360").is_file():
                try:
                    launch(install, data, data / "update-rollback-startup.log")
                except OSError as restart_error:
                    print(f"No se pudo reabrir la versión anterior: {restart_error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        print(f"No se pudo iniciar el actualizador: {error}", file=sys.stderr)
        sys.exit(1)
