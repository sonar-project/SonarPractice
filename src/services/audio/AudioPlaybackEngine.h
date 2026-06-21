#ifndef AUDIOPLAYBACKENGINE_H
#define AUDIOPLAYBACKENGINE_H

#include "ParametricEq.h"
#include "PcmPlaybackIODevice.h"

#include <QAudioFormat>
#include <QAudioSink>
#include <QMutex>
#include <QObject>
#include <QTimer>
#include <QVector>
#include <memory>
#include <vector>

#include <atomic>

struct AudioBuildResult {
    bool success{false};
    bool cancelled{false};
    QString errorMessage;
    QByteArray pcmInt16;
    QVector<float> peaks;
    int sampleRateHz{44100};
    int channelCount{2};
    int tempoPercent{100};
    qint64 durationMs{0};
    quint64 buildGeneration{0};
    std::vector<float> decodedInterleaved;
};

Q_DECLARE_METATYPE(AudioBuildResult)

struct SampleRequest {
    qint64 frameIndex{0};
    int channelIndex{0};
    int channelCount{2};
    int sampleRateHz{44100};
};

class AudioProcessorWorker;

class AudioPlaybackEngine : public QObject {
    Q_OBJECT

  public:
    explicit AudioPlaybackEngine(QObject *parent = nullptr);
    ~AudioPlaybackEngine() override;

    struct BuildParameters {
        QString sourceFilePath;
        std::vector<float> sourceInterleaved;
        int channelCount{0};
        int sampleRateHz{0};
        int tempoPercent{100};
        QString eqPresetId;
        quint64 buildGeneration{0};
        std::atomic_bool *cancelRequested{nullptr};
    };

    void loadFile(const QString &filePath);
    void cancelProcessing();
    void play();
    void pause();
    void stop();
    void setTempoPercent(int tempoPercent);
    void commitTempoPercent();
    void setEqPresetId(const QString &presetId);
    void setRegionMs(qint64 regionStartMs, qint64 regionEndMs);
    void setLoopEnabled(bool enabled);

    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] bool isLoading() const;
    [[nodiscard]] const QString &processingStage() const;
    [[nodiscard]] int processingProgress() const;
    [[nodiscard]] qint64 positionMs() const;
    [[nodiscard]] qint64 durationMs() const;
    [[nodiscard]] const QVector<float> &peaks() const;
    [[nodiscard]] const QString &lastError() const;
    [[nodiscard]] bool hasPlaybackData() const;
    [[nodiscard]] static QByteArray floatToInt16(const std::vector<float> &samples);
    [[nodiscard]] static std::vector<float>
    decodeFileToFloat(const QString &filePath, SampleRequest request, QString &errorMessage,
                      const std::atomic_bool *cancelRequested = nullptr);

  signals:
    void playingChanged();
    void loadingChanged();
    void processingStageChanged();
    void processingProgressChanged();
    void positionMsChanged();
    void durationMsChanged();
    void peaksChanged();
    void errorChanged();
    void playbackReadyChanged();
    void requestProcess(AudioPlaybackEngine::BuildParameters params); // NEU

  private:
    void scheduleRebuild(bool immediate);
    void startRebuild();
    void handleRebuildFinished(AudioBuildResult result);
    void applyBuildResult(const AudioBuildResult &result);
    void destroyAudioSink();
    void teardownActivePlayback();
    bool ensureAudioSink();
    void restartSinkFromPosition(qint64 positionMs);
    void updatePlaybackFormat(SampleRequest request);
    void syncLoopRegionToEngine();
    void handleSinkStateChanged(QAudio::State state);
    void beginPlaybackFromSink();
    void setProcessingState(const QString &stage, int progressPercent);
    void dispatchProcess(BuildParameters params);
    [[nodiscard]] qint64 bytesToMs(qint64 bytes) const;
    [[nodiscard]] qint64 msToBytes(qint64 millisecond) const;

    PcmPlaybackIODevice m_playbackDevice;
    std::unique_ptr<QAudioSink> m_audioSink;
    QAudioFormat m_audioFormat;
    QTimer m_positionTimer;
    QTimer m_tempoDebounceTimer;

    QThread *m_workerThread{nullptr};
    AudioProcessorWorker *m_worker{nullptr};

    std::vector<float> m_decodedInterleaved;
    int m_decodedChannelCount{2};
    int m_decodedSampleRateHz{44100};

    int m_pendingTempoPercent{100};
    int m_appliedTempoPercent{100};

    QString m_eqPresetId = QStringLiteral("flat");
    qint64 m_regionStartMs{0};
    qint64 m_regionEndMs{0};

    bool m_loopEnabled{false};
    bool m_playing{false};
    bool m_loading{false};

    QString m_processingStage;
    int m_processingProgress{-1};

    qint64 m_durationMs{0};

    QVector<float> m_peaks;

    QString m_lastError;
    quint64 m_bufferGeneration{0};
    quint64 m_buildGeneration{0};

    bool m_resumeAfterRebuild{false};

    qint64 m_resumePositionMs{0};

    bool m_pauseRequested{false};

    QString m_activeLoadPath;

    std::atomic_bool m_cancelRequested{false};

    mutable QMutex m_mutex;
};

#endif // AUDIOPLAYBACKENGINE_H
