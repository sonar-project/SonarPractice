# Third-party notices

This file summarizes licenses and linking for dependencies bundled with or required by SonarPractice. It is not legal advice.

## Qt 6

- **Use:** Application framework (Quick, Multimedia, Sql, Widgets, …).
- **License:** [Qt Licensing](https://www.qt.io/licensing/) — Qt libraries are used under the **GNU Lesser General Public License (LGPL) v3**.
- **Linking:** Dynamic (shared Qt libraries from the Qt SDK / installer).

## Rubber Band Library

- **Upstream:** [breakfastquay/rubberband](https://github.com/breakfastquay/rubberband) (v4.0.0 in Windows FetchContent builds).
- **Use:** Pitch-stable tempo change for backing tracks (`AudioLib` / `RubberBandPipeline`).
- **License:** **GNU General Public License (GPL)** (see upstream `COPYING` and file headers). Not LGPL. Commercial/proprietary use requires a [separate licence from Breakfast Quay](https://breakfastquay.com/technology/license.html).
- **Linking:**
  - **Linux:** dynamic only (`librubberband.so` via pkg-config; static archives are rejected at configure time).
  - **Windows:** dynamic only (`librubberband.dll`) build via CMake FetchContent (`single/RubberBandSingle.cpp`, tag `v4.0.0`).

## libgp_parser

- **Upstream:** [sonar-project/libgp_parser](https://github.com/sonar-project/libgp_parser) (FetchContent, release tag pinned in `cmake/Dependencies.cmake`).
- **Use:** Guitar Pro file import.
- **License:** **GNU Affero General Public License (AGPL) v3** (see upstream `LICENSE`).
- **Linking:** Static via FetchContent (compiled into the application).

## aubio

- **Upstream:** [aubio/aubio](https://github.com/aubio/aubio) (v0.4.9 via FetchContent).
- **Use:** Offline pitch / note extraction (`AudioLib` / `AudioAnalyzer`).
- **License:** **GNU General Public License (GPL) v3** (see upstream `COPYING`).
- **Linking:** Static via FetchContent (compiled into the application).

## FFmpeg

- **Upstream:** [FFmpeg Project](https://ffmpeg.org/).
- **Use:** Multimedia handling for video and audio files.
- **License:** Licensed under a combination of the LGPL v2.1, GPL v2 or later, and some parts under proprietary licenses for specific codecs. Please refer to [FFmpeg's official licensing page](https://www.ffmpeg.org/legal.html) for more details.
- **Linking:** Typically dynamic linking (e.g., `libavcodec`, `libavformat`) via CMake's `FindFFmpeg` module.
