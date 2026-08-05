# wiola-player

> One-paragraph description of the player goes here - what it plays, what makes it different,
> who it is for.

Plays WAV, FLAC and MP3.

## Requirements

| | Minimum | Notes |
|---|---|---|
| CMake | 3.28 | ships with Ubuntu 24.04 |
| Compiler | GCC 13 or Clang 17 | C++23 |

Nothing else needs installing. CMake fetches dr_libs, miniaudio, CLI11 and GoogleTest, all pinned
in `cmake/dependencies.cmake`.

## Build

```bash
cmake --preset linux
cmake --build --preset linux
ctest --preset linux

./build/bin/wiola-player --file track.flac
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

## Usage

```
wiola-player [OPTIONS]

  -h, --help       Show this help and exit
  -v, --version    Show version and exit
      --file TEXT  Play an audio file
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for formatting and commit names.
