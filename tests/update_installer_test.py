"""Exercise the Linux installer without replacing a real application."""
from io import BytesIO
from pathlib import Path
import json
import subprocess
import sys
import tarfile
import tempfile


HELPER = Path(__file__).resolve().parents[1] / "updates" / "apply-update-linux.py"


def package(path: Path, *, bad_data: bool = False) -> None:
    payload = {
        "Emulos360": b"new app",
        "engine/Emulos360-core": b"new engine",
        "updates/apply-update-linux.py": b"helper",
    }
    if bad_data:
        payload["data/profile.bin"] = b"injected"
    with tarfile.open(path, "w:gz") as archive:
        for name, content in payload.items():
            info = tarfile.TarInfo("Emulos360-MultiP-linux/" + name)
            info.mode = 0o755
            info.size = len(content)
            archive.addfile(info, BytesIO(content))


def case(root: Path, *, bad_data: bool) -> None:
    install = root / "install"
    data = install / "data"
    engine = install / "engine"
    data.mkdir(parents=True)
    engine.mkdir()
    (install / "Emulos360").write_bytes(b"old app")
    (engine / "Emulos360-core").write_bytes(b"old engine")
    (data / "profile.bin").write_bytes(b"my profile")
    temporary = root / "download"
    temporary.mkdir()
    archive = temporary / "release.tar.gz"
    news = temporary / "news.json"
    package(archive, bad_data=bad_data)
    news.write_text(json.dumps({"version": "0.3.0", "notes": "Novedades"}), encoding="utf-8")
    result = subprocess.run(
        [sys.executable, str(HELPER), str(install), str(archive), "0",
         str(data), str(news), str(temporary), "--no-launch"],
        capture_output=True, text=True,
    )
    assert (result.returncode != 0) == bad_data, result.stderr
    assert (data / "profile.bin").read_bytes() == b"my profile"
    assert (install / "Emulos360").read_bytes() == (b"old app" if bad_data else b"new app")
    assert (engine / "Emulos360-core").read_bytes() == (b"old engine" if bad_data else b"new engine")
    assert (data / ("update-error.txt" if bad_data else "update-news.json")).is_file()


with tempfile.TemporaryDirectory(prefix="emulos-updater-test-") as folder:
    root = Path(folder)
    case(root / "success", bad_data=False)
    case(root / "rejected", bad_data=True)
print("Linux updater: replacement, profile preservation and malicious data rejection passed")
