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
  const MIN_TEMPO_PERCENT = 25;
  const MAX_TEMPO_PERCENT = 300;
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
  let pendingCountInOneTime = false;
  let metronomeVolume = 1;
  let metronomeMuted = false;
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
  let countInTimerId = null;
  let metroClickIndex = 0;
  /** Clicks in the silent count-in bar. 0 when that bar is not running. */
  let metroCountInClicks = 0;
  /** True while the wall-clock metro is supposed to be running (survives timer gaps). */
  let metroClockActive = false;
  /**
   * Count-in told the UI to resume before the downbeat. Pause events in this
   * window must not kill the click clock — a new clock beside it flams.
   */
  let clickClockHoldUntilMs = 0;
  /** True once the count-in bar has asked the UI to start on its next "1". */
  let countInHandoffSent = false;
  /** Audio-clock time of the "1" that starts the score after a count-in. */
  let pendingDownbeatTime = 0;
  let playOnDownbeatTimerId = null;
  /** Oscillators already handed to the audio clock, so a stop can drop future clicks. */
  let scheduledClicks = [];
  /**
   * After count-in, wait for the first musical downbeat before the metro clock
   * clicks — starting on the last count-in beat shifts "1" earlier each loop.
   */
  let pendingMetroStartOnNextDownbeat = false;
  let metroStartFallbackId = null;
  let pendingTransposeSemitones = 0;
  let appliedTransposeSemitones = 0;
  /** Extra speed while a sync segment does not match the audio one-to-one. */
  let syncSpeedRatio = 1;
  let lastScoreNotifyMs = 0;
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

  /**
   * Length of one written bar in source-audio milliseconds (file tempo, not the
   * slowed practice tempo). Sync markers are stored in source-file time.
   */
  function barSourceMs(barNumber) {
    const span = linearBarTicks(barNumber);
    const ticks = Math.max(0, (Number(span.endTick) || 0) - (Number(span.startTick) || 0));
    if (ticks <= 0) {
      return 0;
    }
    let bpm = 120;
    try {
      const tempo = api && api.score ? Number(api.score.tempo) : 0;
      if (tempo > 0) {
        bpm = tempo;
      }
    } catch (e) {
      /* ignore */
    }
    const quarter = quarterTickResolution();
    return Math.max(0, Math.round((ticks / quarter) * (60000 / bpm)));
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
    const p = Math.max(MIN_TEMPO_PERCENT, Math.min(MAX_TEMPO_PERCENT, Number(pendingTempoPercent) || 100));
    const ratio = syncSpeedRatio > 0 ? syncSpeedRatio : 1;
    api.playbackSpeed = (p / 100.0) * ratio;
  }

  /** 1-based bar and beat, plus the playback tick, for a score tick. */
  function scorePositionFromTick(tick) {
    const t = Math.max(0, Number(tick) || 0);
    const result = { bar: 1, beat: 1, tick: Math.round(t) };
    if (!api || !api.score || !api.score.masterBars || api.score.masterBars.length === 0) {
      return result;
    }
    const masterBars = api.score.masterBars;
    let barIndex = 0;
    for (let i = 0; i < masterBars.length; i++) {
      const start = Number(masterBars[i].start) || 0;
      if (start <= t) {
        barIndex = i;
      } else {
        break;
      }
    }
    result.bar = barIndex + 1;
    const tracks = api.score.tracks || [];
    const track =
      selectedTrackIndex >= 0 && selectedTrackIndex < tracks.length
        ? tracks[selectedTrackIndex]
        : tracks[0];
    const beats =
      track &&
      track.staves &&
      track.staves[0] &&
      track.staves[0].bars &&
      track.staves[0].bars[barIndex] &&
      track.staves[0].bars[barIndex].voices &&
      track.staves[0].bars[barIndex].voices[0] &&
      track.staves[0].bars[barIndex].voices[0].beats;
    if (beats && beats.length > 0) {
      let beat = 1;
      for (let i = 0; i < beats.length; i++) {
        const start = Number(beats[i].playbackStart);
        if (Number.isFinite(start) && start <= t) {
          beat = i + 1;
        }
      }
      result.beat = beat;
    }
    return result;
  }

  function publishScorePosition(force) {
    const now = nowMs();
    if (!force && now - lastScoreNotifyMs < 80) {
      return;
    }
    lastScoreNotifyMs = now;
    notifyNative("scorePosition", scorePositionFromTick(lastPlaybackTick));
  }

  /**
   * AlphaTab's range highlight reads beat.realBounds immediately. Before the
   * score has been laid out — and for beats that are not drawn — that object
   * is missing and the setter throws.
   */
  function beatHasRealBounds(beat) {
    if (!beat || !api || !api.boundsLookup || typeof api.boundsLookup.findBeat !== "function") {
      return false;
    }
    try {
      const bounds = api.boundsLookup.findBeat(beat);
      return !!(bounds && bounds.realBounds);
    } catch (e) {
      return false;
    }
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

  /**
   * Setting playbackRange also paints the selection. That paint throws
   * "reading 'realBounds'" when the bar has not been laid out yet.
   * Practice applies the loop twice (score loaded, then player ready).
   */
  function assignPlaybackRange(range) {
    if (!api) {
      return;
    }
    try {
      api.playbackRange = range;
    } catch (e) {
      /* range is stored before the highlight paint; ignore the paint failure */
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
    // Calling highlightPlaybackRange stores the beats even when the paint throws.
    // The next render then paints them again and reports the same error.
    if (!beatHasRealBounds(startBeat) || !beatHasRealBounds(endBeat)) {
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
      assignPlaybackRange(null);
      clearLoopHighlight();
      return;
    }
    const start = Math.max(1, Number(pendingLoop.startBar) || 1);
    const end = Math.max(start, Number(pendingLoop.endBar) || start);
    const range = loopPlaybackTicks(start, end);
    const startTick = range.startTick;
    const musicalEndTick = range.endTick;
    // Tiny pad past the bar line so we can jump before AlphaTab's stop()/noteOffAll.
    const rangeEndTick = musicalEndTick + Math.max(1, Math.round(quarterTickResolution() / 8));
    const existing = api.playbackRange;
    const sameRange =
      existing &&
      Number(existing.startTick) === startTick &&
      Number(existing.endTick) === rangeEndTick;

    // Assigning playbackRange seeks to startTick — avoid that while playing/holding.
    const holdTick =
      (isPlayerPlaying() || restHoldActive || softLoopEnding) && lastPlaybackTick > startTick
        ? lastPlaybackTick
        : 0;
    if (!sameRange) {
      assignPlaybackRange({
        startTick: startTick,
        endTick: rangeEndTick,
      });
    }
    if (holdTick > 0) {
      flushSynthQueueAt(holdTick);
    } else if (!isPlayerPlaying() && !restHoldActive && !softLoopEnding && !countInPhase) {
      // Rendering parks the cursor on bar 1. The range is often already stored,
      // so seek again whenever the player is idle.
      flushSynthQueueAt(startTick);
    }
    // Never leave AlphaTab auto-loop on. Its finished event stops the cursor,
    // and a manual jump keeps the metronome on the same grid.
    api.isLooping = false;
    highlightLoopBars(start, end);
  }

  let loopResumeTick = -1;

  /** Seeking clears AlphaTab's already-rendered sample queue. */
  function flushSynthQueueAt(tick) {
    if (!api) {
      return;
    }
    const target = Math.max(0, Number(tick) || 0);
    try {
      api.tickPosition = target;
    } catch (e) {
      /* ignore */
    }
    lastPlaybackTick = target;
  }

  /** Tick of the configured practice bar, or -1 when no loop is active. */
  function loopStartTick() {
    if (!pendingLoop.enabled) {
      return -1;
    }
    const tick = tickForBar(pendingLoop.startBar);
    return Number.isFinite(tick) ? Math.max(0, tick) : -1;
  }

  /**
   * AlphaTab's tempo change seeks with a scaled time position. While stopped that
   * time is 0, so the cursor and the next Play land on bar 1.
   * Put both back on the practice bar after AlphaTab's own cursor update.
   */
  function placeCursorAtPracticeBar() {
    const tick = loopStartTick();
    if (tick < 0 || !api || countInPhase || isPlayerPlaying() || restHoldActive || softLoopEnding) {
      return;
    }
    flushSynthQueueAt(tick);
    requestAnimationFrame(function () {
      if (!api || countInPhase || isPlayerPlaying() || restHoldActive || softLoopEnding) {
        return;
      }
      flushSynthQueueAt(tick);
    });
  }

  function setNotationInteractive(enabled) {
    if (!hostEl) {
      return;
    }
    hostEl.style.pointerEvents = enabled ? "" : "none";
  }

  /** Clicks on the tab seek the cursor. While Play is running that leaves the audio behind. */
  function syncNotationInteractive() {
    const enabled = !(wantPlayback || countInPhase || isPlayerPlaying());
    setNotationInteractive(enabled);
    if (api && api.settings && api.settings.player) {
      api.settings.player.enableUserInteraction = enabled;
    }
  }

  function clearPlayOnDownbeatTimer() {
    if (playOnDownbeatTimerId !== null) {
      clearTimeout(playOnDownbeatTimerId);
      playOnDownbeatTimerId = null;
    }
  }

  function holdClickClock(extraMs) {
    const until = nowMs() + Math.max(0, extraMs);
    if (until > clickClockHoldUntilMs) {
      clickClockHoldUntilMs = until;
    }
  }

  /**
   * How early the count-in asks the UI to resume. The WebChannel round-trip
   * was landing play() after the downbeat click and starting a second grid.
   */
  function countInHandoffLeadSec() {
    const interval = metroInterval > 0 ? metroInterval : metronomeIntervalSec();
    // Long enough for the WebChannel round-trip (~100ms), short of half a beat.
    return Math.min(0.16, Math.max(0.08, interval * 0.45));
  }

  function playScoreNow() {
    // Ignore Play during the count-in bar itself. Once the handoff is out,
    // Play is the resume and must land on the same "1" as the click.
    if (!api || (countInPhase && !countInHandoffSent)) {
      return;
    }
    wantPlayback = true;
    syncNotationInteractive();
    ensureClickAudio();
    applyPendingMetronome();

    const startScore = function () {
      if (!wantPlayback || !api) {
        return;
      }
      rangePassAnchorPerfMs = 0;
      rangePassAnchorTick = 0;
      // A loop wrap must resume at the loop head. The live tick is often still
      // the last note of the previous pass when the count-in ends.
      if (loopResumeTick >= 0) {
        flushSynthQueueAt(loopResumeTick);
        loopResumeTick = -1;
      } else {
        let tick = 0;
        try {
          tick = Number(api.tickPosition) || 0;
        } catch (e) {
          tick = 0;
        }
        const practiceTick = loopStartTick();
        if (practiceTick > 0 && tick < practiceTick) {
          tick = practiceTick;
        }
        flushSynthQueueAt(tick);
      }
      // Cover pause events that api.play() flushes from the earlier stop().
      holdClickClock(500);
      api.play();
      // The count-in clock already clicked this "1". A new origin here is the flam.
      if (!metroClockActive) {
        armMetronomeForScoreStart();
      }
      pendingDownbeatTime = 0;
      countInHandoffSent = false;
    };

    clearPlayOnDownbeatTimer();
    const ctx = ensureClickAudio();
    if (ctx && pendingDownbeatTime > ctx.currentTime + 0.02) {
      const waitMs = (pendingDownbeatTime - ctx.currentTime) * 1000;
      playOnDownbeatTimerId = setTimeout(function () {
        playOnDownbeatTimerId = null;
        startScore();
      }, waitMs);
      return;
    }
    startScore();
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
    clearSubdivisionClicks();
    endCountInPhase();
    ensureClickAudio();
    applyPendingMetronome();
    const startTick = pendingLoop.enabled ? tickForBar(pendingLoop.startBar) : 0;
    const bar = pendingLoop.enabled ? pendingLoop.startBar : 1;
    rangePassAnchorPerfMs = 0;
    rangePassAnchorTick = startTick;
    loopResumeTick = startTick;
    const countInOnWrap = pendingCountInEnabled && !pendingCountInOneTime;
    if (countInOnWrap) {
      // stop() cuts the sounding note and parks on the loop range. The live tick
      // would otherwise stay on the last note, and Play would repeat only that.
      try {
        api.stop();
      } catch (e) {
        /* ignore */
      }
      flushSynthQueueAt(startTick);
      stopMetronomeClock();
      startStandaloneCountIn(function () {
        if (!wantPlayback) {
          loopRestartInProgress = false;
          softLoopEnding = false;
          return;
        }
        // Audio and the score resume together from QML. Starting the synth
        // here made MIDI lead the backing by one message round-trip.
        notifyNative("countInFinished", { loop: true, bar: bar, tick: startTick });
      });
    } else {
      // Stay in Playing. stop() was the hole: the metronome "1" fell into it,
      // and the following Play came back late, so the riff started off the click.
      const playing = isPlayerPlaying();
      flushSynthQueueAt(startTick);
      if (!playing) {
        holdClickClock(500);
        try {
          api.play();
        } catch (e) {
          /* ignore */
        }
      }
      loopResumeTick = -1;
      notifyNative("loopWrapped", { bar: bar, tick: startTick, seamless: true });
    }
    publishScorePosition(true);

    // Stay latched through the count-in. A short timeout used to drop the
    // guard while the last-bar tick was still in flight, and Play resumed there.
    const countInMs = pendingCountInEnabled && !pendingCountInOneTime
      ? metronomeIntervalMs() * Math.max(1, metronomeClicksPerBar())
      : 0;
    loopRestartTimerId = setTimeout(function () {
      loopRestartTimerId = null;
      loopRestartInProgress = false;
      softLoopEnding = false;
    }, Math.max(800, countInMs + 600));
  }

  /**
   * Loop end: jump score and backing back to the start immediately.
   * A delayed pause left the cursor parked while the already-rendered buffer
   * kept playing past the loop.
   */
  function beginSoftLoopEnd() {
    if (!api || !wantPlayback || !pendingLoop.enabled) {
      return;
    }
    if (softLoopEnding || loopRestartInProgress) {
      return;
    }
    softLoopEnding = true;
    loopRestartInProgress = true;
    restHoldActive = false;
    clearLoopRestartTimer();
    restartPlaybackFromLoopStart();
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

  function cancelFutureClicks() {
    const ctx = clickAudioCtx;
    const now = ctx ? ctx.currentTime : 0;
    const keep = [];
    for (let i = 0; i < scheduledClicks.length; i++) {
      const entry = scheduledClicks[i];
      if (entry.when > now + 0.012) {
        try {
          entry.osc.stop();
        } catch (e) {
          /* already stopped */
        }
        try {
          entry.noise.stop();
        } catch (e) {
          /* already stopped */
        }
      } else {
        keep.push(entry);
      }
    }
    scheduledClicks = keep;
  }

  function stopMetronomeClock() {
    if (metroTimerId !== null) {
      clearTimeout(metroTimerId);
      metroTimerId = null;
    }
    metroClockActive = false;
    metroCountInClicks = 0;
    cancelArmedMetronomeStart();
    // Clicks already in the audio queue would otherwise sound beside the new grid.
    cancelFutureClicks();
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

  function metronomeIntervalSec() {
    return Math.max(0.025, metronomeIntervalMs() / 1000);
  }

  /**
   * One grid for count-in and metronome. Click i is origin + i * interval.
   * The origin stays put, so the accent cannot walk to another beat.
   */
  let metroOriginTime = 0;
  let metroInterval = 0.5;

  function metronomeClockLive() {
    return metroClockActive && metroTimerId !== null;
  }

  /**
   * The score has just been told to play. The Playing event usually starts the
   * clock; this retries when that event never arrives (state still looks paused
   * after a loop wrap) so the click does not stay on the count-in only.
   */
  function armMetronomeForScoreStart() {
    const attempt = function (triesLeft) {
      if (!wantPlayback || countInPhase || !pendingMetronomeEnabled) {
        return;
      }
      if (metronomeClockLive()) {
        return;
      }
      if (!isPlayerPlaying() && !restHoldActive && triesLeft > 0) {
        setTimeout(function () {
          attempt(triesLeft - 1);
        }, 40);
        return;
      }
      // Last try starts anyway. After a loop wrap the facade can stay "paused"
      // while the score is already playing, and the click would never return.
      startMetronomeClock(true, true);
    };
    attempt(12);
  }

  let metroPumping = false;

  function pumpMetronome() {
    if (metroPumping) {
      return;
    }
    metroPumping = true;
    try {
      pumpMetronomeBody();
    } finally {
      metroPumping = false;
    }
  }

  function pumpMetronomeBody() {
    if (metroTimerId !== null) {
      clearTimeout(metroTimerId);
      metroTimerId = null;
    }
    // Count-in is this same clock for one bar. A second scheduler clicked
    // beside it and the two were never on the same beat.
    if (!metroClockActive || (!pendingMetronomeEnabled && !countInPhase)) {
      return;
    }
    if (!wantPlayback) {
      metroClockActive = false;
      metroCountInClicks = 0;
      return;
    }
    const ctx = ensureClickAudio();
    if (!ctx) {
      metroTimerId = setTimeout(pumpMetronome, 40);
      return;
    }
    const perBar = Math.max(1, metronomeClicksPerBar());
    // One beat ahead is enough. A long queue survived tempo updates and
    // ticked on top of the new grid, like a clock beside the count.
    const horizon = ctx.currentTime + metroInterval * 1.25;
    let queued = 0;
    while (queued < 4) {
      // Metronome off: the count-in bar clicks, and the next "1" only starts
      // the song. Metronome on: that "1" is the next accent of this clock.
      if (
        metroCountInClicks > 0 &&
        !pendingMetronomeEnabled &&
        metroClickIndex >= metroCountInClicks
      ) {
        break;
      }
      const when = metroOriginTime + metroClickIndex * metroInterval;
      if (when > horizon) {
        break;
      }
      if (when >= ctx.currentTime - 0.04) {
        try {
          playWebClick(metroClickIndex % perBar === 0, when);
        } catch (e) {
          /* one bad click must not stop the clock */
        }
      }
      metroClickIndex += 1;
      queued += 1;
    }
    maybeRequestCountInHandoff(ctx);
    finishCountInAtDownbeat(ctx);
    if (!metroClockActive || (!pendingMetronomeEnabled && !countInPhase)) {
      return;
    }
    let nextWhen = metroOriginTime + metroClickIndex * metroInterval;
    if (countInPhase && countInEndTime > 0) {
      nextWhen = Math.min(nextWhen, countInEndTime);
      if (!countInHandoffSent) {
        nextWhen = Math.min(nextWhen, countInEndTime - countInHandoffLeadSec());
      }
    }
    const wakeMs = Math.max(15, (nextWhen - ctx.currentTime - 0.02) * 1000);
    metroTimerId = setTimeout(pumpMetronome, wakeMs);
  }

  /**
   * Tempo changed. Keep which click is the "1"; only the spacing after the
   * next click changes. Restarting from "now" walked the accent through the bar.
   */
  function retargetMetronomeGrid() {
    if (!metroClockActive) {
      return;
    }
    const newInterval = metronomeIntervalSec();
    if (!(newInterval > 0) || Math.abs(newInterval - metroInterval) < 0.0005) {
      return;
    }
    const nextIndex = metroClickIndex;
    const nextTime = metroOriginTime + nextIndex * metroInterval;
    metroInterval = newInterval;
    metroOriginTime = nextTime - nextIndex * newInterval;
    if (countInPhase && metroCountInClicks > 0) {
      countInEndTime = metroOriginTime + metroCountInClicks * metroInterval;
    }
  }

  function startMetronomeClock(resetIndex, force) {
    cancelArmedMetronomeStart();
    // The count-in already is this clock. Resetting on Play clicked a second
    // "1" beside the one that starts the bar.
    if (metroClockActive) {
      if (metroTimerId === null) {
        pumpMetronome();
      }
      return;
    }
    if (metroTimerId !== null) {
      clearTimeout(metroTimerId);
      metroTimerId = null;
    }
    if (!pendingMetronomeEnabled || countInPhase) {
      return;
    }
    if (!force && !isPlayerPlaying() && !restHoldActive) {
      return;
    }
    applyPendingTempo();
    const ctx = ensureClickAudio();
    metroInterval = metronomeIntervalSec();
    if (resetIndex) {
      metroClickIndex = 0;
      metroOriginTime = ctx ? ctx.currentTime : 0;
    } else if (ctx) {
      metroOriginTime = ctx.currentTime - metroClickIndex * metroInterval;
    }
    metroClockActive = true;
    pumpMetronome();
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

  let clickNoiseBuffer = null;

  function clickNoiseBufferFor(ctx) {
    if (clickNoiseBuffer && clickNoiseBuffer.sampleRate === ctx.sampleRate) {
      return clickNoiseBuffer;
    }
    const frames = Math.max(1, Math.floor(ctx.sampleRate * 0.02));
    const buffer = ctx.createBuffer(1, frames, ctx.sampleRate);
    const data = buffer.getChannelData(0);
    for (let i = 0; i < frames; i++) {
      data[i] = Math.random() * 2 - 1;
    }
    clickNoiseBuffer = buffer;
    return buffer;
  }

  function playWebClick(accent, when) {
    const ctx = ensureClickAudio();
    if (!ctx) {
      return;
    }
    const level = metronomeMuted ? 0 : Math.max(0, Math.min(2, metronomeVolume));
    if (level <= 0.0001) {
      return;
    }
    const requested = typeof when === "number" ? when : ctx.currentTime;
    const now = Math.max(ctx.currentTime, requested);
    const peak = (accent ? 0.7 : 0.38) * level;
    const entry = { when: now, osc: null, noise: null };

    const osc = ctx.createOscillator();
    const tone = ctx.createGain();
    osc.type = "square";
    osc.frequency.setValueAtTime(accent ? 2100 : 1400, now);
    osc.frequency.exponentialRampToValueAtTime(accent ? 900 : 650, now + 0.018);
    tone.gain.setValueAtTime(peak, now);
    tone.gain.exponentialRampToValueAtTime(0.001, now + (accent ? 0.045 : 0.03));
    osc.connect(tone);
    tone.connect(ctx.destination);
    osc.start(now);
    osc.stop(now + 0.05);
    entry.osc = osc;

    const noise = ctx.createBufferSource();
    noise.buffer = clickNoiseBufferFor(ctx);
    const filter = ctx.createBiquadFilter();
    filter.type = "highpass";
    filter.frequency.value = accent ? 2500 : 1800;
    const noiseGain = ctx.createGain();
    noiseGain.gain.setValueAtTime(peak * 0.65, now);
    noiseGain.gain.exponentialRampToValueAtTime(0.001, now + 0.012);
    noise.connect(filter);
    filter.connect(noiseGain);
    noiseGain.connect(ctx.destination);
    noise.start(now);
    noise.stop(now + 0.015);
    entry.noise = noise;
    scheduledClicks.push(entry);
    if (scheduledClicks.length > 32) {
      scheduledClicks.splice(0, scheduledClicks.length - 16);
    }
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
    silenceCountInTimer();
    countInBeatsPlayed = 0;
    stopCountInCursorFreeze();
  }

  let countInDone = null;
  let countInEndTime = 0;

  function countInBeatWallMs() {
    let denom = 4;
    try {
      const bars = api && api.score && api.score.masterBars;
      if (bars && bars.length > 0) {
        const d = Number(bars[0].timeSignatureDenominator);
        if (d > 0) {
          denom = d;
        }
      }
    } catch (e) {
      /* ignore */
    }
    const speed = api && api.playbackSpeed > 0 ? Number(api.playbackSpeed) : 1;
    return Math.max(40, (60000 / Math.max(1, resolveBpm()) / speed) * (4 / denom));
  }

  function silenceCountInTimer() {
    if (countInTimerId !== null) {
      clearTimeout(countInTimerId);
      countInTimerId = null;
    }
    clearPlayOnDownbeatTimer();
    countInPhase = false;
    countInDone = null;
    countInEndTime = 0;
    countInHandoffSent = false;
    pendingDownbeatTime = 0;
    metroCountInClicks = 0;
  }

  /**
   * Ask the UI to resume one short round-trip before the downbeat, so play()
   * can wait and land on the click instead of starting a second metronome.
   */
  function maybeRequestCountInHandoff(ctx) {
    if (!countInPhase || countInHandoffSent || !countInDone || !(countInEndTime > 0) || !ctx) {
      return;
    }
    if (ctx.currentTime + countInHandoffLeadSec() < countInEndTime) {
      return;
    }
    countInHandoffSent = true;
    pendingDownbeatTime = countInEndTime;
    holdClickClock((countInHandoffLeadSec() + 0.5) * 1000);
    const done = countInDone;
    countInDone = null;
    if (done) {
      done();
    }
  }

  /**
   * The count-in bar and the song share one metronome grid. Playback starts
   * when that clock reaches the next "1", and the click keeps going if the
   * metronome stays on.
   */
  function finishCountInAtDownbeat(ctx) {
    if (!countInPhase || !(countInEndTime > 0) || !ctx) {
      return;
    }
    if (ctx.currentTime + 0.008 < countInEndTime) {
      return;
    }
    const done = countInDone;
    countInPhase = false;
    countInDone = null;
    countInEndTime = 0;
    metroCountInClicks = 0;
    if (!pendingMetronomeEnabled) {
      metroClockActive = false;
      if (metroTimerId !== null) {
        clearTimeout(metroTimerId);
        metroTimerId = null;
      }
      cancelFutureClicks();
    }
    // Fallback if the early handoff never ran (no audio clock yet).
    if (done) {
      countInHandoffSent = true;
      done();
    }
  }

  function cancelStandaloneCountIn() {
    const active = countInPhase;
    silenceCountInTimer();
    clearSubdivisionClicks();
    if (active) {
      notifyNative("countInCancelled", {});
    }
  }

  function pumpCountInEnd() {
    countInTimerId = null;
    if (!countInPhase || !wantPlayback) {
      return;
    }
    const ctx = ensureClickAudio();
    if (ctx && ctx.currentTime + 0.012 < countInEndTime) {
      const wake = Math.max(15, (countInEndTime - ctx.currentTime) * 1000);
      countInTimerId = setTimeout(pumpCountInEnd, Math.min(wake, 40));
      return;
    }
    const done = countInDone;
    countInPhase = false;
    countInDone = null;
    countInEndTime = 0;
    metroCountInClicks = 0;
    if (done) {
      done();
    }
  }

  /**
   * One metronome bar before the score and backing. The clicks are the
   * metronome clock itself; playback starts on its next "1".
   */
  function startStandaloneCountIn(done) {
    silenceCountInTimer();
    clearSubdivisionClicks();
    stopMetronomeClock();
    countInPhase = true;
    countInBeatsPlayed = 0;
    countInDone = done;
    notifyNative("countInStarted", {});
    applyPendingTempo();
    const clicks = Math.max(1, metronomeClicksPerBar());
    const interval = metronomeIntervalSec();
    metroCountInClicks = clicks;
    metroClickIndex = 0;
    metroInterval = interval;
    const ctx = ensureClickAudio();
    if (!ctx) {
      countInTimerId = setTimeout(pumpCountInEnd, clicks * interval * 1000);
      return;
    }
    // Click 0 is "1". The song starts on the next "1", one bar later.
    metroOriginTime = ctx.currentTime;
    countInEndTime = metroOriginTime + clicks * interval;
    metroClockActive = true;
    pumpMetronome();
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
    // Mute AlphaTab's metronome and count-in. Clicks are ours, and the backing
    // must not start until that bar of clicks has finished.
    api.metronomeVolume = 0;
    api.countInVolume = 0;
    // Leave the count-in clock alone. Stopping it here, when Play arrives
    // just before the "1", dropped the downbeat and the next beat queued behind it.
    if (!pendingMetronomeEnabled && !countInPhase) {
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
        // Clicks come only from the metronome grid. A second click here
        // turned the count-in into a tick and pulled the accent off the 1.
        if (!countInPhase && pendingMetronomeEnabled && beatInBar === 0 && !metroClockActive) {
          pendingMetroStartOnNextDownbeat = false;
          startMetronomeClock(true);
        }
      }
    });
    onEvent(instance.playerPositionChanged, function (pos) {
      if (!pos) {
        return;
      }
      const bpm = Number(pos.originalTempo || pos.currentTempo || 0);
      if (bpm > 0 && Math.abs(bpm - liveBpm) > 1) {
        liveBpm = bpm;
        retargetMetronomeGrid();
      }
      const tick = Number(pos.currentTick);
      if (Number.isFinite(tick)) {
        // A pause at the loop end seeks AlphaTab back onto the cursor beat —
        // the last note. Ignore that tick until the next pass is actually playing.
        const staleLoopEnd =
          pendingLoop.enabled &&
          (loopRestartInProgress || softLoopEnding || countInPhase) &&
          tick + 1 >= endTickForBar(pendingLoop.startBar, pendingLoop.endBar);
        // A loop wrap seeks backward. The old tick must not keep the cursor pinned.
        if (!staleLoopEnd && (tick > lastPlaybackTick || tick + 1 < lastPlaybackTick)) {
          lastPlaybackTick = tick;
        }
      }
      publishScorePosition(false);
      if (!pendingLoop.enabled) {
        return;
      }
      const loopEnd = endTickForBar(pendingLoop.startBar, pendingLoop.endBar);
      if (loopRestartInProgress || softLoopEnding || countInPhase) {
        // Don't release the latch on a stale end-tick, and not while the
        // count-in is still clicking — Play has not started the next pass yet.
        if (
          !countInPhase &&
          isPlayerPlaying() &&
          Number.isFinite(tick) &&
          tick + quarterTickResolution() / 2 < loopEnd
        ) {
          loopRestartInProgress = false;
          softLoopEnding = false;
        }
        return;
      }
      // Soft-catch the musical bar end before AlphaTab stop()/noteOffAll hard-cuts.
      if (
        wantPlayback &&
        !countInPhase &&
        Number.isFinite(tick) &&
        tick >= loopEnd - 1
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
        // Count-in is the metronome clock with the score still stopped.
        // A pause or loop wrap must not kill that clock before the "1".
        const keepClickClock =
          wantPlayback &&
          (countInPhase ||
            countInHandoffSent ||
            nowMs() < clickClockHoldUntilMs ||
            (pendingLoop.enabled && (loopRestartInProgress || softLoopEnding)));
        if (!restHoldActive && !keepClickClock) {
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
        if (
          wantPlayback &&
          pendingMetronomeEnabled &&
          !countInPhase &&
          !countInHandoffSent &&
          nowMs() >= clickClockHoldUntilMs &&
          !pendingMetroStartOnNextDownbeat
        ) {
          // Count-in clicks are finished before play(), so this Playing state is
          // the musical downbeat. Do not start again if the clock is already up.
          if (!metronomeClockLive()) {
            startMetronomeClock(true);
          }
        }
      }
      if ((loopRestartInProgress || restHoldActive || softLoopEnding) && !playing) {
        syncNotationInteractive();
        return;
      }
      syncNotationInteractive();
      notifyNative("playerState", { playing: !!playing });
    });

    onEvent(instance.playerFinished, function () {
      if (loopRestartInProgress || softLoopEnding || countInPhase) {
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
      syncNotationInteractive();
      notifyNative("playerState", { playing: false });
    });

    onEvent(instance.beatMouseUp, function (beat) {
      if (wantPlayback || countInPhase || isPlayerPlaying() || !beat) {
        return;
      }
      const master = beat.voice && beat.voice.bar && beat.voice.bar.masterBar;
      const index = master ? Number(master.index) : NaN;
      if (!master || !Number.isFinite(index)) {
        return;
      }
      const bar = index + 1;
      let tick = Number(beat.absolutePlaybackStart);
      if (!Number.isFinite(tick)) {
        tick = tickForBar(bar);
      }
      notifyNative("barAnchor", {
        bar: bar,
        beat: (Number(beat.index) || 0) + 1,
        tick: Math.max(0, Math.round(tick)),
      });
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
      const rawTrack = Number(options.trackIndex);
      if (Number.isFinite(rawTrack)) {
        selectedTrackIndex = Math.max(0, rawTrack);
      }
    }
    if (options.tempoPercent !== undefined && options.tempoPercent !== null) {
      pendingTempoPercent = Math.max(
        MIN_TEMPO_PERCENT,
        Math.min(MAX_TEMPO_PERCENT, Number(options.tempoPercent) || 100)
      );
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
    if (options.countInOneTime !== undefined) {
      pendingCountInOneTime = !!options.countInOneTime;
    }
    if (options.metronomeVolume !== undefined && options.metronomeVolume !== null) {
      const volume = Number(options.metronomeVolume);
      if (Number.isFinite(volume)) {
        metronomeVolume = Math.max(0, Math.min(2, volume));
      }
    }
    if (options.metronomeMuted !== undefined) {
      metronomeMuted = !!options.metronomeMuted;
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
      const next = !!isDark;
      // Every native command is prefixed with setTheme. Re-rendering an unchanged
      // theme rebuilds the cursor on bar 1.
      if (api && next === darkTheme) {
        return;
      }
      darkTheme = next;
      applyPageTheme(darkTheme);
      applyNotationTheme(darkTheme, true);
      placeCursorAtPracticeBar();
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

    beginCountIn: function () {
      if (!api) {
        return;
      }
      wantPlayback = true;
      syncNotationInteractive();
      ensureClickAudio();
      applyPendingMetronome();
      startStandaloneCountIn(function () {
        if (!wantPlayback) {
          return;
        }
        notifyNative("countInFinished", { loop: false, bar: 1 });
      });
    },
    play: function () {
      playScoreNow();
    },
    playFromBar: function (bar) {
      if (!api) {
        return;
      }
      loopResumeTick = tickForBar(Math.max(1, Number(bar) || 1));
      playScoreNow();
    },
    pause: function () {
      wantPlayback = false;
      softLoopEnding = false;
      loopRestartInProgress = false;
      loopResumeTick = -1;
      clearLoopRestartTimer();
      cancelStandaloneCountIn();
      stopMetronomeClock();
      clearSubdivisionClicks();
      endCountInPhase();
      syncNotationInteractive();
      publishScorePosition(true);
      if (api) {
        api.pause();
      }
    },
    stop: function () {
      wantPlayback = false;
      softLoopEnding = false;
      loopRestartInProgress = false;
      loopResumeTick = -1;
      clearLoopRestartTimer();
      cancelStandaloneCountIn();
      stopMetronomeClock();
      clearSubdivisionClicks();
      endCountInPhase();
      syncNotationInteractive();
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
          if (pendingCountInEnabled) {
            startStandaloneCountIn(function () {
              if (!wantPlayback) {
                return;
              }
              notifyNative("countInFinished", { loop: false, bar: 1 });
            });
            return;
          }
        } else {
          wantPlayback = false;
          softLoopEnding = false;
          clearLoopRestartTimer();
          cancelStandaloneCountIn();
          stopMetronomeClock();
          endCountInPhase();
          clearSubdivisionClicks();
        }
        api.playPause();
      }
    },

    setTempoPercent: function (percent) {
      pendingTempoPercent = Math.max(
        MIN_TEMPO_PERCENT,
        Math.min(MAX_TEMPO_PERCENT, Number(percent) || 100)
      );
      const playing = isPlayerPlaying() || restHoldActive || softLoopEnding;
      const keepTick = playing && lastPlaybackTick > 0 ? lastPlaybackTick : -1;
      applyPendingTempo();
      if (keepTick >= 0) {
        flushSynthQueueAt(keepTick);
        requestAnimationFrame(function () {
          if (!api || !(isPlayerPlaying() || restHoldActive || softLoopEnding)) {
            return;
          }
          flushSynthQueueAt(keepTick);
        });
      } else {
        placeCursorAtPracticeBar();
      }
      if (pendingMetronomeEnabled && isPlayerPlaying() && !countInPhase) {
        if (metroClockActive) {
          retargetMetronomeGrid();
        } else if (!pendingCountInEnabled && !pendingMetroStartOnNextDownbeat) {
          startMetronomeClock(false);
        }
      }
    },

    reportBarSourceMs: function (bar, direction) {
      notifyNative("barSourceMs", {
        ms: barSourceMs(bar),
        direction: Number(direction) || 0,
      });
    },

    seekToBar: function (bar) {
      if (!api) {
        return;
      }
      const tick = tickForBar(Math.max(1, Number(bar) || 1));
      try {
        api.tickPosition = tick;
        lastPlaybackTick = tick;
      } catch (e) {
        /* ignore */
      }
      publishScorePosition(true);
    },

    setSyncSpeedRatio: function (ratio) {
      const value = Number(ratio);
      syncSpeedRatio = Number.isFinite(value) && value > 0 ? value : 1;
      applyPendingTempo();
      if (metroClockActive) {
        retargetMetronomeGrid();
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

    setMetronomeVolume: function (volume) {
      const value = Number(volume);
      metronomeVolume = Number.isFinite(value) ? Math.max(0, Math.min(2, value)) : 1;
    },

    setMetronomeMuted: function (muted) {
      metronomeMuted = !!muted;
    },

    setCountIn: function (enabled) {
      pendingCountInEnabled = !!enabled;
      applyPendingMetronome();
    },

    setCountInOneTime: function (oneTime) {
      pendingCountInOneTime = !!oneTime;
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
