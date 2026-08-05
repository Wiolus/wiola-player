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

## Commit names

```
area: what the commit does (#PR)
```

`area` is where the change lives, lower case, followed by a colon. The ones in use:

```
cmake  codec  engine  cli  audio  utils  lockfree  core
tests  scripts  docs  formatting  gitignore  ci  readme
```

Lower case after the colon, no full stop, and say what the commit does rather than what was
wrong. The pull request number closes the line.

```
codec: support seeking for format files while decoding (#43)
audio: add states enum for device (#60)
lockfree: implement clear method for SPSC ring-buffer (#47)
cmake: decrease CXX standard 26->23 (#51)
```

One logical change per commit, and every commit builds and passes the tests on its own. A bisect
that lands on a broken commit costs more than a large one ever saves.
