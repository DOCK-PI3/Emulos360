#!/usr/bin/env bash
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
package="$root/out/Emulos360-MultiP-linux"
source="$root/out/Emulos360-MultiP/server"
[[ -f "$source/upstream/dist/main.js" ]] || { echo 'Prepara primero el servidor con los scripts de Windows.' >&2; exit 1; }
[[ $# -le 1 && ( $# -eq 0 || "$1" == --frontend-only ) ]] || { echo 'Uso: bash scripts/build-multip-linux.sh [--frontend-only]' >&2; exit 2; }
EMULOS_LINUX_OUTPUT="$package" \
EMULOS_LINUX_FRONTEND_BUILD="$root/out/Emulos360-MultiP/linux-build" \
EMULOS_LINUX_ENGINE_BUILD="$root/out/Emulos360-MultiP/linux-engine-build" \
  bash "$root/scripts/build-linux.sh" "$@"
mkdir -p "$package/server"
for item in private-service.cjs panel.html panel.js package.json package-lock.json node_modules upstream; do
  cp -a "$source/$item" "$package/server/"
done
cp "$root/docs/PRIVATE_NETPLAY.md" "$package/LEEME-MultiP.md"
install -m 755 "$root/scripts/install-ubuntu-deps.sh" "$package/install-base-dependencies.sh"
install -m 755 "$root/scripts/install-multip-deps.sh" "$package/install-dependencies.sh"
bash "$package/install-dependencies.sh" --server-only
python3 "$root/scripts/package-linux.py" "$package"
echo "Build MultiP Linux preparada en $package"
