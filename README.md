[![Apache 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![AppVeyor status](https://ci.appveyor.com/api/projects/status/kha9y50o39k3uscu?svg=true)](https://ci.appveyor.com/project/blagodarin/aulos)

[Download the latest Aulos Studio installer (may be unstable)](https://ci.appveyor.com/api/projects/blagodarin/aulos/artifacts/AulosStudio-0.0.0.exe?branch=master&job=Environment%3A%20CONFIG%3DRelease%2C%20ARCH%3Damd64%2C%20QTDIR%3DC%3A%5CQt%5C5.13.2%5Cmsvc2017_64%2C%20INSTALLER%3DON)


# aulos

**aulos** is an audio synthesis toolkit.

Key concepts:
* Fully synthetic sound ("vector audio" as a counterpart to vector graphics).
* Suitable for realtime audio generation in performance-demanding applications (e. g. games).
* Easy to integrate into existing codebases (as a static library which only depends on the C++ standard library).
* Includes a full-featured utility for audio creation, editing and playback.


## Disclaimer

**aulos** is in its early stages of development. Until it reaches version 0.1,
*any* commit may change *anything* (API, ABI, file format) in an incompatible way.
