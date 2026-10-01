#!/usr/bin/env bash
set -euo pipefail
source_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
[[ $# -eq 1 && -d "$1" ]] || { echo 'Uso: bash update-linux.sh /ruta/a/tu/Emulos360-anterior' >&2; exit 2; }
target_dir="$(cd -- "$1" && pwd -P)"
[[ "$source_dir" != "$target_dir" && -f "$source_dir/Emulos360" && -f "$target_dir/Emulos360" ]] || { echo 'Selecciona una instalación anterior de Emulos360 distinta de esta carpeta.' >&2; exit 1; }
if command -v fuser >/dev/null && fuser "$target_dir/Emulos360" "$target_dir/engine/Emulos360-core" >/dev/null 2>&1; then
  echo 'Cierra Emulos360 y el juego antes de actualizar.' >&2; exit 1
fi
# Explicit runtime paths: data (profiles, saves, covers and settings) is never copied.
for item in Emulos360 engine tools intro server assets licenses XEXplugins install-dependencies.sh install-base-dependencies.sh diagnose-linux.sh update-linux.sh LEEME-Linux.md LEEME-MultiP.md LEEME-Compatibilidad.md; do
  [[ ! -e "$source_dir/$item" ]] || cp -a -- "$source_dir/$item" "$target_dir/"
done
echo "Actualizada la aplicación en $target_dir. Tus datos siguen en $target_dir/data."
echo "Ahora ejecuta: bash \"$target_dir/install-dependencies.sh\""
