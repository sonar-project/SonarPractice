#ifndef TST_AUDIOPEAKEXTRACTORTEST_H
#define TST_AUDIOPEAKEXTRACTORTEST_H

#include <QObject>

class TestAudioPeakExtractor : public QObject {
    Q_OBJECT

  private slots:
    void rejectsInvalidInput();
    void peaksAreNormalized();
    void stereoPeaksUseBothChannels();
};

#endif // TST_AUDIOPEAKEXTRACTORTEST_H
