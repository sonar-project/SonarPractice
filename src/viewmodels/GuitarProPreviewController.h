#ifndef GUITARPROPREVIEWCONTROLLER_H
#define GUITARPROPREVIEWCONTROLLER_H

#include "AsciiTabRenderer.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <atomic>
#include <vector>

class ApplicationErrorLog;
class AppSettings;
class IMediaFileRepository;
class IPathResolver;

#ifdef SONARPRACTICE_HAS_WEBENGINE
class SoundFontSchemeHandler;
#endif

/**
 * @brief QML view model for Guitar Pro preview (ASCII) and AlphaTab player bridge.
 *
 * Parses metadata/tab on a worker thread. When WebEngine is available, exposes
 * score bytes as base64 for the embedded AlphaTab page plus playback controls.
 */
class GuitarProPreviewController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Instances are provided via the 'guitarProPreviewController' context property.")

    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY artistChanged)
    Q_PROPERTY(QString tuning READ tuning NOTIFY tuningChanged)
    Q_PROPERTY(QString tabText READ tabText NOTIFY tabTextChanged)
    Q_PROPERTY(QStringList trackNames READ trackNames NOTIFY trackNamesChanged)
    Q_PROPERTY(int selectedTrackIndex READ selectedTrackIndex WRITE setSelectedTrackIndex NOTIFY
                   selectedTrackIndexChanged)
    Q_PROPERTY(qlonglong mediaFileId READ mediaFileId NOTIFY mediaFileIdChanged)

    /** True when the app was built with Qt WebEngine (AlphaTab player). */
    Q_PROPERTY(bool playerAvailable READ playerAvailable CONSTANT)
    Q_PROPERTY(bool usePlayer READ usePlayer WRITE setUsePlayer NOTIFY usePlayerChanged)
    Q_PROPERTY(QString scoreBase64 READ scoreBase64 NOTIFY scoreBase64Changed)
    Q_PROPERTY(QString playerPageUrl READ playerPageUrl CONSTANT)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(int playheadBar READ playheadBar NOTIFY playheadChanged)
    Q_PROPERTY(int playheadBeat READ playheadBeat NOTIFY playheadChanged)
    Q_PROPERTY(int playheadTick READ playheadTick NOTIFY playheadChanged)
    Q_PROPERTY(bool playerReady READ playerReady NOTIFY playerReadyChanged)
    /** True once a usable SoundFont is loaded — Play is gated on this for custom fonts. */
    Q_PROPERTY(bool soundFontReady READ soundFontReady NOTIFY soundFontReadyChanged)
    Q_PROPERTY(int tempoPercent READ tempoPercent WRITE setTempoPercent NOTIFY tempoPercentChanged)
    Q_PROPERTY(int minTempoPercent READ minTempoPercent CONSTANT)
    Q_PROPERTY(int maxTempoPercent READ maxTempoPercent CONSTANT)
    /** Written score tempo in BPM (0 if unknown). */
    Q_PROPERTY(int scoreBpm READ scoreBpm NOTIFY scoreBpmChanged)
    /** Effective playback tempo (score BPM × percent). */
    Q_PROPERTY(int playbackBpm READ playbackBpm WRITE setPlaybackBpm NOTIFY playbackBpmChanged)
    Q_PROPERTY(int minPlaybackBpm READ minPlaybackBpm NOTIFY scoreBpmChanged)
    Q_PROPERTY(int maxPlaybackBpm READ maxPlaybackBpm NOTIFY scoreBpmChanged)
    Q_PROPERTY(bool loopEnabled READ loopEnabled WRITE setLoopEnabled NOTIFY loopChanged)
    Q_PROPERTY(int loopStartBar READ loopStartBar WRITE setLoopStartBar NOTIFY loopChanged)
    Q_PROPERTY(int loopEndBar READ loopEndBar WRITE setLoopEndBar NOTIFY loopChanged)
    Q_PROPERTY(int barCount READ barCount NOTIFY barCountChanged)
    Q_PROPERTY(bool metronomeEnabled READ metronomeEnabled WRITE setMetronomeEnabled NOTIFY
                   metronomeChanged)
    Q_PROPERTY(int metronomeDivision READ metronomeDivision WRITE setMetronomeDivision NOTIFY
                   metronomeChanged)
    Q_PROPERTY(double metronomeVolume READ metronomeVolume WRITE setMetronomeVolume NOTIFY
                   metronomeChanged)
    Q_PROPERTY(bool metronomeMuted READ metronomeMuted WRITE setMetronomeMuted NOTIFY
                   metronomeChanged)
    Q_PROPERTY(bool countInEnabled READ countInEnabled WRITE setCountInEnabled NOTIFY
                   countInChanged)
    /** When true, count-in runs on Play only. Loop wraps continue without it. */
    Q_PROPERTY(bool countInOneTime READ countInOneTime WRITE setCountInOneTime NOTIFY
                   countInChanged)
    Q_PROPERTY(QVariantList mixerTracks READ mixerTracks NOTIFY mixerTracksChanged)
    /** Playback transpose in semitones (notation unchanged). */
    Q_PROPERTY(int transposeSemitones READ transposeSemitones WRITE setTransposeSemitones NOTIFY
                   transposeChanged)

  public:
    struct Dependencies {
        IMediaFileRepository &mediaRepo;
        IPathResolver &pathResolver;
        ApplicationErrorLog &errorLog;
        AppSettings &appSettings;
#ifdef SONARPRACTICE_HAS_WEBENGINE
        SoundFontSchemeHandler *soundFontSchemeHandler = nullptr;
#endif
    };

    explicit GuitarProPreviewController(const Dependencies &dependencies,
                                        QObject *parent = nullptr);

    [[nodiscard]] bool loading() const;
    [[nodiscard]] bool loaded() const;
    [[nodiscard]] bool visible() const;
    void setVisible(bool visible);

    [[nodiscard]] const QString &errorMessage() const;
    [[nodiscard]] const QString &title() const;
    [[nodiscard]] const QString &artist() const;
    [[nodiscard]] const QString &tuning() const;
    [[nodiscard]] const QString &tabText() const;
    [[nodiscard]] const QStringList &trackNames() const;
    [[nodiscard]] int selectedTrackIndex() const;
    void setSelectedTrackIndex(int index);
    [[nodiscard]] qlonglong mediaFileId() const;

    [[nodiscard]] bool playerAvailable() const;
    [[nodiscard]] bool usePlayer() const;
    void setUsePlayer(bool usePlayer);
    [[nodiscard]] const QString &scoreBase64() const;
    [[nodiscard]] QString playerPageUrl() const;
    [[nodiscard]] bool playing() const;
    [[nodiscard]] int playheadBar() const;
    [[nodiscard]] int playheadBeat() const;
    [[nodiscard]] int playheadTick() const;
    [[nodiscard]] bool playerReady() const;
    [[nodiscard]] bool soundFontReady() const;
    static constexpr int kMinTempoPercent = 25;
    /** Headroom above the chart so a fast song can still be practiced quicker. */
    static constexpr int kMaxTempoPercent = 300;

    [[nodiscard]] int tempoPercent() const;
    [[nodiscard]] int minTempoPercent() const { return kMinTempoPercent; }
    [[nodiscard]] int maxTempoPercent() const { return kMaxTempoPercent; }
    void setTempoPercent(int tempoPercent);
    [[nodiscard]] int scoreBpm() const;
    [[nodiscard]] int playbackBpm() const;
    void setPlaybackBpm(int bpm);
    [[nodiscard]] int minPlaybackBpm() const;
    [[nodiscard]] int maxPlaybackBpm() const;
    [[nodiscard]] bool loopEnabled() const;
    void setLoopEnabled(bool enabled);
    [[nodiscard]] int loopStartBar() const;
    void setLoopStartBar(int bar);
    [[nodiscard]] int loopEndBar() const;
    void setLoopEndBar(int bar);
    [[nodiscard]] int barCount() const;
    [[nodiscard]] bool metronomeEnabled() const;
    void setMetronomeEnabled(bool enabled);
    [[nodiscard]] int metronomeDivision() const;
    void setMetronomeDivision(int division);
    [[nodiscard]] double metronomeVolume() const;
    void setMetronomeVolume(double volume);
    [[nodiscard]] bool metronomeMuted() const;
    void setMetronomeMuted(bool muted);
    [[nodiscard]] bool countInEnabled() const;
    void setCountInEnabled(bool enabled);
    [[nodiscard]] bool countInOneTime() const;
    void setCountInOneTime(bool oneTime);
    [[nodiscard]] QVariantList mixerTracks() const;
    [[nodiscard]] int transposeSemitones() const;
    void setTransposeSemitones(int semitones);

  public slots:
    /** Loads and shows preview for @p mediaFileId (Guitar Pro only). */
    Q_INVOKABLE void load(qlonglong mediaFileId);
    /** Hides the panel and clears the current preview. */
    Q_INVOKABLE void clear();

    Q_INVOKABLE void play();
    /** Seeks to a 1-based bar and starts the score in the same step. */
    Q_INVOKABLE void playFromBar(int bar);
    /** Clicks one bar, then reports countInFinished. Does not start the score or audio. */
    Q_INVOKABLE void beginCountIn();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void playPause();
    /** Moves the cursor to the first beat of a 1-based bar without starting playback. */
    Q_INVOKABLE void seekToBar(int bar);
    /**
     * Asks the score for the source-audio length of one bar.
     * @p direction is returned with the result so a later or earlier shift can be applied.
     */
    Q_INVOKABLE void reportBarSourceMs(int bar, int direction);
    /**
     * Multiplies the current tempo while a sync segment is slower or faster than the audio.
     * 1 keeps the tempo the user chose.
     */
    Q_INVOKABLE void setSyncSpeedRatio(double ratio);

    Q_INVOKABLE void setTrackVolume(int index, double volume);
    Q_INVOKABLE void setTrackMuted(int index, bool muted);
    Q_INVOKABLE void transposeUp();
    Q_INVOKABLE void transposeDown();
    Q_INVOKABLE void resetTranspose();

    /** Called from QML/WebChannel when AlphaTab posts a JSON event. */
    Q_INVOKABLE void handlePlayerEvent(const QString &json);
    /** Retry SoundFont load when the player page becomes visible again. */
    Q_INVOKABLE void onPlayerSurfaceActivated();
    /** JavaScript snippet to (re)load the current score into AlphaTab. */
    [[nodiscard]] QString loadScoreJavaScript() const;
    /** JavaScript snippet applying track/tempo/loop to a ready player. */
    [[nodiscard]] QString applyPlayerSettingsJavaScript() const;
    /** JavaScript snippet loading the configured SoundFont into AlphaTab. */
    [[nodiscard]] QString loadSoundFontJavaScript() const;

  signals:
    void loadingChanged();
    void loadedChanged();
    void visibleChanged();
    void errorMessageChanged();
    void titleChanged();
    void artistChanged();
    void tuningChanged();
    void tabTextChanged();
    void trackNamesChanged();
    void selectedTrackIndexChanged();
    void mediaFileIdChanged();
    void usePlayerChanged();
    void scoreBase64Changed();
    void playingChanged();
    void playheadChanged();
    /**
     * AlphaTab restarted a loop at @p bar (@p tick is the score tick of that bar).
     * @p seamless means the score kept playing; only the backing should jump.
     */
    void loopWrapped(int bar, int tick, bool seamless);
    void playerReadyChanged();
    void soundFontReadyChanged();
    void tempoPercentChanged();
    void scoreBpmChanged();
    void playbackBpmChanged();
    void loopChanged();
    void barCountChanged();
    void metronomeChanged();
    void countInChanged();
    /** Count-in clicks are running. Audio and the score stay silent until they finish. */
    void countInStarted();
    void countInCancelled();
    /** Count-in bar ended. @p loopRestart is a loop handoff at @p bar / @p tick. */
    void countInFinished(bool loopRestart, int bar, int tick);
    /** Source-audio milliseconds of one score bar, with the requested shift direction. */
    void barSourceMsReady(int sourceMs, int direction);
    /** The user clicked a score bar to anchor the audio. */
    void barAnchorClicked(int bar, int beat, int tick);
    void mixerTracksChanged();
    void transposeChanged();
    /** Ask QML WebEngineView to evaluate @p javaScript. */
    void runPlayerJavaScript(const QString &javaScript);
    /** Run scripts sequentially in the WebEngine view (used for large SoundFont chunks). */
    void runPlayerJavaScriptSequence(const QStringList &javaScriptCommands);

  private:
    struct LoadResult {
        qlonglong mediaFileId{};
        int generation{};
        bool ok{false};
        QString errorMessage{};
        AsciiTabRenderer::SongPreview preview{};
        QString scoreBase64{};
    };

    void applyLoadResult(const LoadResult &result);
    void applySelectedTrack();
    void setLoading(bool loading);
    void setErrorMessage(const QString &message);
    void resetPreviewState();
    void setPlaying(bool playing);
    void setPlayhead(int bar, int beat, int tick);
    void setPlayerReady(bool ready);
    void setSoundFontReady(bool ready);
    void setBarCount(int count);
    void emitPlayerCommand(const QString &javaScript);
    void syncSoundFontToPlayer();
    void emitSoundFontBytesLoad();
    void ensureCustomSoundFontLoaded();
    void ensurePlaybackSoundFont();
    void flushPendingSoundFontReload();
    void scheduleScoreLoad();
    [[nodiscard]] QString currentSoundFontKey() const;
    void rebuildMixerTracks(const QStringList &names);
    void resetMixerTracks();
    [[nodiscard]] QString mixerTracksJavaScriptArray() const;
    [[nodiscard]] QString playerSettingsObjectJavaScript() const;
    [[nodiscard]] QString configuredSoundFontUrl() const;
    [[nodiscard]] QString configuredCustomSoundFontUrl() const;
    [[nodiscard]] static int clampMetronomeDivision(int division);
    [[nodiscard]] static int clampTransposeSemitones(int semitones);
    [[nodiscard]] int effectiveScoreBpm() const;

    Dependencies m_dependencies;
    std::atomic<int> m_loadGeneration{0};

    bool m_loading{false};
    bool m_loaded{false};
    bool m_visible{false};
    qlonglong m_mediaFileId{};
    QString m_errorMessage{};
    QString m_title{};
    QString m_artist{};
    QString m_tuning{};
    QString m_tabText{};
    QStringList m_trackNames{};
    int m_selectedTrackIndex{-1};
    AsciiTabRenderer::SongPreview m_preview{};

    bool m_usePlayer{true};
    QString m_scoreBase64{};
    bool m_playing{false};
    int m_playheadBar{1};
    int m_playheadBeat{1};
    int m_playheadTick{0};
    bool m_playerReady{false};
    bool m_soundFontReady{false};
    int m_tempoPercent{100};
    bool m_loopEnabled{false};
    int m_loopStartBar{1};
    int m_loopEndBar{1};
    int m_barCount{0};
    bool m_metronomeEnabled{false};
    int m_metronomeDivision{4};
    double m_metronomeVolume{1.0};
    bool m_metronomeMuted{false};
    bool m_countInEnabled{false};
    bool m_countInOneTime{false};
    QVariantList m_mixerTracks{};
    int m_transposeSemitones{0};
    bool m_bridgeReady{false};
    bool m_pendingScoreAfterSoundFont{false};
    bool m_soundFontLoadInFlight{false};
    bool m_pendingSoundFontReload{false};
    QString m_loadedSoundFontKey{};
    bool m_soundFontCustomAbandoned{false};
};

#endif // GUITARPROPREVIEWCONTROLLER_H
