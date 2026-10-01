#!/usr/bin/env bash
set -euo pipefail

root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
engine="$root/engine/xenia"
patch="$root/patches/xenia-netplay/0001-emulos360.patch"
base=6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c

[[ -f "$engine/xenia-build.py" ]] || {
  echo 'Falta el submódulo Xenia. Ejecuta git submodule update --init --recursive.' >&2
  exit 1
}
head="$(git -C "$engine" rev-parse HEAD)"
[[ "$head" == "$base" ]] || {
  echo "Xenia debe estar en la revisión $base; revisión actual: $head" >&2
  exit 1
}

if git -C "$engine" apply --reverse --check "$patch" 2>/dev/null; then
  echo 'Parche Emulos360 ya aplicado al motor.'
  exit 0
fi
git -C "$engine" apply --check "$patch" || {
  echo 'El parche no se puede aplicar. Revisa los cambios locales del submódulo.' >&2
  exit 1
}
git -C "$engine" apply "$patch"
echo 'Parche Emulos360 aplicado al motor.'
