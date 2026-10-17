#ifndef TST_AUDIOPLAYBACKHELPERSTEST_H
#define TST_AUDIOPLAYBACKHELPERSTEST_H

#include <QObject>

class TestAudioPlaybackHelpers : public QObject {
    Q_OBJECT

  private slots:
    void tempoRatioFromPercent();
    void floatToInt16ClampsOutOfRangeValues();
    void floatToInt16ProducesCorrectByteCount();
};

#endif // TST_AUDIOPLAYBACKHELPERSTEST_H
