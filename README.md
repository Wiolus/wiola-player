# wiola-player

> One-paragraph description of the player goes here - what it plays, what makes it different,
> who it is for.

Plays WAV, FLAC and MP3.

## Build

Needs CMake 3.28, GCC 13 or Clang 17, and Qt 6 Widgets. Everything else is fetched during
configure.

```bash
sudo apt install cmake g++ qt6-base-dev

cmake --preset linux
cmake --build --preset linux
ctest --preset linux

./build/bin/wiola-player
```

A Windows executable is cross-compiled from Linux with the `windows` preset, which additionally
needs a Qt built for MinGW-w64.

[CONTRIBUTING.md](CONTRIBUTING.md) has the full guide for both, along with formatting and commit
names.

## Usage

The window opens with no track loaded. Tracks are chosen in it rather than on the command line.

```
wiola-player

  -h, --help     Show this help and exit
  -v, --version  Show version and exit
```
