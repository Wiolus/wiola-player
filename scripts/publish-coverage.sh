#!/usr/bin/env bash
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

# One tag for every run, so a link to the report does not have to name the run that produced it.
tag=coverage

coverage_dir=build-coverage/coverage

for tool in gh zip; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "$tool not found; install with: sudo apt install $tool" >&2
        exit 127
    fi
done

if [ ! -d "$coverage_dir/html" ] || [ ! -f "$coverage_dir/summary.json" ]; then
    echo "no report in $coverage_dir; run scripts/coverage.sh first" >&2
    exit 1
fi

# An existing archive would be added to rather than replaced, keeping files the report dropped.
archive="$PWD/$coverage_dir/coverage-html.zip"
rm -f "$archive"

# Entered first, so that the paths inside start at the report and not at the build directory.
(cd "$coverage_dir/html" && zip -qr "$archive" .)

# What the badge shows: the line total, in the shape shields.io reads.
python3 - "$coverage_dir/summary.json" > "$coverage_dir/badge.json" <<'PY'
import json
import sys

totals = json.load(open(sys.argv[1]))["data"][0]["totals"]
percent = totals["lines"]["percent"]
color = "brightgreen" if percent >= 90 else "yellow" if percent >= 75 else "red"

print(json.dumps({"schemaVersion": 1, "label": "coverage",
                  "message": f"{percent:.1f}%", "color": color}))
PY

# A prerelease, so that this is never the release "latest" resolves to.
gh release view "$tag" >/dev/null 2>&1 ||
    gh release create "$tag" --prerelease --target "$(git rev-parse HEAD)" \
        --title Coverage --notes "The report from the tip of main."

gh release upload "$tag" "$archive" "$coverage_dir/badge.json" --clobber

echo "release: $(gh release view "$tag" --json url --jq .url)"
