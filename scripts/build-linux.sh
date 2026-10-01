#!/usr/bin/env bash
set -euo pipefail

root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"
bash "$root/scripts/prepare-engine.sh"
bash "$root/scripts/setup-linux-sources.sh"
package="${EMULOS_LINUX_OUTPUT:-$root/out/Emulos360-linux}"
frontend_build="${EMULOS_LINUX_FRONTEND_BUILD:-$package/build}"
engine_build="${EMULOS_LINUX_ENGINE_BUILD:-$package/engine-build}"
jobs="${EMULOS_BUILD_JOBS:-4}"

if ! command -v cargo >/dev/null && [[ -x "$HOME/.cargo/bin/cargo" ]]; then
  export PATH="$HOME/.cargo/bin:$PATH"
fi

if [[ $# -gt 1 || ( $# -eq 1 && "$1" != "--frontend-only" ) ]]; then
  echo "Uso: bash scripts/build-linux.sh [--frontend-only]" >&2
  exit 2
fi

for tool in cmake ninja cargo; do
  command -v "$tool" >/dev/null || { echo "Falta $tool." >&2; exit 1; }
done
for source in libdatachannel mbedtls opus; do
  [[ -f ".tools/$source/CMakeLists.txt" ]] || {
    echo "Falta .tools/$source. Prepara las fuentes de voz fijadas antes de compilar." >&2
    exit 1
  }
done
[[ -f .tools/iso2god-rs/Cargo.toml ]] || {
  echo "Falta .tools/iso2god-rs para la importación de ISO." >&2
  exit 1
}

cmake -S "$root" -B "$frontend_build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
  -DEMULOS_SERVICE_TESTS=OFF -DEMULOS_NETPLAY_TESTS=OFF \
  -DEMULOS_AVATAR_TESTS=OFF
cmake --build "$frontend_build" --target los360 --parallel "$jobs"

mkdir -p "$package/intro" "$package/tools" "$package/licenses" \
  "$package/data" "$package/XEXplugins/MetroDashboard" "$package/updates"
install -m 755 "$frontend_build/Emulos360" "$package/Emulos360"
install -m 644 updates/apply-update-linux.py "$package/updates/apply-update-linux.py"
install -m 755 scripts/install-ubuntu-deps.sh "$package/install-dependencies.sh"
cp docs/GAME_COMPATIBILITY.md "$package/LEEME-Compatibilidad.md"
install -m 755 scripts/diagnose-linux.sh "$package/diagnose-linux.sh"
install -m 755 scripts/update-linux.sh "$package/update-linux.sh"
cp docs/LINUX.md "$package/LEEME-Linux.md"
cp assets/intro/aurora.mp4 assets/intro/aurora.png \
  assets/intro/nova.mp4 assets/intro/nova.png "$package/intro/"
cp XEXplugins/MetroDashboard/package/default.xex \
  XEXplugins/MetroDashboard/package/font.png \
  "$package/XEXplugins/MetroDashboard/"

# Original resources already imported by the owner; never copy profile data.
avatar_source="${EMULOS_AVATAR_RESOURCES:-$root/out/Emulos360/data/avatar-system}"
if [[ -f "$avatar_source/AvatarAssetPack.toc" && -f "$avatar_source/avatar-skeleton.bin" ]]; then
  mkdir -p "$package/assets/avatar-system"
  install -m 644 "$avatar_source/AvatarAssetPack.toc" "$avatar_source/avatar-skeleton.bin" "$package/assets/avatar-system/"
fi

CARGO_TARGET_DIR="$package/iso2god-build" \
  cargo build --manifest-path .tools/iso2god-rs/Cargo.toml \
  --release --locked --bin iso2god --jobs "$jobs"
install -m 755 "$package/iso2god-build/release/iso2god" \
  "$package/tools/iso2god"
cp .tools/iso2god-rs/LICENSE "$package/licenses/iso2god-LICENSE.txt"

if [[ -x /usr/lib/7zip/7za ]]; then
  seven_zip=/usr/lib/7zip/7za
elif command -v 7za >/dev/null; then
  seven_zip="$(command -v 7za)"
elif command -v 7zz >/dev/null; then
  seven_zip="$(command -v 7zz)"
else
  echo "Falta 7za o 7zz para importar archivos .7z." >&2
  exit 1
fi
install -m 755 "$seven_zip" "$package/tools/7z"
if [[ -f /usr/share/doc/7zip/copyright ]]; then
  cp /usr/share/doc/7zip/copyright "$package/licenses/7zip-copyright"
fi

cp .tools/libdatachannel/LICENSE "$package/licenses/libdatachannel-LICENSE.txt"
cp .tools/libdatachannel/deps/libjuice/LICENSE "$package/licenses/libjuice-LICENSE.txt"
cp .tools/libdatachannel/deps/usrsctp/LICENSE.md "$package/licenses/usrsctp-LICENSE.md"
cp .tools/libdatachannel/deps/plog/LICENSE "$package/licenses/plog-LICENSE.txt"
cp .tools/mbedtls/LICENSE "$package/licenses/mbedtls-LICENSE.txt"
cp .tools/opus/COPYING "$package/licenses/opus-COPYING.txt"

if [[ "${1:-}" != "--frontend-only" ]]; then
  cmake -S "$root/engine/xenia" -B "$engine_build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ -DXENIA_BUILD_TESTS=OFF \
    -DXENIA_BUILD_MISC=OFF
  cmake --build "$engine_build" --target xenia-app --parallel "$jobs"
  mkdir -p "$package/engine"
  install -m 755 "$engine_build/bin/Linux/Emulos360-core" \
    "$package/engine/Emulos360-core"
fi
if [[ -f "$root/engine/xenia/third_party/gamecontrollerdb/gamecontrollerdb.txt" ]]; then
  install -m 644 "$root/engine/xenia/third_party/gamecontrollerdb/gamecontrollerdb.txt" "$package/engine/gamecontrollerdb.txt"
fi

echo "Build Linux preparada en $package"
