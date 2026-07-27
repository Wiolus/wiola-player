#!/usr/bin/env bash
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

if ! command -v clang-format >/dev/null 2>&1; then
    echo "clang-format not found; install with: pip install -r requirements.txt" >&2
    exit 127
fi

mapfile -d '' -t files < <(
    git ls-files -z -- \
        '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx' '*.ipp'
)

if [ ${#files[@]} -eq 0 ]; then
    exit 0
fi

case "${1-}" in
    --check) exec clang-format --dry-run --Werror "${files[@]}" ;;
    *) exec clang-format -i "${files[@]}" ;;
esac
