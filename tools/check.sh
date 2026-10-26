#!/usr/bin/env bash
set -euo pipefail
if (( $# > 2 )); then printf 'Usage: bash tools/check.sh [release|debug|asan-ubsan|ubsan|tsan] [build-dir]\n' >&2; exit 2; fi
profile=${1:-release}
configuration=Debug
sanitizer=none
case "$profile" in
    release) configuration=Release ;;
    debug) ;;
    asan-ubsan|ubsan|tsan) sanitizer=$profile ;;
    *) printf 'Unknown profile: %s\n' "$profile" >&2; exit 2 ;;
esac
for tool in cmake ctest "${CXX:-c++}"; do
    if ! command -v "$tool" >/dev/null 2>&1; then printf 'Missing prerequisite: %s\n' "$tool" >&2; exit 127; fi
done
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd -- "$root"
build=${2:-build-$profile}
cmake -S . -B "$build" -DCMAKE_BUILD_TYPE="$configuration" \
    -DVITALWATCH_WARNINGS_AS_ERRORS=ON -DVITALWATCH_SANITIZER="$sanitizer"
cmake --build "$build" --config "$configuration" --parallel 2
# Fail-fast instrumentation; a runtime startup failure remains a failed run.
export ASAN_OPTIONS=${ASAN_OPTIONS:-halt_on_error=1:detect_leaks=1}
export UBSAN_OPTIONS=${UBSAN_OPTIONS:-halt_on_error=1:print_stacktrace=1}
export TSAN_OPTIONS=${TSAN_OPTIONS:-halt_on_error=1}
(cd -- "$build" && ctest -C "$configuration" --output-on-failure --timeout 10)
