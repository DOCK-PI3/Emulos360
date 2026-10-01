#!/usr/bin/env bash
set -uo pipefail
package="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
[[ -f "$package/Emulos360" ]] || { echo 'Ejecuta el script incluido en la carpeta de la build Linux.' >&2; exit 1; }
report="$package/diagnostico-linux-$(date +%Y%m%d-%H%M%S).txt"
{
  echo 'Emulos360: diagnóstico Linux (sin perfiles, invitaciones ni partidas)'
  date -Is
  uname -a
  cat /etc/os-release
  printf '\nSesión: XDG_SESSION_TYPE=%s DISPLAY=%s WAYLAND_DISPLAY=%s GDK_BACKEND=%s\n' "${XDG_SESSION_TYPE:-}" "${DISPLAY:-}" "${WAYLAND_DISPLAY:-}" "${GDK_BACKEND:-}"
  getconf GNU_LIBC_VERSION
  lscpu
  free -h
  df -h "$package"
  command -v findmnt >/dev/null && findmnt -T "$package" -o TARGET,FSTYPE,OPTIONS
  for binary in Emulos360 engine/Emulos360-core tools/7z tools/iso2god server/node server/mongodb/bin/mongod; do
    [[ -f "$package/$binary" ]] || continue
    printf '\n%s\n' "$binary"
    ls -l "$package/$binary"
    ldd "$package/$binary"
  done
  printf '\nVulkan\n'
  if command -v vulkaninfo >/dev/null; then timeout 30s vulkaninfo --summary; else echo 'Falta vulkan-tools.'; fi
  printf '\nEventos recientes de memoria y fallos (si el sistema permite leerlos)\n'
  journalctl -k --since '-30 min' --no-pager 2>/dev/null | grep -Ei 'oom|out of memory|killed process|segfault|gpu|nvrm|amdgpu' | tail -60
} > "$report" 2>&1
printf 'Diagnóstico guardado en: %s\nAdjunta este archivo junto con data/core.log y data/core-console.log después de intentar abrir un juego.\n' "$report"
