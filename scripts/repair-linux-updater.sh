#!/usr/bin/env bash
# Install only the corrected update helper, including into a 0.3.0 installation.
set -euo pipefail
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
helper="$script_dir/../updates/apply-update-linux.py"
[[ -f "$helper" ]] || helper="$script_dir/updates/apply-update-linux.py"
[[ $# -eq 1 && -d "$1" ]] || { echo 'Uso: bash scripts/repair-linux-updater.sh /ruta/a/Emulos360' >&2; exit 2; }
target="$(cd -- "$1" && pwd -P)"
[[ -f "$helper" && -f "$target/Emulos360" && -f "$target/engine/Emulos360-core" && -d "$target/updates" ]] || {
  echo 'Selecciona una instalación Linux completa de Emulos360.' >&2; exit 1;
}
[[ ! -L "$target/updates" && ! -L "$target/updates/apply-update-linux.py" && -f "$target/updates/apply-update-linux.py" ]] || {
  echo 'La carpeta updates o su instalador no son archivos normales.' >&2; exit 1;
}
if command -v fuser >/dev/null && fuser "$target/Emulos360" "$target/engine/Emulos360-core" >/dev/null 2>&1; then
  echo 'Cierra Emulos360 y el juego antes de reparar el actualizador.' >&2; exit 1
fi
python3 -c 'import tarfile; assert hasattr(tarfile, "data_filter"), "Actualiza Python 3: falta el filtro seguro de extracción."'
python3 -c 'import ast, pathlib, sys; ast.parse(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))' "$helper"
if cmp -s -- "$helper" "$target/updates/apply-update-linux.py"; then
  echo 'El instalador corregido ya está instalado.'
  exit 0
fi
backup="$target/updates/apply-update-linux.before-linux-fix.py"
if [[ ! -e "$backup" && ! -L "$backup" ]]; then
  cp -p -- "$target/updates/apply-update-linux.py" "$backup"
fi
staged="$(mktemp "$target/updates/.apply-update-linux.XXXXXX")"
trap 'rm -f -- "$staged"' EXIT
cp -- "$helper" "$staged"
chmod 644 "$staged"
mv -f -- "$staged" "$target/updates/apply-update-linux.py"
printf 'Instalador corregido en: %s\n' "$target/updates/apply-update-linux.py"
printf 'Copia del instalador anterior: %s\n' "$backup"
printf 'Abre Emulos360 y repite la actualización.\n'
printf 'Si se cierra la nueva versión, comparte data/update-last-error.txt y data/update-startup.log.\n'
