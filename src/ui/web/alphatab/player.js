/**
 * AlphaTab bridge for SonarPractice (loaded inside Qt WebEngine).
 * Native UI calls window.sonarAlphaTab.* via runJavaScript.
 * Events go to native via console.log('SONAR|' + json).
 */
(function () {
  "use strict";

  const statusEl = document.getElementById("status");
  const hostEl = document.getElementById("alphaTab");
  const scrollEl = document.getElementById("wrap");

  /** @type {alphaTab.AlphaTabApi|null} */
  let api = null;
  let selectedTrackIndex = 0;
  let scoreReady = false;
  let playerReady = false;
  let darkTheme = true;
  let pendingTempoPercent = 100;
  let pendingLoop = {
    enabled: false,
    startBar: 1,
    endBar: 1,
  };
  /** @type {{ volume: number, muted: boolean }[]} */
  let pendingMixer = [];
  let pendingMetronomeEnabled = false;
  let pendingMetronomeDivision = 4;
  let pendingCountInEnabled = false;
  /** True while AlphaTab is playing the one-bar count-in sequence. */
  let countInPhase = false;
  let countInBeatsPlayed = 0;
  /** DOM snapshot so the beat/bar cursor stays put while count-in ticks advance. */
  let countInCursorFreeze = null;
  let countInCursorRaf = null;
  let countInCursorCaptureRaf = null;
  /** Loop restart: seek first, capture cursor, then play (avoids freezing at soft-end). */
  let countInPendingRestartPlay = false;
  /**
   * True while the user wants continuous playback. Distinguishes a natural
   * loop-end (restart with optional count-in) from Stop/Pause.
   */
  let wantPlayback = false;
  /** Suppresses a brief "paused" UI flash while manually restarting a loop. */
  let loopRestartInProgress = false;
  /**
   * Synth often finishes early on trailing rests (no voices left). While true we
   * keep the metronome running until the full loop end tick has elapsed.
   */
  let restHoldActive = false;
  let loopRestartTimerId = null;
  /** Wall-clock anchor when the musical loop range (after count-in) began. */
  let rangePassAnchorPerfMs = 0;
  let rangePassAnchorTick = 0;
  /** Highest tick seen while playing the current pass (survives stop()-seek-to-start). */
  let lastPlaybackTick = 0;
  /** True after we soft-paused at the musical loop end (before count-in). */
  let softLoopEnding = false;
  /** Live score tempo (BPM); updated from playerPositionChanged when available. */
  let liveBpm = 120;
  /** Independent metronome clock (avoids AlphaTab event-rate / duration quirks). */
  let metroTimerId = null;
  let metroClickIndex = 0;
  /** True while the wall-clock metro is supposed to be running (survives timer gaps). */
  let metroClockActive = false;
  /**
   * After count-in, wait for the first musical downbeat before the metro clock
   * clicks — starting on the last count-in beat shifts "1" earlier each loop.
   */
  let pendingMetroStartOnNextDownbeat = false;
  let metroStartFallbackId = null;
  let pendingTransposeSemitones = 0;
  let appliedTransposeSemitones = 0;
  const PERCUSSION_CHANNEL = 9;
  let applyingSettings = false;
  const BUILTIN_SOUND_FONT = "./soundfont/sonivox.sf3";
  const CUSTOM_SOUND_FONT_KEY = "__custom__";
  let useCustomSoundFont = false;
  let activeSoundFontKey = null;
  let loadingSoundFontKey = null;
  /** @type {ArrayBuffer[]|null} */
  let soundFontBytesBuffers = null;

  /** @type {AudioContext|null} */
  let clickAudioCtx = null;
  /** @type {number[]} */
  let subdivisionTimeouts = [];

  function setStatus(text, isError) {
    statusEl.textContent = text || "";
    statusEl.className = isError ? "error" : "";
    statusEl.style.display = text ? "block" : "none";
  }

  function notifyNative(eventName, payload) {
    try {
      console.log("SONAR|" + JSON.stringify({ event: eventName, payload: payload || {} }));
    } catch (e) {
      /* ignore */
    }
  }

  function base64ToArrayBuffer(base64) {
    const binary = atob(base64);
    const len = binary.length;
    const bytes = new Uint8Array(len);
    for (let i = 0; i < len; i++) {
      bytes[i] = binary.charCodeAt(i);
    }
    return bytes.buffer;
  }

  function pageColors(isDark) {
    if (isDark) {
      return {
        bg: "#16161f",
        text: "#eceff1",
        muted: "#90a4ae",
        error: "#ef9a9a",
        cursorBar: "rgba(124, 77, 255, 0.25)",
        cursorBeat: "#b39ddb",
        highlight: "#b39ddb",
      };
    }
    return {
      bg: "#ffffff",
      text: "#1a1a2e",
      muted: "#6b7280",
      error: "#c62828",
      cursorBar: "rgba(101, 31, 255, 0.18)",
      cursorBeat: "#651fff",
      highlight: "#651fff",
    };
  }

  function notationResources(isDark) {
    if (isDark) {
      return {
        staffLineColor: "#ffffff80",
        barSeparatorColor: "#cfd8dc",
        mainGlyphColor: "#cfd8dc",
        secondaryGlyphColor: "#90a4ae",
        scoreInfoColor: "#cfd8dc",
        barNumberColor: "#90a4ae",
      };
    }
    return {
      staffLineColor: "#1a1a2e80",
      barSeparatorColor: "#1a1a2e",
      mainGlyphColor: "#1a1a2e",
      secondaryGlyphColor: "#5f6b7a",
      scoreInfoColor: "#1a1a2e",
      barNumberColor: "#6b7280",
    };
  }

  /**
   * GP files often embed black header colors (Title/SubTitle/…). AlphaTab prefers
   * those over resources.scoreInfoColor — replace with the active theme color.
   */
  function applyEmbeddedScoreInfoColors(isDark) {
    try {
      const style = api && api.score && api.score.style;
      if (!style) {
        return;
      }
      if (!style.colors) {
        style.colors = new Map();
      }
      const Color = alphaTab.model && alphaTab.model.Color;
      if (!Color || typeof Color.fromJson !== "function") {
        return;
      }
      const hex = isDark ? "#cfd8dc" : "#1a1a2e";
      const themed = Color.fromJson(hex);
      const ScoreSubElement = alphaTab.model.ScoreSubElement;
      const keys = [];
      if (ScoreSubElement) {
        [
          "Title",
          "SubTitle",
          "Artist",
          "Album",
          "Words",
          "Music",
          "WordsAndMusic",
          "Transcriber",
          "Copyright",
          "CopyrightSecondLine",
        ].forEach(function (name) {
          if (ScoreSubElement[name] !== undefined) {
            keys.push(ScoreSubElement[name]);
          }
        });
      }
      if (keys.length === 0) {
        for (let i = 0; i <= 9; i++) {
          keys.push(i);
        }
      }
      // Replace known header keys and any other embedded entries (still often black).
      keys.forEach(function (key) {
        style.colors.set(key, themed);
      });
      if (typeof style.colors.forEach === "function") {
        style.colors.forEach(function (_value, key) {
          style.colors.set(key, themed);
        });
      }
    } catch (e) {
      /* ignore */
    }
  }

  function applyPageTheme(isDark) {
    const c = pageColors(isDark);
    const root = document.documentElement;
    root.style.setProperty("--sp-bg", c.bg);
    root.style.setProperty("--sp-text", c.text);
    root.style.setProperty("--sp-muted", c.muted);
    root.style.setProperty("--sp-error", c.error);
    root.style.setProperty("--sp-cursor-bar", c.cursorBar);
    root.style.setProperty("--sp-cursor-beat", c.cursorBeat);
    root.style.setProperty("--sp-highlight", c.highlight);
  }

  /** Update notation colors. Prefer renderTracks over render() so track selection sticks. */
  function applyNotationTheme(isDark, rerender) {
    if (!api) {
      return;
    }
    const Color = alphaTab.model && alphaTab.model.Color;
    if (!Color || typeof Color.fromJson !== "function") {
      return;
    }
    const resources = notationResources(isDark);
    const target = api.settings.display.resources;
    Object.keys(resources).forEach(function (key) {
      target[key] = Color.fromJson(resources[key]);
    });
    applyEmbeddedScoreInfoColors(isDark);
    api.updateSettings();
    installCanvasColorReuseFix(api);
    if (rerender && scoreReady) {
      applyTrackVisibility();
    }
  }

  function scoreRendererOf(instance) {
    if (!instance || !instance.renderer) {
      return null;
    }
    // AlphaTabApi.renderer is a wrapper; the real ScoreRenderer is .instance.
    return instance.renderer.instance || instance.renderer;
  }

  /**
   * AlphaTab's canvas `color` setter only writes fillStyle when rgba changed, but beginRender
   * creates a fresh 2d context without resetting the cached color. Later partials that reuse
   * the same rgba (title → tuning → "rendered by alphaTab") keep the browser default — black.
   */
  function installCanvasColorReuseFix(instance) {
    try {
      const scoreRenderer = scoreRendererOf(instance);
      const canvas = scoreRenderer && scoreRenderer.canvas;
      if (!canvas || typeof canvas.beginRender !== "function") {
        return false;
      }
      const Color = alphaTab.model && alphaTab.model.Color;
      if (!Color || typeof Color.fromJson !== "function") {
        return false;
      }
      const proto = Object.getPrototypeOf(canvas);
      if (proto && !proto.__spColorReuseFix) {
        const origProtoBegin = proto.beginRender;
        proto.beginRender = function () {
          const result = origProtoBegin.apply(this, arguments);
          this.Sm = Color.fromJson("#01020300");
          return result;
        };
        proto.__spColorReuseFix = true;
      }
      if (!canvas.__spColorReuseFix) {
        const origBegin = canvas.beginRender.bind(canvas);
        canvas.beginRender = function () {
          const result = origBegin.apply(this, arguments);
          this.Sm = Color.fromJson("#01020300");
          return result;
        };
        canvas.__spColorReuseFix = true;
      }
      return true;
    } catch (e) {
      return false;
    }
  }

  function applyTrackVisibility() {
    if (!api || !api.score) {
      return;
    }
    const tracks = api.score.tracks || [];
    if (tracks.length === 0) {
      return;
    }
    if (selectedTrackIndex < 0 || selectedTrackIndex >= tracks.length) {
      selectedTrackIndex = 0;
    }
    applyEmbeddedScoreInfoColors(darkTheme);
    installCanvasColorReuseFix(api);
    api.renderTracks([tracks[selectedTrackIndex]]);
  }

  /** Linear (non-repeat) fallback ticks from score.masterBars. */
  function linearBarTicks(barNumber) {
    if (!api || !api.score || !api.score.masterBars || api.score.masterBars.length === 0) {
      return { startTick: 0, endTick: 0 };
    }
    const bars = api.score.masterBars;
    const idx = Math.max(0, Math.min(bars.length - 1, barNumber - 1));
    const startTick = Number(bars[idx].start) || 0;
    let endTick = startTick;
    if (idx + 1 < bars.length) {
      endTick = Number(bars[idx + 1].start) || startTick;
    } else {
      try {
        endTick = startTick + Number(bars[idx].calculateDuration()) || startTick;
      } catch (e) {
        endTick = startTick;
      }
    }
    return { startTick, endTick };
  }

  /**
   * Playback ticks for a 1-based bar range, expanded through repeats/jumps via tickCache.
   * Written masterBars[].start ignores repeats — e.g. bar1×2 then bar2 needs end after bar2,
   * not after two written bar lengths (which would cut before bar2 ever plays).
   */
  function loopPlaybackTicks(startBar, endBar) {
    const startIdx = Math.max(0, (Number(startBar) || 1) - 1);
    const endIdx = Math.max(startIdx, (Number(endBar) || startBar) - 1);
    const cache = api && api.tickCache;
    const lookups = cache && cache.masterBars;
    if (lookups && lookups.length > 0) {
      let startTick = null;
      let endTick = null;
      for (let i = 0; i < lookups.length; i++) {
        const lookup = lookups[i];
        if (!lookup || !lookup.masterBar) {
          continue;
        }
        const idx = Number(lookup.masterBar.index);
        if (!Number.isFinite(idx)) {
          continue;
        }
        if (startTick === null) {
          if (idx === startIdx) {
            startTick = Number(lookup.start);
            endTick = Number(lookup.end);
          }
          continue;
        }
        if (idx < startIdx || idx > endIdx) {
          break;
        }
        endTick = Number(lookup.end);
      }
      if (
        startTick !== null &&
        endTick !== null &&
        Number.isFinite(startTick) &&
        Number.isFinite(endTick) &&
        endTick > startTick
      ) {
        return { startTick, endTick };
      }
    }

    const first = linearBarTicks(startIdx + 1);
    const last = linearBarTicks(endIdx + 1);
    return { startTick: first.startTick, endTick: last.endTick };
  }

  /** Start tick of the first playback of a 1-based master bar. */
  function tickForBar(barNumber) {
    return loopPlaybackTicks(barNumber, barNumber).startTick;
  }

  /** Exclusive end tick after the last in-range playback of endBar (with startBar context). */
  function endTickForBar(startBar, endBar) {
    if (endBar === undefined) {
      // Legacy single-arg: end of that bar's first contiguous span only.
      return loopPlaybackTicks(startBar, startBar).endTick;
    }
    return loopPlaybackTicks(startBar, endBar).endTick;
  }

  function quarterTickResolution() {
    try {
      if (alphaTab.midi && alphaTab.midi.MidiUtils && alphaTab.midi.MidiUtils.quarterTime) {
        return Number(alphaTab.midi.MidiUtils.quarterTime) || 960;
      }
    } catch (e) {
      /* ignore */
    }
    return 960;
  }

  function ticksToWallMs(ticks) {
    const t = Math.max(0, Number(ticks) || 0);
    const bpm = Math.max(1, resolveBpm());
    const speed = api && api.playbackSpeed > 0 ? Number(api.playbackSpeed) : 1;
    const msPerQuarter = 60000 / bpm / speed;
    return (t / quarterTickResolution()) * msPerQuarter;
  }

  function clearLoopRestartTimer() {
    if (loopRestartTimerId !== null) {
      clearTimeout(loopRestartTimerId);
      loopRestartTimerId = null;
    }
    restHoldActive = false;
  }

  function nowMs() {
    return typeof performance !== "undefined" && performance.now
      ? performance.now()
      : Date.now();
  }

  function markRangePassAnchor() {
    rangePassAnchorPerfMs = nowMs();
    rangePassAnchorTick = pendingLoop.enabled
      ? tickForBar(pendingLoop.startBar)
      : 0;
    lastPlaybackTick = rangePassAnchorTick;
  }

  /** Musical ms between two ticks, preferring AlphaTab's tempo map + playbackSpeed. */
  function tickDeltaToWallMs(fromTick, toTick) {
    const from = Math.max(0, Number(fromTick) || 0);
    const to = Math.max(from, Number(toTick) || 0);
    if (api && typeof api.tickPositionToTimePosition === "function") {
      try {
        const endMs = Number(api.tickPositionToTimePosition(to));
        const startMs = Number(api.tickPositionToTimePosition(from));
        if (Number.isFinite(endMs) && Number.isFinite(startMs)) {
          return Math.max(0, endMs - startMs);
        }
      } catch (e) {
        /* fall through */
      }
    }
    return ticksToWallMs(to - from);
  }

  function applyPendingTempo() {
    if (!api) {
      return;
    }
    const p = Math.max(25, Math.min(200, Number(pendingTempoPercent) || 100));
    api.playbackSpeed = p / 100.0;
  }

  /** First beat of a 1-based master bar on the currently selected track. */
  function beatAtBarBoundary(barNumber, preferLast) {
    if (!api || !api.score || !api.score.tracks || api.score.tracks.length === 0) {
      return null;
    }
    const tracks = api.score.tracks;
    const track =
      selectedTrackIndex >= 0 && selectedTrackIndex < tracks.length
        ? tracks[selectedTrackIndex]
        : tracks[0];
    if (!track || !track.staves || track.staves.length === 0) {
      return null;
    }
    const bars = track.staves[0].bars;
    if (!bars || bars.length === 0) {
      return null;
    }
    const idx = Math.max(0, Math.min(bars.length - 1, barNumber - 1));
    const bar = bars[idx];
    if (!bar || !bar.voices || bar.voices.length === 0) {
      return null;
    }
    const beats = bar.voices[0].beats;
    if (!beats || beats.length === 0) {
      return null;
    }
    return preferLast ? beats[beats.length - 1] : beats[0];
  }

  function clearLoopHighlight() {
    if (!api) {
      return;
    }
    if (typeof api.clearPlaybackRangeHighlight === "function") {
      api.clearPlaybackRangeHighlight();
    }
  }

  /** Mirror the top-bar loop range as a visible selection on the tab. */
  function highlightLoopBars(startBar, endBar) {
    if (!api || typeof api.highlightPlaybackRange !== "function") {
      return;
    }
    const startBeat = beatAtBarBoundary(startBar, false);
    const endBeat = beatAtBarBoundary(endBar, true);
    if (!startBeat || !endBeat) {
      return;
    }
    try {
      api.highlightPlaybackRange(startBeat, endBeat);
    } catch (e) {
      /* ignore — beat refs can be stale mid-render */
    }
  }

  function applyPendingLoop() {
    if (!api || !scoreReady) {
      return;
    }
    if (!pendingLoop.enabled) {
      // Never leave AlphaTab auto-loop on; we restart manually on loop end.
      api.isLooping = false;
      api.playbackRange = null;
      clearLoopHighlight();
      return;
    }
    const start = Math.max(1, Number(pendingLoop.startBar) || 1);
    const end = Math.max(start, Number(pendingLoop.endBar) || start);
    const range = loopPlaybackTicks(start, end);
    const startTick = range.startTick;
    const musicalEndTick = range.endTick;
    // Tiny pad past the bar line so we can soft-pause before AlphaTab's stop()/noteOffAll.
    const rangeEndTick = musicalEndTick + Math.max(1, Math.round(quarterTickResolution() / 8));
    const existing = api.playbackRange;
    const sameRange =
      existing &&
      Number(existing.startTick) === startTick &&
      Number(existing.endTick) === rangeEndTick;

    // Assigning playbackRange seeks to startTick — avoid that while playing/holding.
    if (!sameRange) {
      const preserveTick =
        (isPlayerPlaying() || restHoldActive || softLoopEnding) && lastPlaybackTick > startTick
          ? lastPlaybackTick
          : 0;
      api.playbackRange = {
        startTick: startTick,
        endTick: rangeEndTick,
      };
      if (preserveTick > 0) {
        try {
          api.tickPosition = preserveTick;
        } catch (e) {
          /* ignore */
        }
      }
    }
    api.isLooping = false;
    highlightLoopBars(start, end);
  }

  /** Seek to loop start and play, optionally with count-in. */
  function restartPlaybackFromLoopStart() {
    if (!api) {
      return;
    }
    clearLoopRestartTimer();
    softLoopEnding = false;
    restHoldActive = false;
    loopRestartInProgress = true;
    stopMetronomeClock();
    clearSubdivisionClicks();
    endCountInPhase();
    rangePassAnchorPerfMs = 0;
    rangePassAnchorTick = 0;
    ensureClickAudio();
    applyPendingMetronome();
    const startTick = pendingLoop.enabled ? tickForBar(pendingLoop.startBar) : 0;
    try {
      api.tickPosition = startTick;
      lastPlaybackTick = startTick;
    } catch (e) {
      /* ignore */
    }

    if (pendingCountInEnabled) {
      // Seek → paint cursor at loop start → freeze → then play count-in.
      countInPendingRestartPlay = true;
      beginCountInPhaseIfNeeded();
      let frames = 0;
      function waitSeekThenPlay() {
        frames += 1;
        if (!wantPlayback) {
          countInPendingRestartPlay = false;
          loopRestartInProgress = false;
          return;
        }
        if (frames < 3) {
          requestAnimationFrame(waitSeekThenPlay);
          return;
        }
        const snap = readCursorFreezeSnapshot();
        const hasPos =
          snap &&
          ((snap.beat && String(snap.beat.left).length > 0) ||
            (snap.bar && String(snap.bar.left).length > 0));
        if (hasPos) {
          armCountInCursorFreeze(snap);
        }
        finishPendingCountInRestartPlay();
      }
      requestAnimationFrame(waitSeekThenPlay);
      // Fallback if rAF path stalls.
      setTimeout(function () {
        if (countInPendingRestartPlay) {
          const snap = readCursorFreezeSnapshot();
          if (snap) {
            armCountInCursorFreeze(snap);
          }
          finishPendingCountInRestartPlay();
        }
      }, 200);
      return;
    }

    beginCountInPhaseIfNeeded();
    api.play();
    setTimeout(function () {
      loopRestartInProgress = false;
    }, 50);
  }

  /**
   * Soft loop boundary: pause at the end of the last bar (avoid AlphaTab stop()/noteOffAll
   * hard-cut), keep the cursor there for one beat, then count-in + restart.
   */
  function beginSoftLoopEnd() {
    if (!api || !wantPlayback || !pendingLoop.enabled) {
      return;
    }
    if (softLoopEnding || loopRestartInProgress) {
      return;
    }
    softLoopEnding = true;
    restHoldActive = true;
    clearLoopRestartTimer();

    try {
      api.pause();
    } catch (e) {
      /* ignore */
    }

    const range = loopPlaybackTicks(pendingLoop.startBar, pendingLoop.endBar);
    try {
      // Park at the expanded end (after repeats), not the first written occurrence of endBar.
      api.tickPosition = Math.max(range.startTick, range.endTick - 1);
    } catch (e) {
      /* ignore */
    }

    // One quarter-note breath at the bar line before count-in.
    const ringMs = Math.max(250, Math.round(60000 / Math.max(1, resolveBpm()) / Math.max(0.01, api.playbackSpeed || 1)));

    if (pendingMetronomeEnabled && !countInPhase && !metroClockActive) {
      startMetronomeClock(false);
    }

    loopRestartTimerId = setTimeout(function () {
      loopRestartTimerId = null;
      restHoldActive = false;
      softLoopEnding = false;
      if (!wantPlayback || !pendingLoop.enabled) {
        stopMetronomeClock();
        return;
      }
      restartPlaybackFromLoopStart();
    }, ringMs);
  }

  function normalizeDivision(raw) {
    const n = Number(raw);
    if (n === 8 || n === 16 || n === 32) {
      return n;
    }
    return 4;
  }

  function clicksPerQuarter(division) {
    return Math.max(1, normalizeDivision(division) / 4);
  }

  function resolveBpm() {
    if (liveBpm > 0) {
      return liveBpm;
    }
    try {
      if (api && api.score) {
        const t = Number(api.score.tempo);
        if (t > 0) {
          return t;
        }
      }
    } catch (e) {
      /* ignore */
    }
    return 120;
  }

  function metronomeIntervalMs() {
    const speed = api && api.playbackSpeed > 0 ? Number(api.playbackSpeed) : 1;
    const quarterWallMs = 60000 / resolveBpm() / speed;
    return quarterWallMs / clicksPerQuarter(pendingMetronomeDivision);
  }

  function metronomeClicksPerBar() {
    return countInBeatCount() * clicksPerQuarter(pendingMetronomeDivision);
  }

  function cancelArmedMetronomeStart() {
    if (metroStartFallbackId !== null) {
      clearTimeout(metroStartFallbackId);
      metroStartFallbackId = null;
    }
    pendingMetroStartOnNextDownbeat = false;
  }

  function stopMetronomeClock() {
    if (metroTimerId !== null) {
      clearTimeout(metroTimerId);
      metroTimerId = null;
    }
    metroClockActive = false;
    cancelArmedMetronomeStart();
  }

  function isPlayerPlaying() {
    if (!api) {
      return false;
    }
    const Playing =
      alphaTab.synth && alphaTab.synth.PlayerState
        ? alphaTab.synth.PlayerState.Playing
        : 1;
    return api.playerState === Playing;
  }

  function scheduleNextMetronomeClockTick() {
    if (metroTimerId !== null) {
      clearTimeout(metroTimerId);
      metroTimerId = null;
    }
    if (!metroClockActive || !pendingMetronomeEnabled || countInPhase) {
      return;
    }
    if (!isPlayerPlaying() && !restHoldActive) {
      return;
    }
    const interval = Math.max(25, metronomeIntervalMs());
    metroTimerId = setTimeout(function () {
      metroTimerId = null;
      if (!metroClockActive || !pendingMetronomeEnabled || countInPhase) {
        return;
      }
      if (!isPlayerPlaying() && !restHoldActive) {
        return;
      }
      const perBar = metronomeClicksPerBar();
      playWebClick(perBar > 0 && metroClickIndex % perBar === 0);
      metroClickIndex += 1;
      scheduleNextMetronomeClockTick();
    }, interval);
  }

  function startMetronomeClock(resetIndex) {
    cancelArmedMetronomeStart();
    if (resetIndex) {
      metroClickIndex = 0;
    }
    // Already running: never click again (playerStateChanged used to double-attack
    // ~1 buffer after count-in → "1" drifts further each loop).
    if (metroClockActive) {
      return;
    }
    if (!pendingMetronomeEnabled || countInPhase) {
      return;
    }
    if (!isPlayerPlaying() && !restHoldActive) {
      return;
    }
    if (metroTimerId !== null) {
      clearTimeout(metroTimerId);
      metroTimerId = null;
    }
    metroClockActive = true;
    const perBar = metronomeClicksPerBar();
    playWebClick(perBar > 0 && metroClickIndex % perBar === 0);
    metroClickIndex += 1;
    scheduleNextMetronomeClockTick();
  }

  /**
   * Arm metro so the first click lands on the next musical "1", not on the
   * last count-in beat (which already produced a click).
   */
  function armMetronomeStartOnNextDownbeat() {
    cancelArmedMetronomeStart();
    if (!pendingMetronomeEnabled || countInPhase || metroClockActive) {
      return;
    }
    pendingMetroStartOnNextDownbeat = true;
    const interval = Math.max(25, metronomeIntervalMs());
    metroStartFallbackId = setTimeout(function () {
      metroStartFallbackId = null;
      if (!pendingMetroStartOnNextDownbeat) {
        return;
      }
      pendingMetroStartOnNextDownbeat = false;
      if (pendingMetronomeEnabled && !countInPhase && (isPlayerPlaying() || restHoldActive)) {
        startMetronomeClock(true);
      }
    }, interval);
  }

  function ensureClickAudio() {
    if (!clickAudioCtx) {
      const Ctx = window.AudioContext || window.webkitAudioContext;
      if (!Ctx) {
        return null;
      }
      clickAudioCtx = new Ctx();
    }
    if (clickAudioCtx.state === "suspended") {
      clickAudioCtx.resume().catch(function () {
        /* ignore */
      });
    }
    return clickAudioCtx;
  }

  function clearSubdivisionClicks() {
    for (let i = 0; i < subdivisionTimeouts.length; i++) {
      clearTimeout(subdivisionTimeouts[i]);
    }
    subdivisionTimeouts = [];
  }

  function playWebClick(accent) {
    const ctx = ensureClickAudio();
    if (!ctx) {
      return;
    }
    const now = ctx.currentTime;
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();
    osc.type = "square";
    osc.frequency.value = accent ? 1200 : 800;
    gain.gain.setValueAtTime(accent ? 0.18 : 0.09, now);
    gain.gain.exponentialRampToValueAtTime(0.001, now + 0.04);
    osc.connect(gain);
    gain.connect(ctx.destination);
    osc.start(now);
    osc.stop(now + 0.045);
  }

  function scheduleCountInSubdivisionClicks(beatDurationMs) {
    clearSubdivisionClicks();
    if (!countInPhase) {
      return;
    }
    const perBeat = clicksPerQuarter(pendingMetronomeDivision);
    if (perBeat <= 1 || !(beatDurationMs > 0)) {
      return;
    }
    // Count-in event duration is musical beat length; convert to wall clock once.
    const speed = api && api.playbackSpeed > 0 ? Number(api.playbackSpeed) : 1;
    const stepMs = beatDurationMs / speed / perBeat;
    for (let i = 1; i < perBeat; i++) {
      const id = setTimeout(function () {
        playWebClick(false);
      }, stepMs * i);
      subdivisionTimeouts.push(id);
    }
  }

  function applyPendingMixer() {
    if (!api || !api.score || !playerReady) {
      return;
    }
    const tracks = api.score.tracks || [];
    for (let i = 0; i < tracks.length; i++) {
      const entry = pendingMixer[i] || { volume: 1, muted: false };
      const volume = Math.max(0, Math.min(1, Number(entry.volume)));
      const muted = !!entry.muted;
      if (typeof api.changeTrackVolume === "function") {
        api.changeTrackVolume([tracks[i]], Number.isFinite(volume) ? volume : 1);
      }
      if (typeof api.changeTrackMute === "function") {
        api.changeTrackMute([tracks[i]], muted);
      }
    }
  }

  function countInBeatCount() {
    try {
      const bars = api && api.score && api.score.masterBars;
      if (bars && bars.length > 0) {
        const n = Number(bars[0].timeSignatureNumerator);
        if (n > 0) {
          return n;
        }
      }
    } catch (e) {
      /* ignore */
    }
    return 4;
  }

  function readCursorFreezeSnapshot() {
    const bar = document.querySelector(".at-cursor-bar");
    const beat = document.querySelector(".at-cursor-beat");
    if (!bar && !beat) {
      return null;
    }
    function snap(el) {
      if (!el) {
        return null;
      }
      return {
        left: el.style.left,
        top: el.style.top,
        width: el.style.width,
        height: el.style.height,
        transform: el.style.transform,
      };
    }
    return { bar: snap(bar), beat: snap(beat) };
  }

  function applyCursorFreezeSnapshot(snap) {
    if (!snap) {
      return;
    }
    function apply(selector, style) {
      if (!style) {
        return;
      }
      const el = document.querySelector(selector);
      if (!el) {
        return;
      }
      el.style.left = style.left;
      el.style.top = style.top;
      el.style.width = style.width;
      el.style.height = style.height;
      el.style.transform = style.transform;
    }
    apply(".at-cursor-bar", snap.bar);
    apply(".at-cursor-beat", snap.beat);
  }

  function stopCountInCursorFreeze() {
    if (countInCursorRaf !== null) {
      cancelAnimationFrame(countInCursorRaf);
      countInCursorRaf = null;
    }
    if (countInCursorCaptureRaf !== null) {
      cancelAnimationFrame(countInCursorCaptureRaf);
      countInCursorCaptureRaf = null;
    }
    countInCursorFreeze = null;
    countInPendingRestartPlay = false;
    try {
      document.documentElement.classList.remove("sp-count-in");
    } catch (e) {
      /* ignore */
    }
  }

  function countInCursorFreezeTick() {
    countInCursorRaf = null;
    if (!countInPhase) {
      return;
    }
    if (countInCursorFreeze) {
      applyCursorFreezeSnapshot(countInCursorFreeze);
    }
    countInCursorRaf = requestAnimationFrame(countInCursorFreezeTick);
  }

  function armCountInCursorFreeze(snap) {
    if (!countInPhase || !snap) {
      return;
    }
    countInCursorFreeze = snap;
    if (countInCursorRaf === null) {
      countInCursorRaf = requestAnimationFrame(countInCursorFreezeTick);
    }
  }

  function finishPendingCountInRestartPlay() {
    if (!countInPendingRestartPlay || !api) {
      return;
    }
    countInPendingRestartPlay = false;
    if (!wantPlayback) {
      loopRestartInProgress = false;
      return;
    }
    api.play();
    setTimeout(function () {
      loopRestartInProgress = false;
    }, 50);
  }

  /**
   * AlphaTab maps count-in MIDI ticks onto the score cursor, so it walks through
   * the first bar while counting in. Pin the cursor DOM at the pre-count-in spot
   * (loop/song start) until the count-in ends.
   */
  function startCountInCursorFreeze() {
    // Keep class; restart path arms freeze after seek — don't steal an end-bar snapshot.
    if (countInCursorRaf !== null) {
      cancelAnimationFrame(countInCursorRaf);
      countInCursorRaf = null;
    }
    if (countInCursorCaptureRaf !== null) {
      cancelAnimationFrame(countInCursorCaptureRaf);
      countInCursorCaptureRaf = null;
    }
    countInCursorFreeze = null;
    try {
      document.documentElement.classList.add("sp-count-in");
    } catch (e) {
      /* ignore */
    }
    if (countInPendingRestartPlay) {
      return;
    }
    let attempts = 0;
    function tryCapture() {
      countInCursorCaptureRaf = null;
      if (!countInPhase || countInPendingRestartPlay) {
        return;
      }
      attempts += 1;
      const snap = readCursorFreezeSnapshot();
      const hasPos =
        snap &&
        ((snap.beat && String(snap.beat.left).length > 0) ||
          (snap.bar && String(snap.bar.left).length > 0));
      if (hasPos) {
        armCountInCursorFreeze(snap);
        return;
      }
      if (attempts < 12) {
        countInCursorCaptureRaf = requestAnimationFrame(tryCapture);
      }
    }
    countInCursorCaptureRaf = requestAnimationFrame(tryCapture);
  }

  function beginCountInPhaseIfNeeded() {
    if (pendingCountInEnabled) {
      countInPhase = true;
      countInBeatsPlayed = 0;
      startCountInCursorFreeze();
    } else {
      endCountInPhase();
    }
  }

  function endCountInPhase() {
    countInPhase = false;
    countInBeatsPlayed = 0;
    stopCountInCursorFreeze();
  }

  function finishCountInAndMaybeStartMetronome() {
    clearSubdivisionClicks();
    endCountInPhase();
    markRangePassAnchor();
    if (pendingMetronomeEnabled && isPlayerPlaying()) {
      // Last count-in beat already clicked — first metro "1" is the next downbeat.
      armMetronomeStartOnNextDownbeat();
    }
  }

  function applyPendingMetronome() {
    if (!api) {
      return;
    }
    // Mute AlphaTab's ongoing metronome — we synthesize clicks (accent on 1).
    api.metronomeVolume = 0;
    // countInVolume must be > 0 so AlphaSynth runs startCountIn() before the score.
    api.countInVolume = pendingCountInEnabled ? 0.0001 : 0;
    if (!pendingMetronomeEnabled) {
      stopMetronomeClock();
    }
    if (!pendingMetronomeEnabled && !pendingCountInEnabled) {
      clearSubdivisionClicks();
      endCountInPhase();
      return;
    }
    if (pendingMetronomeEnabled && isPlayerPlaying() && !countInPhase && !metroClockActive &&
        !pendingMetroStartOnNextDownbeat && !pendingCountInEnabled) {
      startMetronomeClock(false);
    }
  }

  function clampTransposeSemitones(raw) {
    const n = Math.round(Number(raw) || 0);
    return Math.max(-12, Math.min(12, n));
  }

  function isMelodicTrack(track) {
    if (!track || !track.playbackInfo) {
      return false;
    }
    if (track.isPercussion) {
      return false;
    }
    if (track.playbackInfo.primaryChannel === PERCUSSION_CHANNEL) {
      return false;
    }
    return true;
  }

  function melodicTracks(tracks) {
    const result = [];
    for (let i = 0; i < tracks.length; i++) {
      if (isMelodicTrack(tracks[i])) {
        result.push(tracks[i]);
      }
    }
    return result;
  }

  function applyPendingTranspose() {
    if (!api || !api.score || !playerReady) {
      return;
    }
    const semitones = clampTransposeSemitones(pendingTransposeSemitones);
    if (semitones === appliedTransposeSemitones) {
      return;
    }

    const player = api.player;
    if (!player || typeof player.setChannelTranspositionPitch !== "function") {
      return;
    }

    const tracks = melodicTracks(api.score.tracks || []);
    for (let i = 0; i < tracks.length; i++) {
      const channel = tracks[i].playbackInfo.primaryChannel;
      if (channel === PERCUSSION_CHANNEL) {
        continue;
      }
      player.setChannelTranspositionPitch(channel, semitones);
    }
    appliedTransposeSemitones = semitones;
  }

  function applyAllPendingSettings() {
    if (applyingSettings) {
      return;
    }
    applyingSettings = true;
    try {
      applyTrackVisibility();
      applyPendingTempo();
      applyPendingLoop();
      applyPendingMixer();
      applyPendingMetronome();
      applyPendingTranspose();
    } finally {
      applyingSettings = false;
    }
  }

  function onEvent(emitter, handler) {
    if (emitter && typeof emitter.on === "function") {
      emitter.on(handler);
    }
  }

  function eventMessage(e, fallback) {
    if (!e) {
      return fallback;
    }
    if (typeof e === "string") {
      return e;
    }
    if (e.message && String(e.message).length > 0) {
      return String(e.message);
    }
    if (e.error && e.error.message) {
      return String(e.error.message);
    }
    const text = String(e);
    if (text.length > 0 && text !== "[object Object]") {
      return text;
    }
    return fallback;
  }

  function isBuiltInSoundFontKey(key) {
    return key === BUILTIN_SOUND_FONT;
  }

  function notifySoundFontLoaded() {
    setStatus("");
    if (scoreReady && playerReady) {
      applyAllPendingSettings();
    }
    notifyNative("soundFontLoaded", {
      builtIn: isBuiltInSoundFontKey(activeSoundFontKey),
    });
  }

  function notifySoundFontFailed(message, fallbackToBuiltin) {
    loadingSoundFontKey = null;
    if (fallbackToBuiltin) {
      useCustomSoundFont = false;
      if (!isBuiltInSoundFontKey(activeSoundFontKey)) {
        activeSoundFontKey = null;
      }
      notifyNative("soundFontLoadFailed", {
        message: message + " — using built-in soundfont",
      });
      loadBuiltinSoundFont();
      return;
    }
    activeSoundFontKey = null;
    setStatus(message, true);
    notifyNative("soundFontLoadFailed", { message: message });
  }

  function loadBuiltinSoundFont() {
    useCustomSoundFont = false;
    initApi();
    if (!api) {
      return;
    }
    loadingSoundFontKey = BUILTIN_SOUND_FONT;
    if (!api.loadSoundFont(BUILTIN_SOUND_FONT, false)) {
      notifySoundFontFailed("Built-in SoundFont could not be loaded", false);
    }
  }

  function loadCustomSoundFontFromUrl(url) {
    useCustomSoundFont = true;
    initApi();
    if (!api) {
      return;
    }
    const href = String(url || "");
    if (!href) {
      notifySoundFontFailed("SoundFont URL is empty", true);
      return;
    }
    loadingSoundFontKey = CUSTOM_SOUND_FONT_KEY;
    setStatus("Loading soundfont…");
    // Fetch from the Qt scheme handler (bank preloaded in C++ at app start).
    fetch(href)
      .then(function (response) {
        if (!response.ok) {
          throw new Error("SoundFont HTTP " + response.status);
        }
        return response.arrayBuffer();
      })
      .then(function (bytes) {
        if (!api) {
          throw new Error("Player is not initialized");
        }
        if (!bytes || bytes.byteLength === 0) {
          throw new Error("SoundFont file is empty");
        }
        if (typeof api.resetSoundFonts === "function") {
          api.resetSoundFonts();
        }
        if (!api.loadSoundFont(bytes, false)) {
          throw new Error("Unsupported SoundFont format");
        }
      })
      .catch(function (e) {
        notifySoundFontFailed(eventMessage(e, "SoundFont load failed"), true);
      });
  }

  function beginSoundFontBytesLoad() {
    useCustomSoundFont = true;
    initApi();
    if (!api) {
      return;
    }
    soundFontBytesBuffers = [];
    loadingSoundFontKey = CUSTOM_SOUND_FONT_KEY;
  }

  function appendSoundFontBytesChunk(base64) {
    if (!soundFontBytesBuffers) {
      return;
    }
    soundFontBytesBuffers.push(base64ToArrayBuffer(base64));
  }

  function finishSoundFontBytesLoad() {
    if (!soundFontBytesBuffers) {
      notifySoundFontFailed("SoundFont bytes were not received", true);
      return;
    }
    const parts = soundFontBytesBuffers;
    soundFontBytesBuffers = null;
    try {
      if (!api) {
        throw new Error("Player is not initialized");
      }
      let totalLength = 0;
      for (let i = 0; i < parts.length; i++) {
        totalLength += parts[i].byteLength;
      }
      if (totalLength === 0) {
        throw new Error("SoundFont file is empty");
      }
      const bytes = new Uint8Array(totalLength);
      let offset = 0;
      for (let i = 0; i < parts.length; i++) {
        bytes.set(new Uint8Array(parts[i]), offset);
        offset += parts[i].byteLength;
      }
      if (typeof api.resetSoundFonts === "function") {
        api.resetSoundFonts();
      }
      if (!api.loadSoundFont(bytes, false)) {
        throw new Error("Unsupported SoundFont format");
      }
    } catch (e) {
      notifySoundFontFailed(eventMessage(e, "SoundFont load failed"), true);
    }
  }

  function wireMetronomeEvents(instance) {
    const MidiEventType =
      alphaTab.midi && alphaTab.midi.MidiEventType ? alphaTab.midi.MidiEventType : null;
    if (MidiEventType && MidiEventType.AlphaTabMetronome !== undefined) {
      instance.midiEventsPlayedFilter = [MidiEventType.AlphaTabMetronome];
    }
    onEvent(instance.midiEventsPlayed, function (e) {
      if ((!pendingMetronomeEnabled && !pendingCountInEnabled && !countInPhase) || !e ||
          !e.events) {
        return;
      }
      for (let i = 0; i < e.events.length; i++) {
        const midi = e.events[i];
        if (!(midi && midi.isMetronome)) {
          continue;
        }
        // AlphaTab uses 0-based beat index within the bar (0 = downbeat / "1").
        const beatInBar = Number(midi.metronomeNumerator);
        if (countInPhase) {
          playWebClick(beatInBar === 0);
          scheduleCountInSubdivisionClicks(midi.metronomeDurationInMilliseconds || 0);
          countInBeatsPlayed += 1;
          if (countInBeatsPlayed >= countInBeatCount()) {
            finishCountInAndMaybeStartMetronome();
          }
        } else if (pendingMetronomeEnabled && beatInBar === 0) {
          if (pendingMetroStartOnNextDownbeat) {
            // Musical downbeat after count-in — sync metro "1" here.
            startMetronomeClock(true);
          } else {
            // Resync accent to bar starts; the BPM clock drives the actual click rate.
            metroClickIndex = 0;
          }
        }
      }
    });
    onEvent(instance.playerPositionChanged, function (pos) {
      if (!pos) {
        return;
      }
      const bpm = Number(pos.originalTempo || pos.currentTempo || 0);
      if (bpm > 0) {
        liveBpm = bpm;
      }
      const tick = Number(pos.currentTick);
      if (Number.isFinite(tick) && tick > lastPlaybackTick) {
        lastPlaybackTick = tick;
      }
      // Soft-catch the musical bar end before AlphaTab stop()/noteOffAll hard-cuts.
      if (
        wantPlayback &&
        pendingLoop.enabled &&
        !countInPhase &&
        !softLoopEnding &&
        !loopRestartInProgress &&
        Number.isFinite(tick) &&
        tick >= endTickForBar(pendingLoop.startBar, pendingLoop.endBar) - 1
      ) {
        beginSoftLoopEnd();
      }
    });
  }

  function initApi() {
    if (api) {
      return;
    }
    if (typeof alphaTab === "undefined") {
      setStatus("alphaTab failed to load", true);
      notifyNative("error", { message: "alphaTab failed to load" });
      return;
    }

    applyPageTheme(darkTheme);

    const playerOptions = {
      playerMode: alphaTab.PlayerMode.EnabledSynthesizer,
      enablePlayer: true,
      enableCursor: true,
      enableAnimatedBeatCursor: true,
      enableElementHighlighting: true,
      scrollMode: alphaTab.ScrollMode.Continuous,
      scrollElement: scrollEl,
    };
    // Omit built-in when a custom font will be loaded (loadScore sets useCustomSoundFont
    // before initApi) so Play is not unlocked early by sonivox.
    if (!useCustomSoundFont) {
      playerOptions.soundFont = BUILTIN_SOUND_FONT;
    }

    const instance = new alphaTab.AlphaTabApi(hostEl, {
      core: {
        engine: "html5",
        fontDirectory: "./font/",
        logLevel: alphaTab.LogLevel.Warning,
        // Workers serialize Colors; the watermark keeps a stale black colorOverride.
        useWorkers: false,
        enableLazyLoading: false,
      },
      display: {
        layoutMode: alphaTab.LayoutMode.Page,
        scale: 1.0,
        resources: notationResources(darkTheme),
      },
      notation: {
        rhythmMode: alphaTab.TabRhythmMode.ShowWithBeams,
      },
      player: playerOptions,
    });

    api = instance;
    // Apply theme Colors before any score layout bakes colorOverride (watermark).
    applyNotationTheme(darkTheme, false);
    installCanvasColorReuseFix(instance);

    // AlphaTab 1.8 exposes soundFontLoaded + error, not soundFontLoadFailed on the API.
    onEvent(instance.error, function (e) {
      const message = eventMessage(e, "AlphaTab error");
      if (message === "Error") {
        return;
      }
      setStatus(message, true);
      notifyNative("error", { message: message });
    });
    onEvent(instance.soundFontLoadFailed, function (e) {
      if (loadingSoundFontKey && isBuiltInSoundFontKey(loadingSoundFontKey)) {
        notifySoundFontFailed(eventMessage(e, "SoundFont load failed"), false);
        return;
      }
      notifySoundFontFailed(
        eventMessage(e, "SoundFont load failed") + " — using built-in soundfont",
        true
      );
    });
    onEvent(instance.soundFontLoaded, function () {
      if (loadingSoundFontKey) {
        activeSoundFontKey = loadingSoundFontKey;
        loadingSoundFontKey = null;
      } else {
        activeSoundFontKey = BUILTIN_SOUND_FONT;
      }
      notifySoundFontLoaded();
    });

    onEvent(instance.scoreLoaded, function () {
      scoreReady = true;
      // Replace GP-embedded blacks + apply theme; renderTracks follows in applyAllPendingSettings.
      applyNotationTheme(darkTheme, false);
      installCanvasColorReuseFix(instance);
      try {
        const tempo = Number(instance.score && instance.score.tempo);
        if (tempo > 0) {
          liveBpm = tempo;
        }
      } catch (e) {
        /* ignore */
      }
      applyAllPendingSettings();
      const names = (instance.score.tracks || []).map(function (t, i) {
        return t.name || ("Track " + (i + 1));
      });
      notifyNative("scoreLoaded", {
        title: instance.score.title || "",
        artist: instance.score.artist || "",
        trackNames: names,
        barCount: (instance.score.masterBars && instance.score.masterBars.length) || 0,
      });
      setStatus("");
    });

    onEvent(instance.postRenderFinished, function () {
      installCanvasColorReuseFix(instance);
      if (pendingLoop.enabled) {
        highlightLoopBars(pendingLoop.startBar, pendingLoop.endBar);
      }
    });

    onEvent(instance.playerReady, function () {
      playerReady = true;
      applyAllPendingSettings();
      notifyNative("playerReady", {});
    });

    onEvent(instance.playerStateChanged, function (args) {
      const Playing =
        alphaTab.synth && alphaTab.synth.PlayerState
          ? alphaTab.synth.PlayerState.Playing
          : 1;
      const playing = args && args.state === Playing;
      if (!playing) {
        // Prefer keeping the clock if a loop rest-hold is imminent or active.
        const expectRestHold =
          wantPlayback && pendingLoop.enabled && !loopRestartInProgress;
        if (!restHoldActive && !expectRestHold) {
          stopMetronomeClock();
          clearSubdivisionClicks();
        }
      } else {
        ensureClickAudio();
        // Musical range starts when we enter Playing outside count-in.
        // Only anchor once per pass — re-entry must not reset the clock.
        if (!countInPhase && pendingLoop.enabled && rangePassAnchorPerfMs <= 0) {
          markRangePassAnchor();
        }
        if (pendingMetronomeEnabled && !countInPhase) {
          // With count-in, metro starts only via arm→first musical downbeat.
          // playerStateChanged also fires when count-in hands off to the score and
          // used to startMetronomeClock again → double click + phase drift each loop.
          if (!pendingCountInEnabled && !metroClockActive && !pendingMetroStartOnNextDownbeat) {
            startMetronomeClock(true);
          }
        }
      }
      if ((loopRestartInProgress || restHoldActive || softLoopEnding) && !playing) {
        return;
      }
      notifyNative("playerState", { playing: !!playing });
    });

    onEvent(instance.playerFinished, function () {
      if (loopRestartInProgress || softLoopEnding) {
        return;
      }
      clearSubdivisionClicks();
      endCountInPhase();
      // Fallback if positionChanged missed the bar line.
      if (wantPlayback && pendingLoop.enabled) {
        beginSoftLoopEnd();
        return;
      }
      clearLoopRestartTimer();
      stopMetronomeClock();
      wantPlayback = false;
      notifyNative("playerState", { playing: false });
    });

    wireMetronomeEvents(instance);
  }

  function assimilateMixer(mixer) {
    if (!Array.isArray(mixer)) {
      return;
    }
    pendingMixer = mixer.map(function (entry) {
      const volume = Math.max(0, Math.min(1, Number(entry && entry.volume)));
      return {
        volume: Number.isFinite(volume) ? volume : 1,
        muted: !!(entry && entry.muted),
      };
    });
  }

  function assimilateOptions(options) {
    if (!options || typeof options !== "object") {
      return;
    }
    if (options.trackIndex !== undefined && options.trackIndex !== null) {
      selectedTrackIndex = Math.max(0, Number(options.trackIndex) || 0);
    }
    if (options.tempoPercent !== undefined && options.tempoPercent !== null) {
      pendingTempoPercent = Math.max(25, Math.min(200, Number(options.tempoPercent) || 100));
    }
    if (options.loopEnabled !== undefined) {
      pendingLoop.enabled = !!options.loopEnabled;
    }
    if (options.loopStartBar !== undefined && options.loopStartBar !== null) {
      pendingLoop.startBar = Math.max(1, Number(options.loopStartBar) || 1);
    }
    if (options.loopEndBar !== undefined && options.loopEndBar !== null) {
      pendingLoop.endBar = Math.max(
        pendingLoop.startBar,
        Number(options.loopEndBar) || pendingLoop.startBar
      );
    }
    if (options.mixer !== undefined) {
      assimilateMixer(options.mixer);
    }
    if (options.metronomeEnabled !== undefined) {
      pendingMetronomeEnabled = !!options.metronomeEnabled;
    }
    if (options.metronomeDivision !== undefined && options.metronomeDivision !== null) {
      pendingMetronomeDivision = normalizeDivision(options.metronomeDivision);
    }
    if (options.countInEnabled !== undefined) {
      pendingCountInEnabled = !!options.countInEnabled;
    }
    if (options.transposeSemitones !== undefined && options.transposeSemitones !== null) {
      pendingTransposeSemitones = clampTransposeSemitones(options.transposeSemitones);
    }
    if (options.soundFontUseCustom !== undefined) {
      useCustomSoundFont = !!options.soundFontUseCustom;
    }
  }

  window.sonarAlphaTab = {
    setTheme: function (isDark) {
      darkTheme = !!isDark;
      applyPageTheme(darkTheme);
      applyNotationTheme(darkTheme, true);
    },

    loadScoreBase64: function (base64, options) {
      try {
        assimilateOptions(options);
        initApi();
        if (!api) {
          return;
        }
        wantPlayback = false;
        setStatus("Loading score…");
        scoreReady = false;
        playerReady = false;
        appliedTransposeSemitones = 0;
        clearSubdivisionClicks();
        clearLoopRestartTimer();
        stopMetronomeClock();
        endCountInPhase();
        api.load(base64ToArrayBuffer(base64));
      } catch (e) {
        setStatus(String(e && e.message ? e.message : e), true);
        notifyNative("error", { message: String(e) });
      }
    },

    ensureInitialized: function () {
      initApi();
    },

    play: function () {
      if (api) {
        wantPlayback = true;
        rangePassAnchorPerfMs = 0;
        rangePassAnchorTick = 0;
        ensureClickAudio();
        applyPendingMetronome();
        beginCountInPhaseIfNeeded();
        api.play();
      }
    },
    pause: function () {
      wantPlayback = false;
      softLoopEnding = false;
      clearLoopRestartTimer();
      stopMetronomeClock();
      clearSubdivisionClicks();
      endCountInPhase();
      if (api) {
        api.pause();
      }
    },
    stop: function () {
      wantPlayback = false;
      softLoopEnding = false;
      clearLoopRestartTimer();
      stopMetronomeClock();
      clearSubdivisionClicks();
      endCountInPhase();
      if (api) {
        api.stop();
      }
    },
    playPause: function () {
      if (api) {
        ensureClickAudio();
        const Playing =
          alphaTab.synth && alphaTab.synth.PlayerState
            ? alphaTab.synth.PlayerState.Playing
            : 1;
        const starting = api.playerState !== Playing && !restHoldActive && !softLoopEnding;
        applyPendingMetronome();
        if (starting) {
          wantPlayback = true;
          softLoopEnding = false;
          clearLoopRestartTimer();
          rangePassAnchorPerfMs = 0;
          rangePassAnchorTick = 0;
          beginCountInPhaseIfNeeded();
        } else {
          wantPlayback = false;
          softLoopEnding = false;
          clearLoopRestartTimer();
          stopMetronomeClock();
          endCountInPhase();
          clearSubdivisionClicks();
        }
        api.playPause();
      }
    },

    setTempoPercent: function (percent) {
      pendingTempoPercent = Math.max(25, Math.min(200, Number(percent) || 100));
      applyPendingTempo();
      if (pendingMetronomeEnabled && isPlayerPlaying() && !countInPhase) {
        if (metroClockActive) {
          // Retarget interval only — do not click again.
          scheduleNextMetronomeClockTick();
        } else if (!pendingCountInEnabled && !pendingMetroStartOnNextDownbeat) {
          startMetronomeClock(false);
        }
      }
    },

    setTrackIndex: function (index) {
      const raw = Number(index);
      selectedTrackIndex = Math.max(0, Number.isFinite(raw) ? raw : 0);
      applyTrackVisibility();
    },

    setLoop: function (enabled, startBar, endBar) {
      pendingLoop.enabled = !!enabled;
      pendingLoop.startBar = Math.max(1, Number(startBar) || 1);
      pendingLoop.endBar = Math.max(pendingLoop.startBar, Number(endBar) || pendingLoop.startBar);
      applyPendingLoop();
    },

    setMixer: function (tracks) {
      assimilateMixer(tracks);
      applyPendingMixer();
    },

    setMetronome: function (enabled, division) {
      pendingMetronomeEnabled = !!enabled;
      if (division !== undefined && division !== null) {
        pendingMetronomeDivision = normalizeDivision(division);
      }
      applyPendingMetronome();
    },

    setCountIn: function (enabled) {
      pendingCountInEnabled = !!enabled;
      applyPendingMetronome();
    },

    setTranspose: function (semitones) {
      pendingTransposeSemitones = clampTransposeSemitones(semitones);
      applyPendingTranspose();
    },

    applySettings: function (options) {
      assimilateOptions(options);
      if (scoreReady) {
        applyAllPendingSettings();
      }
    },

    loadBuiltInSoundFont: function () {
      activeSoundFontKey = null;
      loadBuiltinSoundFont();
    },

    loadCustomSoundFontFromUrl: loadCustomSoundFontFromUrl,

    beginSoundFontBytesLoad: beginSoundFontBytesLoad,
    appendSoundFontBytesChunk: appendSoundFontBytesChunk,
    finishSoundFontBytesLoad: finishSoundFontBytesLoad,

    isReady: function () {
      return !!(scoreReady && playerReady);
    },
  };

  applyPageTheme(darkTheme);
  setStatus("Player ready — waiting for score");
  notifyNative("bridgeReady", {});
})();
