#!/usr/bin/env bash
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

preset=linux-coverage
cxx=${CXX:-clang++}

# Where that preset builds. The profile and the report are written beside the binaries they were
# taken from, so this script needs the path as well.
build_dir=build-coverage

# What the report covers, named rather than filtered. A header-only dependency is compiled into
# our own translation units and instrumented with them, wherever it was found; naming what is
# ours keeps everything else out.
sources=(include src)

if ! "$cxx" --version 2>/dev/null | grep -qi clang; then
    echo "$cxx is not clang; the instrumentation flags are LLVM's. Set CXX." >&2
    exit 127
fi

# A profile is read by the llvm tools of the compiler's own version, so they are looked up by it.
version=$("$cxx" -dumpversion | cut -d. -f1)

for tool in llvm-profdata llvm-cov; do
    if command -v "$tool-$version" >/dev/null 2>&1; then
        declare "${tool//-/_}=$tool-$version"
    elif command -v "$tool" >/dev/null 2>&1; then
        declare "${tool//-/_}=$tool"
    else
        echo "$tool not found; install with: sudo apt install llvm-$version" >&2
        exit 127
    fi
done

# The compiler is not named by the preset, so it is handed over the way cmake takes it.
export CXX="$cxx"

cmake --preset "$preset"
cmake --build --preset "$preset" -j

# One profile per process, and a test binary is run once per case. Old ones are dropped: they were
# written by a build that no longer exists.
rm -rf "$build_dir/coverage"
mkdir -p "$build_dir/coverage"

LLVM_PROFILE_FILE="$PWD/$build_dir/coverage/%p.profraw" ctest --preset "$preset"

"$llvm_profdata" merge -sparse \
    "$build_dir"/coverage/*.profraw \
    -o "$build_dir/coverage/wiola.profdata"

# The player is measured beside the tests, since it carries the code no test links yet.
mapfile -t objects < <(
    find "$build_dir/tests" -type f -name '*_test' -perm -u+x -printf '-object\n%p\n'
)

report=("$build_dir/bin/wiola-player" "${objects[@]}"
    "-instr-profile=$build_dir/coverage/wiola.profdata")

"$llvm_cov" report "${report[@]}" "${sources[@]}" |
    tee "$build_dir/coverage/summary.txt"

"$llvm_cov" show "${report[@]}" \
    -format=html -output-dir="$build_dir/coverage/html" \
    "${sources[@]}"

# The same measurement in the format the tools that compare it against a diff read.
"$llvm_cov" export "${report[@]}" -format=lcov "${sources[@]}" \
    > "$build_dir/coverage/coverage.lcov"

# The same measurement without the per-line detail, for whatever reads a number, not a report.
"$llvm_cov" export "${report[@]}" -format=text -summary-only "${sources[@]}" \
    > "$build_dir/coverage/summary.json"

echo "html report: $build_dir/coverage/html/index.html"
