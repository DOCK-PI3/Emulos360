#!/usr/bin/env bash
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
engine="$root/engine/xenia"
base=6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c
[[ -f "$engine/xenia-build.py" ]] || { echo 'Falta el submódulo Xenia.' >&2; exit 1; }
head="$(git -C "$engine" rev-parse HEAD)"
[[ "$head" == "$base" ]] || { echo "Xenia debe estar en $base; revisión actual: $head" >&2; exit 1; }
for name in 0001-emulos360.patch 0002-core-maintenance-0.6.0.patch; do
  patch="$root/patches/xenia-netplay/$name"
  if git -C "$engine" apply --reverse --check "$patch" 2>/dev/null; then
    echo "$name ya aplicado al motor."
    continue
  fi
  git -C "$engine" apply --check "$patch" || { echo "No se puede aplicar $name. Revisa cambios locales." >&2; exit 1; }
  git -C "$engine" apply "$patch"
  echo "$name aplicado al motor."
done
