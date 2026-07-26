#!/usr/bin/env bash
set -euo pipefail

root="$(git rev-parse --show-toplevel)"
hooks_dir="$(cd "$root" && git rev-parse --path-format=absolute --git-path hooks)"
src_dir="$root/scripts/hooks"

mkdir -p "$hooks_dir"

for src in "$src_dir"/*; do
    [ -f "$src" ] && [ -x "$src" ] || continue

    dest="$hooks_dir/$(basename "$src")"

    if [ -e "$dest" ] && [ ! -L "$dest" ]; then
        echo "refusing to overwrite existing hook: $dest" >&2
        echo "move it aside and re-run" >&2
        exit 1
    fi

    ln -sfn "$(realpath --relative-to="$hooks_dir" "$src")" "$dest"
    echo "installed $(basename "$dest")"
done
