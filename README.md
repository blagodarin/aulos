[![Apache 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![AppVeyor status](https://ci.appveyor.com/api/projects/status/kha9y50o39k3uscu?svg=true)](https://ci.appveyor.com/project/blagodarin/aulos)
[![Travis CI status](https://travis-ci.org/blagodarin/aulos.svg?branch=master)](https://travis-ci.org/github/blagodarin/aulos)


### [Download Aulos Studio 0.0.3 for Windows](https://ci.appveyor.com/api/buildjobs/39onvf6nvf22bw0t/artifacts/AulosStudio-0.0.3.exe)

[...or get the latest unstable installer at your own risk](https://ci.appveyor.com/api/projects/blagodarin/aulos/artifacts/AulosStudio-0.0.3-unstable.exe?branch=master&job=Environment%3A%20CONFIG%3DRelease%2C%20ARCH%3Damd64%2C%20QTDIR%3DC%3A%5CQt%5C5.13.2%5Cmsvc2017_64%2C%20INSTALLER%3DON)


# Aulos

Aulos is an audio synthesis toolkit consisting of:
* **Aulos Studio**, a full-featured tool for working with Aulos compositions.
  Aulos Studio provides convenient ways to create, edit, play and export Aulos compositions
  without requiring any experience in making music.
* **aulos** library which provides functionality to convert Aulos compositions into waveform data.
  Aulos compositions are tiny compared to what one would expect from a piece of music,
  yet they can be rendered with any desired quality.

The primary features of Aulos are:
* **Embeddable technology.** The library has no external dependencies (apart from
  the C++ standard library) and can be easily integrated into third-party applications.
* **Fully synthetic sound.** Each Aulos composition is completely self-contained
  and doesn't require additional data (e. g. sound banks) to be rendered.
* **Focus on performance.** All design decision are guided by performance considerations,
  and every change in the synthesis code is benchmarked.
* **Incremental synthesis.** A composition can be rendered block by block,
  which allows to reduce memory requirements and improve load times.


## What does it look like

![Screenshot of Aulos Studio](studio/screenshot.png)


## What to expect

First of all, Aulos is in its early stages of development. *Any* commit may change *anything*
(API, ABI, file format) in an incompatible way until Aulos reaches at least version 0.1,
which will be the first more-or-less production-ready release.

This bumpy journey may also bring:
* **Binary file format.** Binary data is more compact and should be faster to load,
  so this is definitely coming, but not before the composition structure gets stabilized.
  At this stage of development, some format changes require manual file manipulations,
  which are easier to do with text than with binary data.
* **Ergonomic voice editor.** The current editor is both limited and complicated.
  It clearly lacks visualization of what's going on.
* **Voice libraries.** It's inconvenient to start every composition from scratch,
  reinventing all voices or copying them value-by-value from another composition.
  It's also more user-friendly to provide newcomers with a set of pre-designed
  voices than to force them to make their own.

P. S. No estimates, sorry.
