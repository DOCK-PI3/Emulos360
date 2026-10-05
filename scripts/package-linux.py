#!/usr/bin/env python3
"""Package the Linux runtime with executable modes and without personal data."""
import argparse
import hashlib
import pathlib
import tarfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('package', type=pathlib.Path)
args = parser.parse_args()
package = args.package.resolve()
if not (package / 'Emulos360').is_file():
    parser.error('La carpeta debe contener la build Linux de Emulos360.')
for required in ('engine/Emulos360-core', 'updates/apply-update-linux.py'):
    if not (package / required).is_file():
        parser.error(f'Falta en la build Linux: {required}')
for private in ('server/data', 'server/logs', 'server/.env', 'server/mongodb/data'):
    if (package / private).exists():
        parser.error(f'Revisa datos privados antes de empaquetar: {private}')
archive = package.with_name(package.name + '.tar.gz')
excluded = {'build', 'engine-build', 'iso2god-build', 'data'}
executables = {'Emulos360', 'engine/Emulos360-core', 'tools/7z', 'tools/iso2god',
               'server/node', 'server/mongodb/bin/mongod', }


def runtime(info):
    relative = pathlib.PurePosixPath(info.name).relative_to(package.name)
    if relative.parts[:2] in (('engine', 'edge'), ('engine', 'xblig')) or relative.name in {'LEEME-XBLIG.md', 'LEEME-DUAL-CORE.md', 'LEEME-CORES.md'}:
        return None
    if relative.parts and relative.parts[0] in excluded:
        return None
    info.uid = info.gid = 0
    info.uname = info.gname = ''
    if info.isdir() or relative.as_posix() in executables or relative.suffix == '.sh':
        info.mode = 0o755
    elif info.isfile():
        info.mode = 0o644
    return info


with tarfile.open(archive, 'w:gz', compresslevel=6) as output:
    output.add(package, arcname=package.name, filter=runtime)
with archive.open('rb') as source:
    digest = hashlib.file_digest(source, 'sha256').hexdigest()
archive.with_name(archive.name + '.sha256').write_text(
    f'{digest}  {archive.name}\n', encoding='utf-8')
print(f'{archive}: {archive.stat().st_size / 1048576:.1f} MiB, SHA256 {digest}')
