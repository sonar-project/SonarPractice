#ifndef AUDIOANALYZERWORKER_H
#define AUDIOANALYZERWORKER_H

#include <QObject>
#include <QString>

/** Input for a background pitch-analysis job. */
struct PitchAnalysisRequest {
    QString filePath;
    qint64 regionStartMs{};
    qint64 regionEndMs{};
    bool useRegion{};
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
    void analyze(const PitchAnalysisRequest &request);

  signals:
    void finished(bool success, const QString &errorMessage, const QString &jsonPath,
                  int noteCount);
};

#endif // AUDIOANALYZERWORKER_H
