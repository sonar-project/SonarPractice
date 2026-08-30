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
        barSeparatorColor: "#eceff1",
        mainGlyphColor: "#eceff1",
        secondaryGlyphColor: "#b0bec5",
        scoreInfoColor: "#eceff1",
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
    if (!Color) {
      return;
    }
    const resources = notationResources(isDark);
    const target = api.settings.display.resources;
    Object.keys(resources).forEach(function (key) {
      target[key] = Color.fromJson(resources[key]);
    });
    api.updateSettings();
    if (rerender && scoreReady) {
      applyTrackVisibility();
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
    api.renderTracks([tracks[selectedTrackIndex]]);
  }

  function tickForBar(barNumber) {
    if (!api || !api.score || !api.score.masterBars || api.score.masterBars.length === 0) {
      return 0;
    }
    const bars = api.score.masterBars;
    const idx = Math.max(0, Math.min(bars.length - 1, barNumber - 1));
    return bars[idx].start;
  }

  function endTickForBar(barNumber) {
    if (!api || !api.score || !api.score.masterBars || api.score.masterBars.length === 0) {
      return 0;
    }
    const bars = api.score.masterBars;
    const idx = Math.max(0, Math.min(bars.length - 1, barNumber - 1));
    const bar = bars[idx];
    return bar.start + bar.calculateDuration();
  }

  function applyPendingTempo() {
    if (!api) {
      return;
    }
    const p = Math.max(25, Math.min(200, Number(pendingTempoPercent) || 100));
    api.playbackSpeed = p / 100.0;
  }

  function applyPendingLoop() {
    if (!api || !scoreReady) {
      return;
    }
    if (!pendingLoop.enabled) {
      api.isLooping = false;
      api.playbackRange = null;
      return;
    }
    const start = Math.max(1, Number(pendingLoop.startBar) || 1);
    const end = Math.max(start, Number(pendingLoop.endBar) || start);
    api.playbackRange = {
      startTick: tickForBar(start),
      endTick: endTickForBar(end),
    };
    api.isLooping = true;
  }

  function applyAllPendingSettings() {
    applyTrackVisibility();
    applyPendingTempo();
    applyPendingLoop();
  }

  function onEvent(emitter, handler) {
    if (emitter && typeof emitter.on === "function") {
      emitter.on(handler);
    }
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

    const instance = new alphaTab.AlphaTabApi(hostEl, {
      core: {
        engine: "html5",
        fontDirectory: "./font/",
        logLevel: alphaTab.LogLevel.Warning,
      },
      display: {
        layoutMode: alphaTab.LayoutMode.Page,
        scale: 1.0,
        resources: notationResources(darkTheme),
      },
      notation: {
        rhythmMode: alphaTab.TabRhythmMode.ShowWithBeams,
      },
      player: {
        playerMode: alphaTab.PlayerMode.EnabledSynthesizer,
        enablePlayer: true,
        enableCursor: true,
        enableAnimatedBeatCursor: true,
        enableElementHighlighting: true,
        soundFont: "./soundfont/sonivox.sf3",
        scrollMode: alphaTab.ScrollMode.Continuous,
        scrollElement: scrollEl,
      },
    });

    // AlphaTab 1.8 exposes soundFontLoaded + error, not soundFontLoadFailed on the API.
    onEvent(instance.error, function (e) {
      const message = (e && e.message) ? e.message : String(e || "AlphaTab error");
      setStatus(message, true);
      notifyNative("error", { message: message });
    });
    onEvent(instance.soundFontLoadFailed, function (e) {
      const message = (e && e.message) ? e.message : "SoundFont load failed";
      setStatus(message, true);
      notifyNative("error", { message: message });
    });

    onEvent(instance.scoreLoaded, function () {
      scoreReady = true;
      // Colors only — do not api.render() here (async render races renderTracks).
      applyNotationTheme(darkTheme, false);
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
      notifyNative("playerState", { playing: !!playing });
    });

    onEvent(instance.playerFinished, function () {
      notifyNative("playerState", { playing: false });
    });

    api = instance;
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
        setStatus("Loading score…");
        scoreReady = false;
        playerReady = false;
        api.load(base64ToArrayBuffer(base64));
      } catch (e) {
        setStatus(String(e && e.message ? e.message : e), true);
        notifyNative("error", { message: String(e) });
      }
    },

    play: function () {
      if (api) {
        api.play();
      }
    },
    pause: function () {
      if (api) {
        api.pause();
      }
    },
    stop: function () {
      if (api) {
        api.stop();
      }
    },
    playPause: function () {
      if (api) {
        api.playPause();
      }
    },

    setTempoPercent: function (percent) {
      pendingTempoPercent = Math.max(25, Math.min(200, Number(percent) || 100));
      applyPendingTempo();
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

    applySettings: function (options) {
      assimilateOptions(options);
      if (scoreReady) {
        applyAllPendingSettings();
      }
    },

    isReady: function () {
      return !!(scoreReady && playerReady);
    },
  };

  applyPageTheme(darkTheme);
  setStatus("Player ready — waiting for score");
  notifyNative("bridgeReady", {});
})();
