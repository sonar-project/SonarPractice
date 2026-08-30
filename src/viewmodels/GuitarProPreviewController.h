#ifndef GUITARPROPREVIEWCONTROLLER_H
#define GUITARPROPREVIEWCONTROLLER_H

#include "AsciiTabRenderer.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

#include <atomic>
#include <vector>

class ApplicationErrorLog;
class IMediaFileRepository;
class IPathResolver;

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
    Q_PROPERTY(bool playerReady READ playerReady NOTIFY playerReadyChanged)
    Q_PROPERTY(int tempoPercent READ tempoPercent WRITE setTempoPercent NOTIFY tempoPercentChanged)
    Q_PROPERTY(bool loopEnabled READ loopEnabled WRITE setLoopEnabled NOTIFY loopChanged)
    Q_PROPERTY(int loopStartBar READ loopStartBar WRITE setLoopStartBar NOTIFY loopChanged)
    Q_PROPERTY(int loopEndBar READ loopEndBar WRITE setLoopEndBar NOTIFY loopChanged)
    Q_PROPERTY(int barCount READ barCount NOTIFY barCountChanged)

  public:
    struct Dependencies {
        IMediaFileRepository &mediaRepo;
        IPathResolver &pathResolver;
        ApplicationErrorLog &errorLog;
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
    [[nodiscard]] bool playerReady() const;
    [[nodiscard]] int tempoPercent() const;
    void setTempoPercent(int tempoPercent);
    [[nodiscard]] bool loopEnabled() const;
    void setLoopEnabled(bool enabled);
    [[nodiscard]] int loopStartBar() const;
    void setLoopStartBar(int bar);
    [[nodiscard]] int loopEndBar() const;
    void setLoopEndBar(int bar);
    [[nodiscard]] int barCount() const;

  public slots:
    /** Loads and shows preview for @p mediaFileId (Guitar Pro only). */
    Q_INVOKABLE void load(qlonglong mediaFileId);
    /** Hides the panel and clears the current preview. */
    Q_INVOKABLE void clear();

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void playPause();

    /** Called from QML/WebChannel when AlphaTab posts a JSON event. */
    Q_INVOKABLE void handlePlayerEvent(const QString &json);
    /** JavaScript snippet to (re)load the current score into AlphaTab. */
    Q_INVOKABLE QString loadScoreJavaScript() const;
    /** JavaScript snippet applying track/tempo/loop to a ready player. */
    Q_INVOKABLE QString applyPlayerSettingsJavaScript() const;

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
    void playerReadyChanged();
    void tempoPercentChanged();
    void loopChanged();
    void barCountChanged();
    /** Ask QML WebEngineView to evaluate @p javaScript. */
    void runPlayerJavaScript(const QString &javaScript);

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
    void setPlayerReady(bool ready);
    void setBarCount(int count);
    void emitPlayerCommand(const QString &javaScript);

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
    bool m_playerReady{false};
    int m_tempoPercent{100};
    bool m_loopEnabled{false};
    int m_loopStartBar{1};
    int m_loopEndBar{1};
    int m_barCount{0};
};

#endif // GUITARPROPREVIEWCONTROLLER_H
