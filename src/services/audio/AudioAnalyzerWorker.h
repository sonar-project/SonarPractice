#ifndef AUDIOANALYZERWORKER_H
#define AUDIOANALYZERWORKER_H

#include <QObject>
#include <QString>

/** Input for a background pitch-analysis job. */
struct PitchAnalysisRequest {
    QString filePath;       /**< Resolved path to the audio file on disk. */
    qint64 regionStartMs{}; /**< Region start in source time (when useRegion is true). */
    qint64 regionEndMs{};   /**< Region end in source time (when useRegion is true). */
    bool useRegion{};       /**< When true, only analyze regionStartMs–regionEndMs. */
    int detectionMode{};    /**< 0 = rhythmic, 1 = melodic, 2 = hybrid. */
};

Q_DECLARE_METATYPE(PitchAnalysisRequest)

/**
 * @brief Runs AudioAnalyzer on a worker thread.
 *
 * Created by AudioConfigController and moved to a dedicated QThread so the UI
 * stays responsive during long offline pitch detection.
 */
class AudioAnalyzerWorker : public QObject {
    Q_OBJECT

  public:
    explicit AudioAnalyzerWorker(QObject *parent = nullptr);

  public slots:
    /** Decodes audio, detects notes, writes JSON, and emits finished(). */
    void analyze(const PitchAnalysisRequest &request);

  signals:
    /** Emitted when analysis completes or fails. */
    void finished(bool success, const QString &errorMessage, const QString &jsonPath,
                  int noteCount);
};

#endif // AUDIOANALYZERWORKER_H
