#!/usr/bin/env bash
set -euo pipefail
taskroot=$(cd "$(dirname "$0")/../.." && pwd)
taskbuild=${CORE_TEST_BUILD_DIR:-/var/tmp/emulos360-core-extract-20261006}
mkdir -p "$taskbuild"
for variant in baseline candidate; do
  valuesource="$taskroot/engine/xenia/src/xenia/cpu/hir/value.cc"
  if [[ "$variant" == candidate ]]; then
    valuesource="$taskroot/local/core-maintenance-20261006/candidate/src/xenia/cpu/hir/value.cc"
  fi
  if ! cmake -S "$taskroot/tests/core-semantics" -B "$taskbuild/$variant" \
      -G Ninja -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCORE_SEMANTICS_SANITIZERS=ON \
      -DCORE_VALUE_SOURCE="$valuesource" > "$taskbuild/$variant-build.log" 2>&1; then
    tail -n 35 "$taskbuild/$variant-build.log"
    exit 2
  fi
  if ! cmake --build "$taskbuild/$variant" >> "$taskbuild/$variant-build.log" 2>&1; then
    tail -n 45 "$taskbuild/$variant-build.log"
    exit 2
  fi
done
export ASAN_OPTIONS=detect_leaks=1:halt_on_error=1
export UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
baseline_status=0
"$taskbuild/baseline/core_semantics_tests" --lanes \
  > "$taskbuild/baseline-lanes.log" 2>&1 || baseline_status=$?
cat "$taskbuild/baseline-lanes.log"
printf 'baseline_exit=%s\n' "$baseline_status"
if [[ "$baseline_status" != 1 ]]; then
  echo 'Expected the unpatched baseline to reproduce the lane error.' >&2
  exit 2
fi
bounds_status=0
"$taskbuild/baseline/core_semantics_tests" --all-indices \
  > "$taskbuild/baseline-bounds.log" 2>&1 || bounds_status=$?
head -n 10 "$taskbuild/baseline-bounds.log"
printf 'baseline_bounds_exit=%s\n' "$bounds_status"
if [[ "$bounds_status" == 0 ]]; then
  echo 'Expected a baseline bounds failure.' >&2
  exit 2
fi
ctest --test-dir "$taskbuild/candidate" --output-on-failure \
  | tee "$taskbuild/candidate-tests.log"
"$taskbuild/candidate/core_semantics_tests" --lanes \
  | tee "$taskbuild/candidate-lanes.log"
"$taskbuild/candidate/core_semantics_tests" --all-indices \
  | tee "$taskbuild/candidate-all-indices.log"
