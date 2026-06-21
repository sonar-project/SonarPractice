#ifndef AUDIOPROCESSORWORKER_H
#define AUDIOPROCESSORWORKER_H

#include "AudioPlaybackEngine.h"
#include <QObject>

class AudioProcessorWorker : public QObject {
    Q_OBJECT

  public:
    explicit AudioProcessorWorker(QObject *parent = nullptr);

  public slots:
    void process(AudioPlaybackEngine::BuildParameters params);

  signals:
    void resultReady(AudioBuildResult result);
    void progressUpdated(QString stage, int percent);
};

#endif // AUDIOPROCESSORWORKER_H
