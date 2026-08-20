# wiola-player

> One-paragraph description of the player goes here - what it plays, what makes it different,
> who it is for.

Plays WAV, FLAC and MP3.

## Requirements

| | Minimum | Notes |
|---|---|---|
| CMake | 3.28 | ships with Ubuntu 24.04 |
| Compiler | GCC 13 or Clang 17 | C++23 |
| Qt | 6, Widgets | `qt6-base-dev` |

Qt is the only thing that has to be installed, and it is not optional: the player is the window.
CMake fetches dr_libs, miniaudio, CLI11 and GoogleTest, all pinned in `cmake/dependencies.cmake`.

## Build

```bash
cmake --preset linux
cmake --build --preset linux
ctest --preset linux

./build/bin/wiola-player
```

Builds are `RelWithDebInfo` unless you ask for something else. The compiler is recorded in the
cache on first configure, so switching it needs a fresh build directory.

## Windows

Cross-compiled from Linux. The result needs no DLLs beside it.

```bash
sudo apt install g++-mingw-w64-x86-64-posix

cmake --preset windows
cmake --build --preset windows        # build-win/bin/wiola-player.exe
```

`QT_MINGW_ROOT` has to name a Qt built for MinGW, because no distribution packages one and
configure will not go on without it. `scripts/build-qt-mingw.sh` builds a static Qt that keeps the
no-DLL property, and [scripts/README.md](scripts/README.md) says what it needs:

```bash
scripts/build-qt-mingw.sh ~/opt/qt6-mingw-static
export QT_MINGW_ROOT=~/opt/qt6-mingw-static
```

## Usage

Run it with nothing and the window opens. Tracks are chosen in it, not on the command line.

```
wiola-player

  -h, --help     Show this help and exit
  -v, --version  Show version and exit
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for formatting and commit names.
