# Contributing

## Building

| | Minimum | Where it comes from |
|---|---|---|
| CMake | 3.28 | `cmake` |
| Compiler | GCC 13 or Clang 17 | `g++`, C++23 |
| Qt | 6, Widgets | `qt6-base-dev` |

Qt is the only dependency that has to be installed, and no build works without it. dr_libs,
miniaudio, CLI11 and GoogleTest are fetched during configure, pinned in `cmake/dependencies.cmake`.

### Linux

```bash
sudo apt install cmake g++ qt6-base-dev

cmake --preset linux                   # configure into build/
cmake --build --preset linux           # build/bin/wiola-player
ctest --preset linux                   # run the tests
```

Builds are `RelWithDebInfo` unless a type is named. That is deliberate rather than a default: the
decoder feeds a device callback that has to answer within a period, and unoptimized it runs several
times slower than the deadline allows.

`-DENABLE_UNIT_TESTS=OFF` skips the tests and GoogleTest with them. In-source builds are refused,
so configure from the repository root as above.

### Windows

Cross-compiled from Linux. The result is one executable that needs no DLLs beside it.

```bash
sudo apt install g++-mingw-w64-x86-64-posix
```

The build also needs a Qt built for MinGW-w64, which no distribution packages. Build one once with
`scripts/build-qt-mingw.sh`, described in [scripts/README.md](scripts/README.md); it compiles Qt
from source, so expect it to take a while and to want several gigabytes. `QT_MINGW_ROOT` then
points the toolchain file at it and has to be set in the shell that configures:

```bash
scripts/build-qt-mingw.sh ~/opt/qt6-mingw-static
export QT_MINGW_ROOT=~/opt/qt6-mingw-static

cmake --preset windows                 # configure into build-win/
cmake --build --preset windows         # build-win/bin/wiola-player.exe
```

The tests are off in this preset: they are built for Windows and the host cannot run them. Run
them under the linux preset instead.

To confirm the executable stands alone, list what it imports. Every name should be a Windows
system library:

```bash
x86_64-w64-mingw32-objdump -p build-win/bin/wiola-player.exe | grep "DLL Name"
```

### Releases

What a published binary is built from, one preset per platform:

```bash
cmake --preset linux-release            # configure into build-release/
cmake --build --preset linux-release    # build-release/bin/wiola-player

cmake --preset windows-release          # configure into build-win-release/
cmake --build --preset windows-release  # build-win-release/bin/wiola-player.exe
```

They differ from the presets above in build type only, `Release` rather than the `RelWithDebInfo`
a plain preset leaves in place, and each has a directory of its own so that neither disturbs the
build you develop in. Unit tests are off in both: nothing is shipped from them, and skipping them
skips fetching GoogleTest.

### When configure fails

`Could not find a package configuration file provided by "Qt6"` means Qt is missing: install
`qt6-base-dev` for the linux preset, or set `QT_MINGW_ROOT` for the windows one.

Changing the compiler or `QT_MINGW_ROOT` after a successful configure does nothing, because both
are already recorded in the cache. Delete the build directory and configure again.

## Formatting

Only the scripts. Never `clang-format` or `gersemi` by hand - they format what the scripts choose,
which is every tracked file, not whatever happened to be edited.

```bash
pip install -r requirements.txt        # the pinned python tools

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

## Coverage

```bash
sudo apt install llvm-18               # the profile tools, one version per clang

scripts/coverage.sh                    # build instrumented, run the tests, print the report
```

The build is the `linux-coverage` preset, so `build-coverage/` and `build/` never meet. Debug, because
optimized code no longer sits on the lines it was written on and the counts stop matching the
source. The script needs clang, takes the one in `CXX` if that is set, and looks the profile tools
up by the compiler's version, since a profile is not read by another.

The report covers `include/` and `src/`, and nothing else can enter it. A header-only dependency
is compiled into our translation units and instrumented with them, so leaving it out is a matter
of naming what is ours rather than of filtering out what is not. The player is measured beside
the tests, so the GUI reads 0 until there are GUI tests. Line by line, open
`build-coverage/coverage/html/index.html`.

`warning: N functions have mismatched data` is expected. It is what one profile covering several
test binaries looks like, and the numbers are unaffected.

The script also writes `coverage.lcov`, which is what CI hands to `diff-cover` to say how much of
a pull request's own lines are covered. The same locally:

```bash
diff-cover build-coverage/coverage/coverage.lcov --compare-branch origin/main
```

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
