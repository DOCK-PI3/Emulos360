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
archive = package.with_name(package.name + '.tar.gz')
excluded = {'build', 'engine-build', 'iso2god-build', 'data'}
executables = {'Emulos360', 'engine/Emulos360-core', 'tools/7z', 'tools/iso2god',
               'server/node', 'server/mongodb/bin/mongod'}


def runtime(info):
    relative = pathlib.PurePosixPath(info.name).relative_to(package.name)
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
