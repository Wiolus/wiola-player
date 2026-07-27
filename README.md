# wiola-player

> One-paragraph description of the player goes here — what it plays, what makes
> it different, who it is for.

## Requirements

| | Minimum | Notes |
|---|---|---|
| CMake | 3.28 | ships with Ubuntu 24.04 |
| Compiler | GCC 14 or Clang 17 | C++26 is required |

GCC 13 and older cannot build this project — they do not accept `-std=c++26`.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/bin/wiola-player
```

If your default compiler is too old, select one explicitly. The compiler is
recorded in the CMake cache on first configure, so switching it needs a fresh
build directory:

```bash
rm -rf build
cmake -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug
```

`CMAKE_BUILD_TYPE` has no default. Omitting it produces a binary with neither
optimisation nor debug information.

## Usage

```
Usage: wiola-player [options] [file...]

Options:
  -h, --help       Show this help and exit
  -v, --version    Show version and exit
```

## Development

C++ sources are formatted with clang-format and CMake files with
[gersemi](https://github.com/BlankSpruce/gersemi). Both are pinned in
`requirements.txt`, so every contributor gets identical output:

```bash
python3 -m venv .venv && . .venv/bin/activate
pip install -r requirements.txt
```

Install the git hooks once per clone. This symlinks `scripts/hooks/*` into
`.git/hooks/` and does not modify your git configuration:

```bash
./scripts/install-hooks.sh
```

The `pre-commit` hook rejects commits when any tracked source or CMake file is
unformatted. Format everything with:

```bash
./scripts/format-cpp.sh      # C/C++ sources and headers
./scripts/format-cmake.sh    # CMakeLists.txt and *.cmake
```

Pass `--check` to either script to report without modifying anything, which is
what the hook does.

Styles are defined in `.clang-format` and `.gersemirc`. Both scripts operate on
files tracked by git, so generated CMake files under `build/` are never touched.
