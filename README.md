[![Apache 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![AppVeyor status](https://ci.appveyor.com/api/projects/status/kha9y50o39k3uscu?svg=true)](https://ci.appveyor.com/project/blagodarin/aulos)

[Download the latest unstable Aulos Studio installer](https://ci.appveyor.com/api/projects/blagodarin/aulos/artifacts/AulosStudio-0.0.2-unstable.exe?branch=master&job=Environment%3A%20CONFIG%3DRelease%2C%20ARCH%3Damd64%2C%20QTDIR%3DC%3A%5CQt%5C5.13.2%5Cmsvc2017_64%2C%20INSTALLER%3DON)


# Aulos

Aulos is an audio synthesis toolkit consisting of:
* **Aulos Studio**, a full-featured tool for working with Aulos compositions.
  Aulos Studio provides convenient ways to create, edit, play and export Aulos compositions
  without requiring any experience in making music.
* **aulos** library which provides functionality to convert Aulos compositions into waveform data.
  Aulos compositions are tiny compared to what one would expect from a piece of music,
  yet they can be rendered with any desired quality.
  The library has no external dependencies (apart from the C++ standard library)
  and can be easily integrated into third-party applications.

**Disclaimer.** Aulos is in its early stages of development.
*Any* commit may change *anything* (API, ABI, file format) in an incompatible way
until Aulos reaches version 0.1, which will be the first production-ready release.
You are encouraged to play with it right away though. :)
