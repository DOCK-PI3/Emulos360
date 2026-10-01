#!/usr/bin/env bash
set -euo pipefail

root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$root/.tools"
command -v git >/dev/null || { echo 'Falta git.' >&2; exit 1; }

prepare_tagged() {
  local name="$1" url="$2" tag="$3" revision="$4" submodules="$5"
  local path="$root/.tools/$name"
  if [[ ! -d "$path/.git" ]]; then
    [[ ! -e "$path" ]] || { echo "Existe $path sin repositorio Git." >&2; exit 1; }
    git clone --depth 1 --branch "$tag" "$url" "$path"
  fi
  local current
  current="$(git -C "$path" rev-parse HEAD)"
  [[ "$current" == "$revision" ]] || {
    echo "$name debe estar en $revision; revisión actual: $current" >&2
    exit 1
  }
  if [[ "$submodules" == yes ]]; then
    git -C "$path" submodule update --init --recursive --depth 1
  fi
}

prepare_tagged libdatachannel https://github.com/paullouisageneau/libdatachannel.git \
  v0.24.5 443f6934d9007eb7076ab7825ba330f355fcbead yes
prepare_tagged mbedtls https://github.com/Mbed-TLS/mbedtls.git \
  v3.6.7 068ff080b369adfac81509f9b57b2afabaf82dc5 yes
prepare_tagged opus https://github.com/xiph/opus.git \
  v1.6.1 22244de5a79bd1d6d623c32e72bf1954b56235be no

iso="$root/.tools/iso2god-rs"
iso_revision=76207d7d7f89f551c7a5769560c0073e9ffe50e9
if [[ ! -d "$iso/.git" ]]; then
  [[ ! -e "$iso" ]] || { echo "Existe $iso sin repositorio Git." >&2; exit 1; }
  git init -q "$iso"
  git -C "$iso" remote add origin https://github.com/iliazeus/iso2god-rs.git
fi
if [[ "$(git -C "$iso" rev-parse HEAD 2>/dev/null || true)" != "$iso_revision" ]]; then
  git -C "$iso" fetch --depth 1 origin "$iso_revision"
  git -C "$iso" checkout --detach "$iso_revision"
fi
echo 'Fuentes Linux fijadas y listas.'
