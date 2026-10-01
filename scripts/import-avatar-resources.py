"""Import the user's original avatar asset package without changing the source.

Supported input: read-only STFS (PIRS/LIVE) avatar package, not the whole HDD.
Use inspect-xbox-hdd.py first for an HDD image. Extracts only the two known TOCs.
"""
import argparse
import hashlib
import json
from pathlib import Path


def extract(package: Path) -> dict[str, bytes]:
    if package.stat().st_size > 64 * 1024 * 1024:
        raise ValueError("Not an avatar package: input exceeds 64 MiB")
    data = package.read_bytes()
    if len(data) < 0x400 or data[:4] not in (b"PIRS", b"LIVE") or not data[0x37B] & 1:
        raise ValueError("Expected an original read-only STFS avatar package")
    header = (int.from_bytes(data[0x340:0x344], "big") + 4095) & ~4095
    count = int.from_bytes(data[0x395:0x399], "big")
    table_count = int.from_bytes(data[0x37C:0x37E], "little")
    table_first = int.from_bytes(data[0x37E:0x381], "little")
    if not 0 < count < 28900 or not 0 < table_count <= 128 or header >= len(data):
        raise ValueError("Unsupported or corrupt STFS block table")

    def block(number: int) -> tuple[bytes, int]:
        if not 0 <= number < count:
            raise ValueError("STFS block outside package")
        physical = number
        for level in (170, 28900, 4913000):
            physical += (number + level) // level
            if number < level:
                break
        offset = header + physical * 4096
        payload = data[offset:offset + 4096]
        hash_block = 0 if number < 170 else (number // 170) * 171 + 1
        entry = header + hash_block * 4096 + (number % 170) * 24
        record = data[entry:entry + 24]
        if len(payload) != 4096 or len(record) != 24 or hashlib.sha1(payload).digest() != record[:20]:
            raise ValueError(f"Original package block {number} is truncated or has a wrong SHA-1")
        return payload, int.from_bytes(record[21:24], "big")

    def chain(first: int, size: int) -> bytes:
        if size <= 0 or size > count * 4096:
            raise ValueError("Invalid STFS file size")
        remaining, number, seen, result = size, first, set(), bytearray()
        while remaining:
            if number in seen:
                raise ValueError("Cyclic STFS file chain")
            seen.add(number)
            payload, number = block(number)
            copied = min(remaining, 4096)
            result.extend(payload[:copied])
            remaining -= copied
        if number != 0xFFFFFF:
            raise ValueError("STFS chain does not end at the expected size")
        return bytes(result)

    names = {"AvatarAssetPack.toc", "AvatarAssetPackLegacyV1.toc"}
    result = {}
    table = chain(table_first, table_count * 4096)
    for offset in range(0, len(table), 64):
        record = table[offset:offset + 64]
        length = record[40] & 63
        if not length or record[40] & 0x80:
            continue
        if length > 40:
            raise ValueError("Invalid STFS directory name")
        name = record[:length].decode("ascii", errors="strict")
        if name not in names:
            continue
        if name in result:
            raise ValueError("Duplicate avatar catalog")
        first = int.from_bytes(record[47:50], "little")
        size = int.from_bytes(record[52:56], "big")
        result[name] = chain(first, size)
    if set(result) != names:
        raise ValueError("The package does not contain both original avatar catalogs")
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("package", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    catalogs = extract(args.package)
    # Validate all destinations before writing any; preserve different copies.
    for name, data in catalogs.items():
        target = args.destination / name
        if target.exists() and target.read_bytes() != data:
            raise ValueError(f"A different catalog already exists: {target}")
    args.destination.mkdir(parents=True, exist_ok=True)
    report = {}
    for name, data in catalogs.items():
        target = args.destination / name
        if not target.exists():
            with target.open("xb") as output:
                output.write(data)
        report[name] = {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
