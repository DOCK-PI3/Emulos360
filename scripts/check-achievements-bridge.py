"""Read-only achievement IPC checks with synthetic GPDs in a disposable profile."""
import argparse
import hashlib
import os
import pathlib
import struct
import subprocess
import tempfile
import tomllib

project = pathlib.Path(__file__).resolve().parent.parent
core = project / "engine/xenia/build/bin/Windows/Release/Emulos360-core.exe"
parser = argparse.ArgumentParser()
parser.add_argument("--ui", type=pathlib.Path, help="Also run the Qt achievement screen test")
args = parser.parse_args()

def gpd(entries):
    payload = bytearray()
    table = bytearray()
    for section, identifier, data in entries:
        table += struct.pack(">HQII", section, identifier, len(payload), len(data))
        payload += data
    return (struct.pack(">6I", 0x58444246, 0x10000, len(entries), len(entries), 1, 1)
            + table + struct.pack(">II", len(payload), 0xFFFFFFFF) + payload)

def text(value):
    return (value + "\0").encode("utf-16-be")

def title(identifier, name, total, unlocked, points, earned):
    return (4, identifier, struct.pack(">5IH6sIQ", identifier, total, unlocked,
            points, earned, 0, bytes(6), 0, 0) + text(name))

def achievement(identifier, name, points, flags, timestamp=0):
    return (1, identifier, struct.pack(">5IQ", 28, identifier, identifier, points, flags, timestamp)
            + text(name) + text("Objetivo completado") + text("Completa el objetivo"))

with tempfile.TemporaryDirectory(prefix="emulos-achievements-", dir=project / "local") as folder:
    root = pathlib.Path(folder)
    engine = root / "engine"
    engine.mkdir()
    config = engine / "xenia-canary.config.toml"
    def command(action, xuid=""):
        output = root / "result.toml"
        subprocess.run([str(core), f"--config={config}", f"--storage_root={engine}",
                        f"--content_root={engine / 'content'}", f"--log_file={root / 'core.log'}",
                        f"--emulos_profile_command={action}", "--emulos_profile_name=LogrosTest",
                        f"--emulos_profile_xuid={xuid}", f"--emulos_profile_output={output}",
                        "--discord=false", "--network_mode=0", "--upnp=false",
                        *[f"--logged_profile_slot_{i}_xuid=" for i in range(4)]],
                       cwd=root, check=True, timeout=60, capture_output=True)
        return tomllib.loads(output.read_text(encoding="utf-8"))
    created = command("create")
    assert created["ok"], created
    xuid = created["profiles"][0]["xuid"]
    profile = next((engine / "content").rglob("Account")).parent
    empty = command("achievements", xuid)
    assert empty["ok"] and empty["achievementTitles"] == [], empty
    assert not command("achievements", "0000000000000000")["ok"]
    dashboard = profile / "FFFE07D1.gpd"
    game = profile / "EE000001.gpd"
    dashboard.write_bytes(gpd([title(0xEE000001, "Circuito de prueba", 3, 2, 60, 30),
                               title(0xEE000002, "Sin detalle", 1, 1, 5, 5)]))
    game_bytes = gpd([achievement(1, "Primera vuelta", 10, 0x20008, 134346240000000000),
                      achievement(2, "Victoria online", 20, 0x10008),
                      achievement(3, "Ruta secreta", 30, 4)])
    game.write_bytes(game_bytes)
    def hashes():
        return {str(p.relative_to(profile)): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in profile.rglob("*") if p.is_file()}
    before = hashes()
    result = command("achievements", xuid)
    assert result["ok"] and result["achievementXuid"] == xuid, result
    titles = {t["titleId"]: t for t in result["achievementTitles"]}
    row = titles["EE000001"]
    assert (row["total"], row["unlocked"], row["points"], row["totalPoints"]) == (3, 2, 30, 60)
    entries = row["achievements"]
    assert [a["unlocked"] for a in entries] == [True, True, False]
    assert entries[0]["unlockedAt"] > 0 and entries[2]["secret"]
    assert not titles["EE000002"]["detailsAvailable"]
    assert hashes() == before, "Reading achievements modified profile contents"
    if args.ui:
        config.write_text(f'[Profiles]\nlogged_profile_slot_0_xuid = "{xuid}"\n', encoding="utf-8")
        env = {k.upper(): v for k, v in os.environ.items()}
        env.update(EMULOS_ACHIEVEMENTS_DATA=str(root), EMULOS_TEST_CORE=str(core),
                   QT_QUICK_BACKEND="software", QT_QPA_FONTDIR="C:/Windows/Fonts")
        qt = project / ".tools/Qt/6.8.3/msvc2022_64/bin"
        msvc = pathlib.Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC"
        runtimes = sorted(msvc.glob("*/bin/Hostx64/x64"), reverse=True)
        env["PATH"] = os.pathsep.join([str(qt), *map(str, runtimes), env.get("PATH", "")])
        ui_log = args.ui.resolve().parent.parent / "achievements-ui-results.txt"
        subprocess.run([str(args.ui.resolve()), "achievementLibrary", "-platform", "offscreen", "-o", str(ui_log) + ",txt"],
                       env=env, check=True, timeout=90)
        print(ui_log.read_text(encoding="utf-8"))
        assert hashes() == before, "Qt screen modified profile contents"
    game.write_bytes(game_bytes[:-1])
    malformed = command("achievements", xuid)
    assert malformed["ok"] and not malformed["achievementTitles"][0]["detailsAvailable"]
    dashboard.write_bytes(b"XDBF" + bytes(20))
    assert not command("achievements", xuid)["ok"], "Corrupt dashboard was treated as an empty history"
    print("PASS: empty/unknown profiles, totals, offline/online unlocks, secrets, dates, missing/corrupt GPDs and unchanged profile hashes")
