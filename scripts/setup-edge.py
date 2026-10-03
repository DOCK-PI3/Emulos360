#!/usr/bin/env python3
"""Install the pinned official Linux Edge AppImage, with its license and hashes."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import urllib.request

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--destination', type=Path, required=True)
args = parser.parse_args()
metadata = json.loads((root / 'engine/edge-release.json').read_text(encoding='utf-8'))
destination = args.destination.resolve()
destination.mkdir(parents=True, exist_ok=True)

def install(url, digest, name):
    target = destination / name
    if target.is_file() and hashlib.sha256(target.read_bytes()).hexdigest() == digest:
        return target
    temporary = target.with_suffix(target.suffix + '.download')
    with urllib.request.urlopen(url, timeout=90) as response, temporary.open('wb') as output:
        shutil.copyfileobj(response, output)
    with temporary.open('rb') as source:
        if hashlib.file_digest(source, 'sha256').hexdigest() != digest:
            raise SystemExit(f'Hash incorrecto para {name}; no se instala.')
    temporary.replace(target)
    return target

image = install(metadata['linux']['url'], metadata['linux']['sha256'], 'xenia_edge_linux.AppImage')
image.chmod(0o755)
install(metadata['license']['url'], metadata['license']['sha256'], 'LICENSE')
shutil.copy2(root / 'engine/edge-release.json', destination / 'release.json')
print(f"Canary Edge {metadata['revision']} instalado en {destination}")
