#!/usr/bin/env bash
set -euo pipefail
package="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
if [[ ! -f "$package/Emulos360" && -f "$package/../out/Emulos360-MultiP-linux/Emulos360" ]]; then
  package="$(cd -- "$package/../out/Emulos360-MultiP-linux" && pwd)"
fi
mode="${1:-}"
case "$mode" in
  ''|--server-only|--check|--launch) ;;
  -h|--help) echo 'Uso: bash install-dependencies.sh [--check | --launch | --server-only]'; exit 0 ;;
  *) echo 'Opción desconocida. Usa --help.' >&2; exit 2 ;;
esac
[[ "$(uname -m)" == "x86_64" ]] || { echo "Esta build necesita Ubuntu x86_64." >&2; exit 1; }
if [[ "$mode" == "--check" ]]; then
  bash "$package/install-base-dependencies.sh" --check
  [[ -x "$package/server/node" && -x "$package/server/mongodb/bin/mongod" ]] || { echo 'Faltan dependencias del servidor MultiP.' >&2; exit 1; }
  "$package/server/node" --version
  "$package/server/mongodb/bin/mongod" --version
  exit 0
fi
if [[ "$mode" != "--server-only" ]]; then
  bash "$package/install-base-dependencies.sh"
fi
if ! dpkg-query -W -f='${Status}\n' ca-certificates curl xz-utils libssl3t64 libcurl4t64 2>/dev/null | awk '$0 != "install ok installed" {bad=1} END {if (NR != 5) bad=1; exit bad}'; then
  if [[ $EUID -eq 0 ]]; then root_command=(); else root_command=(sudo); fi
  "${root_command[@]}" apt-get update
  "${root_command[@]}" apt-get install -y ca-certificates curl xz-utils libssl3t64 libcurl4t64
fi
runtime="$package/server"
mkdir -p "$runtime"
for binary in "$runtime/node" "$runtime/mongodb/bin/mongod"; do
  if [[ -f "$binary" && ! -x "$binary" ]]; then
    chmod u+x "$binary" || { echo "No se pudo reparar el permiso de $binary." >&2; exit 1; }
    [[ -x "$binary" ]] || { echo "La unidad no permite ejecutar $binary. Copia el programa a tu carpeta personal (unidad sin noexec)." >&2; exit 1; }
  fi
done
download="$(mktemp -d)"
trap 'rm -rf -- "$download"' EXIT
node_version=24.15.0
node_archive="node-v$node_version-linux-x64.tar.xz"
if [[ ! -x "$runtime/node" ]]; then
  curl --fail --location --retry 3 "https://nodejs.org/dist/v$node_version/$node_archive" -o "$download/$node_archive"
  curl --fail --location "https://nodejs.org/dist/v$node_version/SHASUMS256.txt" -o "$download/checksums"
  (cd "$download" && awk -v archive="$node_archive" '$2 == archive {print}' checksums | sha256sum --check --strict)
  tar -xJf "$download/$node_archive" -C "$download"
  install -m 755 "$download/node-v$node_version-linux-x64/bin/node" "$runtime/node"
  mkdir -p "$package/licenses"
  cp "$download/node-v$node_version-linux-x64/LICENSE" "$package/licenses/Node-LICENSE.txt"
fi
if [[ ! -x "$runtime/mongodb/bin/mongod" ]]; then
  curl --fail --location --retry 3 'https://fastdl.mongodb.org/linux/mongodb-linux-x86_64-ubuntu2404-8.0.32.tgz' -o "$download/mongodb.tgz"
  echo 'b411be17c31ef249767ed91974d876e007c91afd5f45e1534057d247eada9f0d  mongodb.tgz' | (cd "$download" && sha256sum --check --strict)
  tar -xzf "$download/mongodb.tgz" -C "$download"
  mkdir -p "$runtime/mongodb/bin"
  install -m 755 "$download/mongodb-linux-x86_64-ubuntu2404-8.0.32/bin/mongod" "$runtime/mongodb/bin/mongod"
  mkdir -p "$package/licenses"
  for file in LICENSE-Community.txt THIRD-PARTY-NOTICES README; do
    if [[ -f "$download/mongodb-linux-x86_64-ubuntu2404-8.0.32/$file" ]]; then cp "$download/mongodb-linux-x86_64-ubuntu2404-8.0.32/$file" "$package/licenses/MongoDB-$file"; fi
  done
fi
"$runtime/node" --version
"$runtime/mongodb/bin/mongod" --version
echo 'Dependencias MultiP listas. Abre Emulos360 y entra en Multijugador.'
if [[ "$mode" == "--launch" ]]; then exec "$package/Emulos360"; fi
