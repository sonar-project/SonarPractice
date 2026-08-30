/**
 * AlphaTab bridge for SonarPractice (loaded inside Qt WebEngine).
 * Native UI calls window.sonarAlphaTab.* via runJavaScript.
 * Events go to native via console.log('SONAR|' + json) (no WebChannel required).
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

  function applyTrackVisibility() {
    if (!api || !api.score) {
      return;
    }
    const tracks = api.score.tracks || [];
    if (selectedTrackIndex < 0 || selectedTrackIndex >= tracks.length) {
      selectedTrackIndex = 0;
    }
    api.renderTracks = [tracks[selectedTrackIndex]];
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

  function initApi() {
    if (api) {
      return;
    }
    if (typeof alphaTab === "undefined") {
      setStatus("alphaTab failed to load", true);
      notifyNative("error", { message: "alphaTab failed to load" });
      return;
    }

    api = new alphaTab.AlphaTabApi(hostEl, {
      core: {
        engine: "html5",
        fontDirectory: "./font/",
        logLevel: alphaTab.LogLevel.Warning,
      },
      display: {
        layoutMode: alphaTab.LayoutMode.Page,
        scale: 1.0,
      },
      notation: {
        rhythmMode: alphaTab.TabRhythmMode.ShowWithBeams,
      },
      player: {
        enablePlayer: true,
        enableCursor: true,
        enableAnimatedBeatCursor: true,
        enableElementHighlighting: true,
        soundFont: "./soundfont/sonivox.sf3",
        scrollMode: alphaTab.ScrollMode.Continuous,
        scrollElement: scrollEl,
      },
    });

    api.scoreLoaded.on(function () {
      scoreReady = true;
      applyTrackVisibility();
      const names = (api.score.tracks || []).map(function (t, i) {
        return t.name || ("Track " + (i + 1));
      });
      notifyNative("scoreLoaded", {
        title: api.score.title || "",
        artist: api.score.artist || "",
        trackNames: names,
        barCount: (api.score.masterBars && api.score.masterBars.length) || 0,
      });
      setStatus("");
    });

    api.playerReady.on(function () {
      playerReady = true;
      notifyNative("playerReady", {});
    });

    api.playerStateChanged.on(function (args) {
      const playing = args && args.state === alphaTab.synth.PlayerState.Playing;
      notifyNative("playerState", { playing: !!playing });
    });

    api.playerFinished.on(function () {
      notifyNative("playerState", { playing: false });
    });
  }

  window.sonarAlphaTab = {
    loadScoreBase64: function (base64) {
      try {
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
      if (!api) {
        return;
      }
      const p = Math.max(25, Math.min(200, Number(percent) || 100));
      api.playbackSpeed = p / 100.0;
    },

    setTrackIndex: function (index) {
      selectedTrackIndex = Math.max(0, Number(index) || 0);
      applyTrackVisibility();
    },

    setLoop: function (enabled, startBar, endBar) {
      if (!api || !scoreReady) {
        return;
      }
      if (!enabled) {
        api.isLooping = false;
        api.playbackRange = null;
        return;
      }
      const start = Math.max(1, Number(startBar) || 1);
      const end = Math.max(start, Number(endBar) || start);
      api.playbackRange = {
        startTick: tickForBar(start),
        endTick: endTickForBar(end),
      };
      api.isLooping = true;
    },

    isReady: function () {
      return !!(scoreReady && playerReady);
    },
  };

  setStatus("Player ready — waiting for score");
  notifyNative("bridgeReady", {});
})();
