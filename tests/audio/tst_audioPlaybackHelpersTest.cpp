#include "tst_audioPlaybackHelpersTest.h"

#include "AudioConstants.h"
#include "AudioPlaybackEngine.h"

#include <QTest>

void TestAudioPlaybackHelpers::tempoRatioFromPercent() {
    QCOMPARE(AudioConstants::rubberBandTimeRatioFromTempoPercent(100), 1.0);
    QCOMPARE(AudioConstants::rubberBandTimeRatioFromTempoPercent(50), 2.0);
    QCOMPARE(AudioConstants::rubberBandTimeRatioFromTempoPercent(80), 1.25);
}

void TestAudioPlaybackHelpers::floatToInt16ClampsOutOfRangeValues() {
    const std::vector<float> samples = {-2.0F, -1.0F, 0.0F, 1.0F, 2.0F};
    const QByteArray pcm = AudioPlaybackEngine::floatToInt16(samples);
    const auto *output = reinterpret_cast<const qint16 *>(pcm.constData());

    QCOMPARE(output[0], static_cast<qint16>(-32767));
    QCOMPARE(output[2], static_cast<qint16>(0));
    QCOMPARE(output[4], static_cast<qint16>(32767));
}

void TestAudioPlaybackHelpers::floatToInt16ProducesCorrectByteCount() {
    const std::vector<float> samples(10, 0.25F);
    const QByteArray pcm = AudioPlaybackEngine::floatToInt16(samples);
    QCOMPARE(pcm.size(), static_cast<int>(samples.size() * AudioConstants::kBytesPerSample));
}

QTEST_MAIN(TestAudioPlaybackHelpers)
