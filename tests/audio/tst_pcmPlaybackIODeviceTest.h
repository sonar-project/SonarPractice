#ifndef TST_PCMPLAYBACKIODEVICETEST_H
#define TST_PCMPLAYBACKIODEVICETEST_H

#include <QObject>

class TestPcmPlaybackIODevice : public QObject {
    Q_OBJECT

  private slots:
    void readDataNeverExceedsBufferBounds();
    void readDataReturnsZeroForEmptyBuffer();
    void setPlaybackPositionClampsToBufferSize();
    void loopRegionEndClampsToBufferSize();
    void loopingRewindsAtRegionEnd();
    void nonLoopingStopsAtRegionEnd();
};

#endif // TST_PCMPLAYBACKIODEVICETEST_H
