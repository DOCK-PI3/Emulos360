"""Read-only FATX system-partition inventory from a retail Xbox 360 HDD image.

Geometry reference: hetelek/Velocity XboxInternals/Fatx/FatxConstants.h and
FatxDrive::processBootSector (community reverse engineering, not an XDK ABI).
Never mounts or writes the source. Does not inspect profiles/security sectors.
"""
import argparse
import json
import struct
import hashlib
from pathlib import Path


def inventory(source, extract=None):
    results = []
    with source.open('rb') as stream:
        for name, offset, length in [('SystemAuxiliary', 0x10C080000, 0xCE30000),
                                     ('SystemExtended', 0x118EB0000, 0x8000000),
                                     ('Compatibility', 0x120EB0000, 0x10000000)]:
            def read(at, size):
                if at < offset or at + size > offset + length:
                    raise ValueError('Read outside partition')
                stream.seek(at)
                data = stream.read(size)
                if len(data) != size:
                    raise ValueError('Truncated image')
                return data

            header = read(offset, 16)
            part = {'name': name, 'offset': hex(offset), 'magic': header[:4].hex(), 'files': []}
            results.append(part)
            if header[:4] != b'XTAF':
                continue
            _, _, sectors, root = struct.unpack('>4I', header)
            if sectors not in (2, 4, 8, 16, 32, 64, 128):
                raise ValueError('Invalid cluster geometry')
            cluster_size = sectors * 512
            width = 4 if length // cluster_size + 1 >= 0xFFF0 else 2
            fat_size = ((length // cluster_size + 1) * width + 4095) & ~4095
            start = offset + 4096 + fat_size
            count = (length - 4096 - fat_size) // cluster_size
            visited_dirs = set()

            def chain(first):
                seen = set()
                cluster = first
                while True:
                    if not 1 <= cluster <= count or cluster in seen:
                        raise ValueError('Invalid or cyclic FAT chain')
                    seen.add(cluster)
                    yield start + (cluster - 1) * cluster_size
                    cluster = int.from_bytes(read(offset + 4096 + cluster * width, width), 'big')
                    if cluster >= (0xFFF8 if width == 2 else 0xFFFFFFF8):
                        break

            def walk(cluster, prefix='', depth=0):
                if depth > 24 or cluster in visited_dirs or len(part['files']) > 100000:
                    raise ValueError('Directory traversal limit')
                visited_dirs.add(cluster)
                for address in chain(cluster):
                    block = read(address, cluster_size)
                    for pos in range(0, len(block), 64):
                        entry = block[pos:pos+64]
                        n = entry[0]
                        if n in (0, 255):
                            return
                        if n == 0xE5:
                            continue
                        if n > 42:
                            raise ValueError('Invalid filename length')
                        filename = entry[2:2+n].decode('ascii', errors='replace')
                        first, size = struct.unpack_from('>II', entry, 44)
                        path = prefix + filename
                        if entry[1] & 16:
                            walk(first, path + '/', depth+1)
                        else:
                            record = {'path': path, 'size': size, 'cluster': first}
                            part['files'].append(record)
                            selected = ('avatar' in path.lower() or path == 'system.manifest' or
                                        path.startswith('Content/0000000000000000/FFFE07DF/00008000/'))
                            if extract and selected and name == 'SystemAuxiliary':
                                target = (extract / name / path).resolve()
                                if not target.is_relative_to(extract.resolve()) or size > 128 * 1024 * 1024:
                                    raise ValueError('Invalid extraction target or oversized resource')
                                target.parent.mkdir(parents=True, exist_ok=True)
                                digest = hashlib.sha256()
                                remaining = size
                                with target.open('xb') as output:
                                    if remaining:
                                        for at in chain(first):
                                            chunk = read(at, min(cluster_size, remaining))
                                            output.write(chunk)
                                            digest.update(chunk)
                                            remaining -= len(chunk)
                                            if remaining == 0:
                                                break
                                    if remaining:
                                        raise ValueError('File shorter than directory size')
                                record['sha256'] = digest.hexdigest()
                                record['extracted'] = str(target)
            walk(root)
    return {'image': str(source), 'bytes': source.stat().st_size, 'partitions': results}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--extract-avatar-resources', type=Path,
                        help='New local directory for selected resources; never overwrites files')
    args = parser.parse_args()
    if args.output.resolve() == args.image.resolve():
        parser.error('Output must not be the input image')
    if args.extract_avatar_resources:
        destination = args.extract_avatar_resources.resolve()
        if not destination.is_relative_to(Path.cwd().resolve()) or destination.exists():
            parser.error('Extraction must use a NEW directory inside the current workspace')
    report = inventory(args.image, args.extract_avatar_resources)
    args.output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    for partition in report['partitions']:
        print(partition['name'], partition['magic'], len(partition['files']), 'files')
        for item in partition['files'][:60]:
            print(' ', item['path'], item['size'])
