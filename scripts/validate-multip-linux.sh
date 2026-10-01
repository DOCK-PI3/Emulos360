#!/usr/bin/env bash
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"
package="$root/out/Emulos360-MultiP-linux"
log="$root/.tools/multip-server"
EMULOS_PRIVATE_REAL_SERVER_DIR="$package/server" "$package/server/node" --test tests/private_server.test.cjs > "$log/linux-server-tests.txt" 2>&1
cmake -S tests/private-native -B out/Emulos360-MultiP/native-sanitizer -G Ninja -DCMAKE_BUILD_TYPE=Debug > "$log/linux-sanitizer-build.txt" 2>&1
cmake --build out/Emulos360-MultiP/native-sanitizer --parallel 2 >> "$log/linux-sanitizer-build.txt" 2>&1
EMULOS_PRIVATE_SERVER_DIR="$package/server" QT_QPA_PLATFORM=offscreen \
  out/Emulos360-MultiP/native-sanitizer/private_native_tests > "$log/linux-native-tests.txt" 2>&1
QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software "$package/Emulos360" \
  --data-dir "$log/linux-preview-data" --page multiplayer --capture "$log/linux-panel.png" > "$log/linux-capture.txt" 2>&1
echo 'Comprobaciones MultiP Linux completadas.'
