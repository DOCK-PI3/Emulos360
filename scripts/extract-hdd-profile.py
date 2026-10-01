"""Recover one named profile and its saved-game packages from a FATX HDD backup.

Source is opened read-only. Extraction requires a new destination and preserves
the original Content/XUID/title/type/package layout and package bytes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import tomllib


class Fatx:
    def __init__(self, stream, length):
        self.stream = stream
        self.offset = 0x130EB0000  # Retail HDD data partition, community FATX layout.
        self.length = length - self.offset
        magic, _, sectors, self.root = struct.unpack(">4I", self.read(self.offset, 16))
        if magic != 0x58544146 or sectors not in (2, 4, 8, 16, 32, 64, 128):
            raise ValueError("Unsupported FATX data partition")
        self.cluster_size = sectors * 512
        self.width = 4 if self.length // self.cluster_size + 1 >= 0xFFF0 else 2
        fat_size = ((self.length // self.cluster_size + 1) * self.width + 4095) & ~4095
        self.start = self.offset + 4096 + fat_size
        self.count = (self.length - 4096 - fat_size) // self.cluster_size
        self.fat = self.read(self.offset + 4096, fat_size)

    def read(self, offset, length):
        if offset < self.offset or length < 0 or offset + length > self.offset + self.length:
            raise ValueError("Read outside the FATX data partition")
        self.stream.seek(offset)
        data = self.stream.read(length)
        if len(data) != length:
            raise ValueError("Truncated backup")
        return data

    def chain(self, first):
        seen = set()
        while True:
            if first < 1 or first > self.count or first in seen:
                raise ValueError("Invalid/cyclic FATX chain")
            seen.add(first)
            yield self.start + (first - 1) * self.cluster_size
            first = int.from_bytes(self.fat[first*self.width:(first+1)*self.width], "big")
            if first >= (0xFFFFFFF8 if self.width == 4 else 0xFFF8):
                break

    def directory(self, first):
        result = []
        names = set()
        for address in self.chain(first):
            block = self.read(address, self.cluster_size)
            for position in range(0, len(block), 64):
                entry = block[position:position+64]
                size = entry[0]
                if size in (0, 255):
                    return result
                if size == 0xE5:
                    continue
                if size > 42:
                    raise ValueError("Invalid FATX filename")
                name = entry[2:2+size].decode("cp1252")
                if name in (".", "..") or any(c in name for c in '\\/:\0') or name.casefold() in names:
                    raise ValueError("Unsafe/duplicate FATX filename")
                names.add(name.casefold())
                cluster, length = struct.unpack_from(">II", entry, 44)
                result.append({"name": name, "directory": bool(entry[1] & 16), "cluster": cluster, "size": length})
        return result

    def child(self, directory, name):
        matches = [entry for entry in self.directory(directory) if entry["name"].casefold() == name.casefold()]
        if len(matches) != 1:
            return None
        return matches[0]

    def chunks(self, entry, limit=None):
        remaining = entry["size"] if limit is None else min(entry["size"], limit)
        if remaining == 0:
            return
        for address in self.chain(entry["cluster"]):
            chunk = self.read(address, min(remaining, self.cluster_size))
            yield chunk
            remaining -= len(chunk)
            if not remaining:
                return
        raise ValueError("File chain shorter than its declared length")

    def walk(self, directory, prefix, seen=None, depth=0):
        seen = set() if seen is None else seen
        if directory in seen or depth > 24:
            raise ValueError("Cyclic/deep directory tree")
        seen.add(directory)
        for entry in self.directory(directory):
            path = prefix + "/" + entry["name"]
            if entry["directory"]:
                yield from self.walk(entry["cluster"], path, seen, depth+1)
            else:
                yield path, entry


def text_at(header, offset, size=128):
    return header[offset:offset+size].decode("utf-16-be", errors="replace").split("\0", 1)[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path)
    parser.add_argument("--gamertag", required=True)
    parser.add_argument("--accounts", type=Path,
                        help="Profile list produced by the core after decrypting the extracted Account files")
    parser.add_argument("--destination", type=Path)
    args = parser.parse_args()
    matches = []
    candidates = []
    accounts = {}
    if args.accounts:
        with args.accounts.open("rb") as account_file:
            account_list = tomllib.load(account_file)
        if not account_list.get("ok"):
            raise ValueError("Account identification did not succeed")
        for account in account_list.get("profiles", []):
            xuid = account["xuid"].upper()
            if not re.fullmatch(r"[0-9A-F]{16}", xuid) or xuid in accounts:
                raise ValueError("Invalid or duplicate identified account")
            accounts[xuid] = account["gamertag"]
    report = {"source": str(args.image), "source_bytes": args.image.stat().st_size,
              "gamertag_requested": args.gamertag, "source_mode": "read-only", "files": []}
    with args.image.open("rb") as stream:
        fatx = Fatx(stream, report["source_bytes"])
        content = fatx.child(fatx.root, "Content")
        if not content or not content["directory"]:
            raise ValueError("No Content directory")
        for owner in fatx.directory(content["cluster"]):
            if not owner["directory"] or not re.fullmatch(r"[0-9A-Fa-f]{16}", owner["name"]) or owner["name"] == "0000000000000000":
                continue
            dashboard = fatx.child(owner["cluster"], "FFFE07D1")
            if not dashboard or not dashboard["directory"]:
                continue
            profiles = fatx.child(dashboard["cluster"], "00010000")
            if not profiles or not profiles["directory"]:
                continue
            for profile in fatx.directory(profiles["cluster"]):
                if profile["directory"]:
                    continue
                header = b"".join(fatx.chunks(profile, 0x1711))
                if header[:4] not in (b"CON ", b"LIVE", b"PIRS") or int.from_bytes(header[0x344:0x348], "big") != 0x10000:
                    continue
                names = [text_at(header, 0x411 + i*256, 256) for i in range(9)]
                gamertag = accounts.get(owner["name"].upper())
                candidates.append({"xuid": owner["name"], "gamertag": gamertag,
                                   "package_labels": sorted(set(n for n in names if n))})
                # STFS display names can be XUIDs; only the decrypted Account
                # identifies the actual gamertag. Do not guess from a label.
                if gamertag and gamertag.casefold() == args.gamertag.casefold():
                    matches.append((owner, profile))
        if len(matches) != 1:
            print(json.dumps({"matches": len(matches), "profiles": candidates}, ensure_ascii=False, indent=2))
            raise SystemExit("Expected exactly one matching original profile; no data extracted.")
        owner, profile = matches[0]
        report["xuid"] = owner["name"].upper()
        selected = []
        for path, entry in fatx.walk(owner["cluster"], "Content/" + owner["name"]):
            pieces = path.split("/")
            if len(pieces) >= 5 and (pieces[3].upper() == "00000001" or
                                     (pieces[2].upper() == "FFFE07D1" and pieces[3].upper() == "00010000")):
                selected.append((path, entry))
        report["selected_packages"] = len(selected)
        report["selected_bytes"] = sum(entry["size"] for _, entry in selected)
        if not args.destination:
            print(json.dumps({key: value for key, value in report.items() if key != "files"}, indent=2))
            return
        destination = args.destination.resolve()
        if destination.exists() or not destination.is_relative_to(Path.cwd().resolve()):
            raise ValueError("Use a NEW destination inside this workspace")
        destination.mkdir(parents=True)
        for path, entry in selected:
            target = (destination / path).resolve()
            if not target.is_relative_to(destination):
                raise ValueError("Invalid extraction destination")
            target.parent.mkdir(parents=True, exist_ok=True)
            digest = hashlib.sha256()
            header = b""
            with target.open("xb") as output:
                for chunk in fatx.chunks(entry):
                    if not header:
                        header = chunk[:0x1711]
                    output.write(chunk)
                    digest.update(chunk)
            # Read-back verification covers the bytes actually written to disk.
            with target.open("rb") as verify:
                check = hashlib.file_digest(verify, "sha256").hexdigest()
            if check != digest.hexdigest() or target.stat().st_size != entry["size"]:
                raise ValueError("Extracted file verification failed")
            report["files"].append({"path": path, "bytes": entry["size"], "sha256": check,
                                    "title_id": path.split("/")[2], "type": path.split("/")[3],
                                    "name": text_at(header, 0x411, 256), "title": text_at(header, 0x1691)})
        (destination / "manifest.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
        print(json.dumps({"destination": str(destination), "xuid": report["xuid"], "packages": len(selected), "bytes": report["selected_bytes"]}, indent=2))


if __name__ == "__main__":
    main()
