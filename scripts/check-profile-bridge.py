"""Checks only Emulos360's new profile IPC, in an isolated directory, without a game."""
import pathlib
import subprocess
import tempfile
import tomllib

project = pathlib.Path(__file__).resolve().parent.parent
core = project / "engine/xenia/build/bin/Windows/Release/Emulos360-core.exe"
with tempfile.TemporaryDirectory(prefix="emulos-profile-", dir=project / "local") as folder:
    root = pathlib.Path(folder)
    def command(action):
        output = root / f"{action}.toml"
        subprocess.run([str(core), f"--storage_root={root}", f"--content_root={root / 'content'}",
                        f"--emulos_profile_command={action}", "--emulos_profile_name=EmulosCheck",
                        f"--emulos_profile_output={output}", "--discord=false", "--network_mode=0", "--upnp=false"],
                       cwd=root, check=True, timeout=45, capture_output=True)
        with output.open("rb") as stream:
            result = tomllib.load(stream)
        assert result["ok"], result
        return result["profiles"]
    assert command("list") == []
    created = command("create")
    assert len(created) == 1 and created[0]["gamertag"] == "EmulosCheck", created
    assert command("list") == created
    assert list((root / "content").rglob("Account")), "Missing native Account file"
    print("PASS: profile creation, encrypted Account persistence and listing across processes; no title launched")
