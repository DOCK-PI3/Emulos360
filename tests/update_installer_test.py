"""Check Linux update replacement, restart diagnostics and rollback in isolated folders."""
from io import BytesIO
from pathlib import Path
import importlib.util
import json
import os
import signal
import subprocess
import sys
import tarfile
import tempfile
import time
import unittest
from unittest import mock


HELPER = Path(__file__).resolve().parents[1] / "updates" / "apply-update-linux.py"
PREFIX = "Emulos360-MultiP-linux/"


def app(*, failure=False, label="new") -> bytes:
    body = ("import sys\nprint('libExample.so: cannot open shared object file', file=sys.stderr)\nsys.exit(72)\n" if failure else
            "import os, pathlib, sys, time\n"
            "data = pathlib.Path(sys.argv[sys.argv.index('--data-dir') + 1])\n"
            f"(data / '{label}-pid').write_text(str(os.getpid()))\n"
            f"print('{label} application started', flush=True)\n"
            "time.sleep(120)\n")
    return (f"#!{sys.executable}\n" + body).encode()


def package(path: Path, *, application: bytes, extra=None, omit_core=False) -> None:
    payload = {
        "Emulos360": application,
        "engine/Emulos360-core": b"new engine",
        "updates/apply-update-linux.py": b"helper",
    }
    if omit_core:
        del payload["engine/Emulos360-core"]
    with tarfile.open(path, "w:gz") as archive:
        for name, content in payload.items():
            info = tarfile.TarInfo(PREFIX + name)
            info.mode = 0o755 if name in ("Emulos360", "engine/Emulos360-core") else 0o644
            info.size = len(content)
            archive.addfile(info, BytesIO(content))
        if extra is not None:
            archive.addfile(extra, BytesIO(b"x") if extra.isfile() else None)


class LinuxUpdaterTest(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory(prefix="emulos-updater-test-")
        self.root = Path(self.folder.name)
        self.install = self.root / "install"
        self.data = self.install / "data"
        (self.install / "engine").mkdir(parents=True)
        self.data.mkdir()
        (self.install / "engine/edge").mkdir()
        (self.install / "engine/edge/old-core").write_bytes(b"retired core")
        (self.data / "cores/edge/content").mkdir(parents=True)
        (self.data / "cores/edge/content/legacy-save").write_bytes(b"legacy save")
        self.old_app = app(label="old")
        (self.install / "Emulos360").write_bytes(self.old_app)
        (self.install / "Emulos360").chmod(0o755)
        (self.install / "engine/Emulos360-core").write_bytes(b"old engine")
        self.sentinels = {"profile.bin": b"my profile", "save.bin": b"my save", "cover.bin": b"my cover"}
        for name, content in self.sentinels.items():
            (self.data / name).write_bytes(content)
        self.temporary = self.root / "download"
        self.temporary.mkdir()
        self.archive = self.temporary / "release.tar.gz"
        self.news = self.temporary / "news.json"
        self.news.write_text(json.dumps({"version": "0.4.0", "notes": "Novedades"}), encoding="utf-8")

    def tearDown(self):
        # Only terminate the fixture applications launched by this test case.
        for name in ("new-pid", "old-pid"):
            marker = self.data / name
            if marker.exists():
                pid = int(marker.read_text())
                try:
                    command = Path(f"/proc/{pid}/cmdline").read_bytes().split(b"\0")
                    if os.fsencode(str(self.install / "Emulos360")) in command:
                        os.kill(pid, signal.SIGTERM)
                except (FileNotFoundError, ProcessLookupError):
                    pass
        self.folder.cleanup()

    def args(self, launch=False):
        result = [str(HELPER), str(self.install), str(self.archive), "0", str(self.data), str(self.news), str(self.temporary)]
        if not launch:
            result.append("--no-launch")
        return result

    def run_helper(self, launch=False):
        result = subprocess.run([sys.executable, *self.args(launch)], capture_output=True, text=True, timeout=20)
        for name, content in self.sentinels.items():
            self.assertEqual((self.data / name).read_bytes(), content)
        return result

    def assert_restored(self):
        self.assertEqual((self.install / "Emulos360").read_bytes(), self.old_app)
        self.assertEqual((self.install / "engine/Emulos360-core").read_bytes(), b"old engine")
        self.assertEqual((self.install / "engine/edge/old-core").read_bytes(), b"retired core")

    def test_replacement_preserves_data_and_executable_modes(self):
        package(self.archive, application=app())
        result = self.run_helper()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((self.install / "engine/Emulos360-core").read_bytes(), b"new engine")
        self.assertTrue((self.install / "engine/Emulos360-core").stat().st_mode & 0o111)
        self.assertTrue((self.data / "update-news.json").is_file())
        self.assertFalse((self.install / "engine/edge").exists())
        self.assertEqual((self.data / "cores/edge/content/legacy-save").read_bytes(), b"legacy save")
        self.assertFalse(self.temporary.exists())

    def test_restarts_new_version(self):
        package(self.archive, application=app())
        result = self.run_helper(launch=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue((self.data / "new-pid").exists())
        self.assertIn("new application started", (self.data / "update-startup.log").read_text())
        self.assertFalse((self.data / "old-pid").exists())

    def test_startup_failure_restores_and_retains_original_error(self):
        package(self.archive, application=app(failure=True))
        result = self.run_helper(launch=True)
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assert_restored()
        detail = (self.data / "update-error.txt").read_text(encoding="utf-8")
        self.assertIn("72", detail)
        self.assertIn("libExample.so", detail)
        self.assertIn("reiniciando Emulos360", detail)
        self.assertEqual((self.data / "update-last-error.txt").read_text(encoding="utf-8"), detail)
        self.assertIn("libExample.so", (self.data / "update-startup.log").read_text())
        self.assertNotIn("libExample.so", (self.data / "update-rollback-startup.log").read_text())
        deadline = time.monotonic() + 5
        while "old application started" not in (self.data / "update-rollback-startup.log").read_text() and time.monotonic() < deadline:
            time.sleep(0.02)
        self.assertTrue((self.data / "old-pid").exists())
        self.assertIn("old application started", (self.data / "update-rollback-startup.log").read_text())
        self.assertFalse((self.data / "update-news.json").exists())

    def test_rejects_data_traversal_and_links(self):
        for name, link in (("data/profile.bin", False), ("../outside", False), ("engine/link", True)):
            with self.subTest(name=name):
                info = tarfile.TarInfo(PREFIX + name)
                if link:
                    info.type = tarfile.SYMTYPE
                    info.linkname = "/etc/passwd"
                else:
                    info.size = 1
                package(self.archive, application=app(), extra=info)
                result = self.run_helper()
                self.assertEqual(result.returncode, 1, result.stderr)
                self.assert_restored()
                self.assertFalse((self.root / "outside").exists())

    def test_rejects_missing_main_core(self):
        package(self.archive, application=app(), omit_core=True)
        result = self.run_helper()
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertIn("Emulos360-core", result.stderr)
        self.assert_restored()

    def test_preparation_error_is_reported_without_deleting_previous_news(self):
        spec = importlib.util.spec_from_file_location("installer", HELPER)
        helper = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(helper)
        package(self.archive, application=app())
        old_news = b'{"version":"previous"}'
        (self.data / "update-news.json").write_bytes(old_news)
        with mock.patch.object(sys, "argv", self.args()), mock.patch.object(helper.tempfile, "mkdtemp", side_effect=PermissionError("permission denied: installation parent")):
            self.assertEqual(helper.main(), 1)
        self.assert_restored()
        self.assertEqual((self.data / "update-news.json").read_bytes(), old_news)
        self.assertIn("permission denied", (self.data / "update-last-error.txt").read_text(encoding="utf-8"))

    def test_custom_data_inside_replaced_directory_is_rejected(self):
        other_data = self.install / "engine/private-data"
        other_data.mkdir()
        sentinel = other_data / "profile.bin"
        sentinel.write_bytes(b"profile")
        package(self.archive, application=app())
        args = self.args()
        args[4] = str(other_data)
        result = subprocess.run([sys.executable, *args], capture_output=True, text=True, timeout=20)
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertEqual(sentinel.read_bytes(), b"profile")
        self.assert_restored()


if __name__ == "__main__":
    if sys.platform != "linux":
        raise SystemExit("Run this test on Linux: python3 tests/update_installer_test.py")
    unittest.main(verbosity=2)
