#include "tst_audioPeakExtractorTest.h"

#include "AudioPeakExtractor.h"

#include <QTest>
#include <algorithm>

void TestAudioPeakExtractor::rejectsInvalidInput() {
    const std::vector<float> samples = {0.5F, 0.5F};
    QVERIFY(AudioPeakExtractor::computePeaks({}, 2, 32).isEmpty());
    QVERIFY(AudioPeakExtractor::computePeaks(samples, 0, 32).isEmpty());
    QVERIFY(AudioPeakExtractor::computePeaks(samples, 2, 0).isEmpty());
}

void TestAudioPeakExtractor::peaksAreNormalized() {
    std::vector<float> samples(2000, 0.0F);
    samples[100] = 0.5F;
    samples[500] = 1.0F;

    const QVector<float> peaks = AudioPeakExtractor::computePeaks(samples, 2, 32);
    QCOMPARE(peaks.size(), 32);

    const float maxPeak = *std::max_element(peaks.begin(), peaks.end(),
                                            [](float left, float right) { return left < right; });
    QVERIFY(maxPeak <= 1.0F);
    QVERIFY(maxPeak > 0.9F);
}

void TestAudioPeakExtractor::stereoPeaksUseBothChannels() {
    std::vector<float> samples(200, 0.0F);
    samples[10] = 0.2F;
    samples[11] = 0.9F;

    const QVector<float> peaks = AudioPeakExtractor::computePeaks(samples, 2, 8);
    QCOMPARE(peaks.size(), 8);
    QVERIFY(peaks[0] > 0.8F);
}

QTEST_MAIN(TestAudioPeakExtractor)
