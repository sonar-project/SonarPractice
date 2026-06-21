#ifndef AUDIOCONFIGCONTROLLER_H
#define AUDIOCONFIGCONTROLLER_H

#include "AudioConstants.h"
#include "AudioPlaybackEngine.h"

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <vector>

class IAudioConfigPresetRepository;
class IMediaFileRepository;
class IPathResolver;

/**
 * @brief QML view model for audio playback, region markers, and per-file presets.
 *
 * Tempo, EQ, loop region, and loop flag are cached in memory per media file for
 * the session; named presets are persisted via IAudioConfigPresetRepository.
 */
class AudioConfigController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Instances are provided via the 'audioConfigController' context property.")

    Q_PROPERTY(
        qlonglong mediaFileId READ mediaFileId WRITE setMediaFileId NOTIFY mediaFileIdChanged)
    Q_PROPERTY(QString displayName READ displayName NOTIFY displayNameChanged)
    Q_PROPERTY(QString songTitle READ songTitle WRITE setSongTitle NOTIFY songTitleChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString processingStage READ processingStage NOTIFY processingStageChanged)
    Q_PROPERTY(int processingProgress READ processingProgress NOTIFY processingProgressChanged)
    Q_PROPERTY(qint64 positionMs READ positionMs NOTIFY positionMsChanged)
    Q_PROPERTY(qint64 durationMs READ durationMs NOTIFY durationMsChanged)
    Q_PROPERTY(int tempoPercent READ tempoPercent WRITE setTempoPercent NOTIFY tempoPercentChanged)
    Q_PROPERTY(QString eqPresetId READ eqPresetId WRITE setEqPresetId NOTIFY eqPresetIdChanged)
    Q_PROPERTY(qint64 regionStartMs READ regionStartMs WRITE setRegionStartMs NOTIFY regionChanged)
    Q_PROPERTY(qint64 regionEndMs READ regionEndMs WRITE setRegionEndMs NOTIFY regionChanged)
    Q_PROPERTY(bool loopEnabled READ loopEnabled WRITE setLoopEnabled NOTIFY loopEnabledChanged)
    Q_PROPERTY(QVariantList peaks READ peaks NOTIFY peaksChanged)
    Q_PROPERTY(QStringList presetNames READ presetNames NOTIFY presetNamesChanged)
    Q_PROPERTY(int selectedPresetIndex READ selectedPresetIndex WRITE setSelectedPresetIndex NOTIFY
                   selectedPresetIndexChanged)
    Q_PROPERTY(QString presetNameInput READ presetNameInput WRITE setPresetNameInput NOTIFY
                   presetNameInputChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool canUndoRegion READ canUndoRegion NOTIFY canUndoRegionChanged)
    /** Set after a named preset was loaded and applied to the engine. */
    Q_PROPERTY(bool presetApplied READ presetApplied NOTIFY presetAppliedChanged)

  public:
    struct Dependencies {
        IMediaFileRepository &mediaRepo;
        IAudioConfigPresetRepository &presetRepo;
        IPathResolver &pathResolver;
    };

    explicit AudioConfigController(const Dependencies &dependencies, QObject *parent = nullptr);

    qlonglong mediaFileId() const;
    void setMediaFileId(qlonglong mediaFileId);

    [[nodiscard]] const QString &displayName() const;
    [[nodiscard]] const QString &songTitle() const;
    void setSongTitle(const QString &songTitle);

    [[nodiscard]] bool playing() const;
    [[nodiscard]] bool loading() const;
    [[nodiscard]] const QString &processingStage() const;
    [[nodiscard]] int processingProgress() const;
    [[nodiscard]] bool loopEnabled() const;
    void setLoopEnabled(bool enabled);

    [[nodiscard]] qint64 positionMs() const;
    [[nodiscard]] qint64 durationMs() const;
    [[nodiscard]] int tempoPercent() const;

    void setTempoPercent(int tempoPercent);

    [[nodiscard]] const QString &eqPresetId() const;
    [[nodiscard]] const QStringList &presetNames() const;
    [[nodiscard]] const QString &presetNameInput() const;
    void setPresetNameInput(const QString &name);
    [[nodiscard]] int selectedPresetIndex() const;
    void setSelectedPresetIndex(int index);

    [[nodiscard]] bool presetApplied() const;
    void setEqPresetId(const QString &presetId);

    [[nodiscard]] qint64 regionStartMs() const;
    void setRegionStartMs(qint64 regionStartMs);
    [[nodiscard]] qint64 regionEndMs() const;
    void setRegionEndMs(qint64 regionEndMs);

    [[nodiscard]] QVariantList peaks() const;
    [[nodiscard]] const QString &statusMessage() const;
    [[nodiscard]] bool canUndoRegion() const;

  public slots:
    void reloadMedia();
    /** Loads @p mediaFileId if needed; pauses when that file is already playing. */
    Q_INVOKABLE void playMediaFile(qlonglong mediaFileId);
    /** Switches preset-list context without decoding audio. */
    Q_INVOKABLE void preparePresetsForMedia(qlonglong mediaFileId);
    /** Applies preset @p presetIndex; defers until decode finishes when playback is not ready. */
    Q_INVOKABLE void loadPresetForMedia(qlonglong mediaFileId, int presetIndex);
    void togglePlayback();
    void toggleLoop();
    void stopPlayback();
    /** Re-processes audio with the current tempo (typically after the tempo slider is released). */
    void commitTempo();
    /** Aborts an in-flight decode/tempo pass; status updates when cancellation completes. */
    Q_INVOKABLE void cancelProcessing();
    void setRegionFromPosition(bool isStartMarker);
    void undoRegion();
    void savePreset();
    void loadSelectedPreset();
    void deleteSelectedPreset();

  signals:
    void mediaFileIdChanged();
    void displayNameChanged();
    void songTitleChanged();
    void playingChanged();
    void loadingChanged();
    void processingStageChanged();
    void processingProgressChanged();
    void positionMsChanged();
    void durationMsChanged();
    void tempoPercentChanged();
    void eqPresetIdChanged();
    void regionChanged();
    void loopEnabledChanged();
    void peaksChanged();
    void presetNamesChanged();
    void selectedPresetIndexChanged();
    void presetNameInputChanged();
    void statusMessageChanged();
    void canUndoRegionChanged();
    void presetAppliedChanged();
    /** Short UI notice (e.g. preset loaded); QML may auto-dismiss separately from statusMessage. */
    void transientNoticeRequested(const QString &message);

  private:
    struct RegionSnapshot {
        qint64 startMs{};
        qint64 endMs{};
    };

    /** Session-only settings for one media file (not written to the preset repository). */
    struct CachedMediaSettings {
        int tempoPercent{AudioConstants::kDefaultTempoPercent};
        QString eqPresetId = QStringLiteral("flat");
        qint64 regionStartMs{};
        qint64 regionEndMs{};
        bool loopEnabled{false};
        bool valid{false};
    };

    /** Wires engine signals; runs deferred play/preset actions when decoding finishes. */
    void connectEngine();
    void refreshPresetList();
    void syncPresetSelectionAfterRefresh();
    void syncPeaksFromEngine();
    void setPresetApplied(bool applied);
    void setStatusMessage(const QString &message);
    void pushRegionUndoSnapshot();
    void clearRegionUndoStack();
    void applyRegionMs(qint64 regionStartMs, qint64 regionEndMs, bool recordUndo);
    /** Rescales region markers when processed duration changes (e.g. after tempo commit). */
    void scaleRegionToNewDuration(qint64 oldDurationMs, qint64 newDurationMs);
    [[nodiscard]] static qint64 scalePlaybackMs(qint64 millisecond, qint64 oldDurationMs,
                                                qint64 newDurationMs);
    void saveSettingsForMedia(qlonglong mediaFileId);
    bool restoreSettingsForMedia(qlonglong mediaFileId);
    void persistCurrentSettings();

    Dependencies m_dependencies;
    AudioPlaybackEngine m_engine{};
    qlonglong m_mediaFileId{};
    QString m_displayName{};
    QString m_songTitle{};

    static constexpr int maxPercent{100};
    int m_tempoPercent{maxPercent};
    qint64 m_lastPlaybackDurationMs{};

    QStringList m_presetNames{};
    QString m_eqPresetId = QStringLiteral("flat");
    std::vector<qlonglong> m_presetIds{};
    int m_selectedPresetIndex{-1};
    QString m_presetNameInput{};
    bool m_presetApplied{false};

    qint64 m_regionStartMs{};
    qint64 m_regionEndMs{};
    std::vector<RegionSnapshot> m_regionUndoStack{};

    bool m_loopEnabled{false};

    QString m_statusMessage{};

    QHash<qlonglong, CachedMediaSettings> m_settingsByMediaId{};

    bool m_pendingPlayAfterLoad{false};
    bool m_pendingPresetLoad{false};
    bool m_cancelPending{false};

    qlonglong m_engineLoadedMediaFileId{};
    qlonglong m_loadingTargetMediaFileId{};

    QVariantList m_peaks{};
};

#endif // AUDIOCONFIGCONTROLLER_H
