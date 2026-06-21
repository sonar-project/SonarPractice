#ifndef TST_AUDIOBUFFERCONVERTERTEST_H
#define TST_AUDIOBUFFERCONVERTERTEST_H

#include <QObject>

class TestAudioBufferConverter : public QObject {
    Q_OBJECT

  private slots:
    void createEmptyRejectsZeroDimensions();
    void fromInterleavedRejectsEmptyInput();
    void fromInterleavedRoundTripPreservesSamples();
    void fromQAudioBufferRejectsInvalidBuffer();
    void fromQAudioBufferConvertsFloatStereo();
    void fromQAudioBufferConvertsInt16Mono();
    void interleaveIntoRejectsUnsafeDimensions();
    void channelAccessOutOfRangeReturnsEmptySpan();
};

#endif // TST_AUDIOBUFFERCONVERTERTEST_H
