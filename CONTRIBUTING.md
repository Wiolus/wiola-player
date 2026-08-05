# Contributing

## Formatting

Only the scripts. Never `clang-format` or `gersemi` by hand - they format what the scripts choose,
which is every tracked file, not whatever happened to be edited.

```bash
pip install -r requirements.txt        # the pinned clang-format and gersemi

scripts/format-cpp.sh                  # rewrite every tracked C/C++ file
scripts/format-cpp.sh --check          # report instead, and fail if anything differs

scripts/format-cmake.sh                # rewrite every tracked CMake file
scripts/format-cmake.sh --check        # report instead, and fail if anything differs

scripts/install-hooks.sh               # run both checks before every commit, from now on
```

`format-cmake.sh` hands its argument to gersemi, so anything gersemi takes works there -
`--diff`, for one. `format-cpp.sh` knows only `--check`; any other argument rewrites.

`.clang-format` and `.gersemirc` decide everything. Do not argue with them in review, and do not
restate their settings anywhere else.
