#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Uso: bash install-dependencies.sh [--check | --build | --launch]

Sin opciones instala las dependencias para ejecutar Emulos360 en Ubuntu 26.04.
  --check   Comprueba la instalación sin instalar paquetes.
  --build   Instala también las herramientas necesarias para compilar.
  --launch  Instala, comprueba y abre Emulos360 en esta sesión gráfica.
EOF
}

fail() {
  printf 'Error: %s\n' "$*" >&2
  exit 1
}

mode=install
if [[ $# -gt 1 ]]; then
  usage >&2
  exit 2
fi
case "${1:-}" in
  '') ;;
  --check) mode=check ;;
  --build) mode=build ;;
  --launch) mode=launch ;;
  -h|--help) usage; exit 0 ;;
  *) usage >&2; exit 2 ;;
esac

[[ -r /etc/os-release ]] || fail 'No se puede identificar la distribución Linux.'
# shellcheck disable=SC1091
source /etc/os-release
[[ "${ID:-}" == ubuntu ]] || fail 'Este instalador está preparado para Ubuntu.'
command -v dpkg >/dev/null || fail 'No se encontró dpkg.'
[[ "$(dpkg --print-architecture)" == amd64 ]] || fail 'Esta build requiere Ubuntu x86-64 (amd64).'

glibc_version="$(getconf GNU_LIBC_VERSION 2>/dev/null || true)"
glibc_version="${glibc_version##* }"
[[ -n "$glibc_version" ]] || fail 'No se pudo comprobar la versión de glibc.'
if dpkg --compare-versions "$glibc_version" lt 2.43; then
  fail "La build actual necesita glibc 2.43 y este Ubuntu tiene $glibc_version. Se compiló en Ubuntu 26.04; instalar bibliotecas con APT no puede actualizar glibc de forma segura."
fi

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
package_dir=''
if [[ -f "$script_dir/Emulos360" ]]; then
  package_dir="$script_dir"
elif [[ -f "$script_dir/../out/Emulos360-linux/Emulos360" ]]; then
  package_dir="$(cd -- "$script_dir/../out/Emulos360-linux" && pwd -P)"
elif [[ -f "$script_dir/../out/Emulos360-MultiP-linux/Emulos360" ]]; then
  package_dir="$(cd -- "$script_dir/../out/Emulos360-MultiP-linux" && pwd -P)"
fi

runtime_packages=(
  ca-certificates fonts-dejavu-core xdg-utils 7zip xwayland
  libqt6core6t64 libqt6svg6 qt6-svg-plugins qt6-qpa-plugins qt6-wayland
  qml6-module-qtquick qml6-module-qtquick-window
  qml6-module-qtquick-controls qml6-module-qtquick-layouts
  qml6-module-qtquick-dialogs qml6-module-qtquick3d
  qml6-module-qtqml-workerscript qml6-module-qtmultimedia
  libgl1 libegl1 libgl1-mesa-dri
  libxcb-cursor0 libxcb-xinerama0 libxkbcommon-x11-0
  libgtk-3-0t64 libsdl2-2.0-0 libcurl4t64 liblz4-1
  libx11-xcb1 libasound2t64 libpulse0
  libvulkan1 mesa-vulkan-drivers vulkan-tools
)

build_packages=(
  build-essential cmake ninja-build clang lld git python3 pkg-config
  rustc cargo 7zip
  qt6-base-dev qt6-declarative-dev qt6-quick3d-dev
  qt6-multimedia-dev qt6-shadertools-dev
  libgtk-3-dev libx11-xcb-dev libcurl4-openssl-dev liblz4-dev
  libvulkan-dev libsdl2-dev libc++-dev libc++abi-dev
  spirv-tools glslang-tools
)

if [[ "$mode" != check ]]; then
  if (( EUID == 0 )); then
    apt=(env DEBIAN_FRONTEND=noninteractive apt-get)
  else
    command -v sudo >/dev/null || fail 'Se necesita sudo para instalar paquetes.'
    sudo -v
    apt=(sudo env DEBIAN_FRONTEND=noninteractive apt-get)
  fi

  "${apt[@]}" update
  if ! apt-cache show qml6-module-qtquick3d >/dev/null 2>&1; then
    "${apt[@]}" install -y software-properties-common
    sudo_prefix=()
    (( EUID == 0 )) || sudo_prefix=(sudo)
    "${sudo_prefix[@]}" add-apt-repository -y universe
    "${apt[@]}" update
  fi
  apt-cache show qml6-module-qtquick3d >/dev/null 2>&1 || \
    fail 'No se encontraron los módulos Qt 6 de Ubuntu. Comprueba los repositorios de Ubuntu 26.04.'

  "${apt[@]}" install -y --no-install-recommends "${runtime_packages[@]}"
  if [[ "$mode" == build ]]; then
    "${apt[@]}" install -y --no-install-recommends "${build_packages[@]}"
  fi
fi

missing_packages=()
for package in "${runtime_packages[@]}"; do
  status="$(dpkg-query -W -f='${Status}' "$package" 2>/dev/null || true)"
  [[ "$status" == 'install ok installed' ]] || missing_packages+=("$package")
done
if (( ${#missing_packages[@]} )); then
  fail "Faltan paquetes: ${missing_packages[*]}. Ejecuta este script sin --check."
fi

qt_version="$(dpkg-query -W -f='${Version}' libqt6core6t64 2>/dev/null || true)"
[[ -n "$qt_version" ]] || fail 'Falta Qt 6. Ejecuta este script sin --check.'
dpkg --compare-versions "$qt_version" ge 6.10.2 || \
  fail "La build actual necesita Qt 6.10.2 o posterior; se encontró $qt_version."

qml_packages=(
  qml6-module-qtquick qml6-module-qtquick-controls
  qml6-module-qtquick-layouts qml6-module-qtquick-dialogs
  qml6-module-qtquick3d qml6-module-qtmultimedia
)
for package in "${qml_packages[@]}"; do
  status="$(dpkg-query -W -f='${Status}' "$package" 2>/dev/null || true)"
  [[ "$status" == 'install ok installed' ]] || fail "Falta el módulo $package. Ejecuta este script sin --check."
done

if [[ -n "$package_dir" ]]; then
  binaries=("$package_dir/Emulos360" "$package_dir/engine/Emulos360-core"
            "$package_dir/tools/7z" "$package_dir/tools/iso2god")
  for optional in server/node server/mongodb/bin/mongod; do
    [[ ! -f "$package_dir/$optional" ]] || binaries+=("$package_dir/$optional")
  done
  for binary in "${binaries[@]}"; do
    [[ -f "$binary" ]] || fail "El paquete está incompleto: falta $binary. Extrae la build Linux completa."
    if [[ ! -x "$binary" ]]; then
      [[ "$mode" != check ]] || fail "Falta permiso de ejecución en $binary. Ejecuta este script sin --check."
      chmod u+x "$binary" || fail "No se pudo dar permiso de ejecución a $binary."
    fi
    [[ -x "$binary" ]] || fail "No se puede ejecutar $binary. Comprueba si la unidad está montada con noexec."
    if ! linkage="$(ldd "$binary" 2>&1)"; then
      printf '%s\n' "$linkage" >&2
      fail "No se pudieron comprobar las bibliotecas de $binary."
    fi
    if grep -Eq 'not found|version .* not found' <<<"$linkage"; then
      printf '%s\n' "$linkage" >&2
      fail "Faltan bibliotecas para $binary."
    fi
  done
  "$package_dir/tools/7z" i >/dev/null || fail 'No se puede ejecutar 7-Zip. Comprueba los permisos y si la unidad está montada con noexec.'
  "$package_dir/tools/iso2god" --version || fail 'No se puede ejecutar ISO2GOD.'
  printf 'Permisos y herramientas de importación comprobados.\n'
else
  printf 'Aviso: no se encontró Emulos360 para comprobar el ejecutable.\n' >&2
fi

printf 'Dependencias listas: Ubuntu %s, glibc %s, Qt %s.\n' "${VERSION_ID:-?}" "$glibc_version" "$qt_version"
if [[ -z "${DISPLAY:-}" && -z "${WAYLAND_DISPLAY:-}" ]]; then
  printf 'Aviso: no se detecta una sesión gráfica (DISPLAY o WAYLAND_DISPLAY).\n' >&2
fi
if [[ -n "$package_dir" ]]; then
  printf 'Para abrir: %q\n' "$package_dir/Emulos360"
fi
if [[ "$mode" == build ]]; then
  printf 'Herramientas de compilación instaladas. Ejecuta bash scripts/build-linux.sh desde el proyecto.\n'
fi
if [[ "$mode" == launch ]]; then
  [[ -n "$package_dir" ]] || fail 'No se encontró la build Linux para abrirla.'
  (( EUID != 0 )) || fail 'Abre la aplicación como usuario normal, sin sudo.'
  [[ -n "${DISPLAY:-}" || -n "${WAYLAND_DISPLAY:-}" ]] || \
    fail 'No hay una sesión gráfica activa para abrir la aplicación.'
  exec "$package_dir/Emulos360"
fi
