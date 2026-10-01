"""Copy original Xbox 360 saves from a readable USB Content tree, without modifying it.

The owner XUID is preserved, not treated as proof of a gamertag. No content is
installed into the emulator. Destinations must be new and inside the workspace.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import stat


def regular(path, directory=False):
    info = path.lstat()
    if info.st_file_attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT:
        raise ValueError(f"Reparse point rejected: {path}")
    if not (stat.S_ISDIR(info.st_mode) if directory else stat.S_ISREG(info.st_mode)):
        raise ValueError(f"Unexpected file type: {path}")


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def text(header, offset, length):
    return header[offset:offset+length].decode("utf-16-be", errors="replace").split("\0", 1)[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("content", type=Path)
    parser.add_argument("xuid")
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    xuid = args.xuid.upper()
    if not re.fullmatch(r"[0-9A-F]{16}", xuid) or xuid == "0000000000000000":
        raise ValueError("Expected a nonzero owner XUID")
    regular(args.content, True)
    owner = args.content / xuid
    regular(owner, True)
    destination = args.destination.resolve()
    workspace = Path(__file__).resolve().parent.parent
    if destination.exists() or not destination.is_relative_to(workspace):
        raise ValueError("Use a new destination inside the workspace")
    selected = []
    for title in sorted(owner.iterdir()):
        regular(title, True)
        if not re.fullmatch(r"[0-9A-Fa-f]{8}", title.name):
            raise ValueError(f"Unexpected title directory: {title}")
        for kind in sorted(title.iterdir()):
            regular(kind, True)
            if kind.name.upper() not in ("00000001", "00010000"):
                continue
            for source in sorted(kind.iterdir()):
                regular(source)
                with source.open("rb") as stream:
                    header = stream.read(0x1711)
                if len(header) != 0x1711 or header[:4] not in (b"CON ", b"LIVE", b"PIRS"):
                    raise ValueError(f"Invalid package header: {source}")
                content_type = int.from_bytes(header[0x344:0x348], "big")
                title_id = header[0x360:0x364].hex().upper()
                profile_id = header[0x371:0x379].hex().upper()
                if content_type != int(kind.name, 16) or title_id != title.name.upper() or profile_id != xuid:
                    raise ValueError(f"Package identity disagrees with its directory: {source}")
                names = [text(header, 0x411+i*256, 256) for i in range(9)]
                selected.append((source, {"path": "Content/"+source.relative_to(args.content).as_posix(),
                    "title_id": title_id, "type": f"{content_type:08X}", "profile_id": profile_id,
                    "title": text(header, 0x1691, 128), "display_names": sorted(set(n for n in names if n))}))
    if not selected:
        raise ValueError("No profile/save packages found")
    destination.mkdir(parents=True)
    report = {"source": str(args.content), "source_mode": "read-only", "xuid": xuid,
              "gamertag": None, "files": [], "complete": False}
    for source, metadata in selected:
        target = destination / metadata["path"]
        target.parent.mkdir(parents=True, exist_ok=True)
        before = source.stat()
        copied_hash = hashlib.sha256()
        with source.open("rb") as reader, target.open("xb") as writer:
            while chunk := reader.read(1024*1024):
                writer.write(chunk)
                copied_hash.update(chunk)
        after = source.stat()
        copied = digest(target)
        original = digest(source)
        if (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns) or target.stat().st_size != before.st_size or copied != original or copied != copied_hash.hexdigest():
            raise ValueError(f"Copy verification failed: {source}")
        metadata.update(bytes=before.st_size, sha256=copied)
        report["files"].append(metadata)
    report["complete"] = True
    report["save_packages"] = sum(f["type"] == "00000001" for f in report["files"])
    report["profile_packages"] = sum(f["type"] == "00010000" for f in report["files"])
    report["bytes"] = sum(f["bytes"] for f in report["files"])
    (destination / "manifest.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
