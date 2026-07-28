#!/usr/bin/env bash
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

if ! command -v gersemi >/dev/null 2>&1; then
    echo "gersemi not found; install with: pip install -r requirements.txt" >&2
    exit 127
fi

mapfile -d '' -t tracked < <(
    git ls-files -z -- \
        '*CMakeLists.txt' '*CMakeLists.txt.in' '*.cmake' '*.cmake.in'
)

# git lists index entries, so a file deleted in the working tree is still reported.
files=()
for file in "${tracked[@]}"; do
    if [ -f "$file" ]; then
        files+=("$file")
    fi
done

if [ ${#files[@]} -eq 0 ]; then
    exit 0
fi

exec gersemi "${1:--i}" "${files[@]}"
