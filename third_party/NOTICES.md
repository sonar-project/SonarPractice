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

## FFmpeg

- **Upstream:** [FFmpeg Project](https://ffmpeg.org/).
- **Use:** Multimedia handling for video and audio files.
- **License:** Licensed under a combination of the LGPL v2.1, GPL v2 or later, and some parts under proprietary licenses for specific codecs. Please refer to [FFmpeg's official licensing page](https://ffmpeg.org/legal.html) for more details.
- **Linking:** Typically dynamic linking (e.g., `libavcodec`, `libavformat`) via CMake's `FindFFmpeg` module.

## alphaTab (optional interactive GP player)

- **Upstream:** [coderline/alphaTab](https://github.com/CoderLine/alphaTab) (vendored under `third_party/alphatab` / `src/ui/web/alphatab`, currently **1.8.4**).
- **Use:** Score + tablature rendering and MIDI playback inside an embedded Qt WebEngine view.
- **License:** **Mozilla Public License 2.0** (see `src/ui/web/alphatab/ALPHATAB_LICENSE`).
- **Bundled assets:**
  - **Bravura** music font — SIL Open Font License (see `src/ui/web/alphatab/font/Bravura-OFL.txt`).
  - **SONiVOX SoundFont** (`sonivox.sf3`) — Apache License 2.0 (see `src/ui/web/alphatab/soundfont/LICENSE`).
- **Linking:** Not linked as native code; JS/font/soundfont are packaged as Qt resources and executed in Qt WebEngine when `Qt6::WebEngineQuick` is available at build time (`SONARPRACTICE_HAS_WEBENGINE`).
