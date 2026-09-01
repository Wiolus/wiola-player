[![coverage](https://img.shields.io/endpoint?url=https%3A%2F%2Fwiolus.github.io%2Fwiola-player%2Fcoverage%2Fbadge.json)](https://wiolus.github.io/wiola-player/coverage/)

# Wiola Player

A small desktop music player for WAV, FLAC and MP3. One window: play, seek, volume and a ten-band
equalizer. On Windows it is a single executable: nothing to install, no DLLs to place beside it.

<img src="assets/wiola-demo.png" alt="wiola-player playing a track on Windows" width="410">

## Run it on Windows

1. Download
   [`wiola-player.exe`](https://github.com/Wiolus/wiola-player/releases/latest/download/wiola-player.exe)
   from the [latest release](https://github.com/Wiolus/wiola-player/releases/latest).
2. Double-click it.
3. Press **Open** and pick a track.

There is no installer and no setup. Delete the file when you are done with it.

## Run it on Linux

There is no package yet, so it is built from source. [CONTRIBUTING.md](CONTRIBUTING.md) has the
steps.

## Using it

- **Open** picks a track, **Play** and **Stop** run it.
- The bar shows where you are - click it to jump.
- The slider sets the volume, **+** takes it to 140%.
- **EQ** opens a ten-band equalizer, up to 12 dB a band.

The volume and the equalizer are kept for the next run.

## Contributing

[CONTRIBUTING.md](CONTRIBUTING.md) is the one place the build is described, for both platforms,
along with the test suite, coverage, formatting and commit names.

## License

GPL-3.0. See [LICENSE](LICENSE).
