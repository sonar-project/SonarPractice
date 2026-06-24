#include "tst_audioPeakCacheTest.h"

#include "AudioPeakCache.h"
#include "AudioConstants.h"

#include <QTemporaryDir>
#include <QTest>

void TestAudioPeakCache::roundTripWritesAndReadsPeaks() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString sourcePath = tempDir.filePath(QStringLiteral("track.wav"));
    QFile sourceFile(sourcePath);
    QVERIFY(sourceFile.open(QIODevice::WriteOnly));
    sourceFile.write("fake-audio");
    sourceFile.close();

    AudioPeakCacheEntry written;
    written.peaks.resize(AudioConstants::kPeakBucketCount);
    for (int bucket = 0; bucket < AudioConstants::kPeakBucketCount; ++bucket) {
        written.peaks[bucket] = static_cast<float>(bucket) /
                                static_cast<float>(AudioConstants::kPeakBucketCount);
    }
    written.durationMs = 12000;
    written.sampleRateHz = 44100;
    written.channelCount = 2;
    QVERIFY(AudioPeakCache::write(sourcePath, written));

    AudioPeakCacheEntry readBack;
    QVERIFY(AudioPeakCache::read(sourcePath, readBack));
    QCOMPARE(readBack.peaks, written.peaks);
    QCOMPARE(readBack.durationMs, written.durationMs);
    QCOMPARE(readBack.sampleRateHz, written.sampleRateHz);
    QCOMPARE(readBack.channelCount, written.channelCount);
}

void TestAudioPeakCache::rejectsStaleCacheWhenSourceChanges() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString sourcePath = tempDir.filePath(QStringLiteral("track.wav"));
    QFile sourceFile(sourcePath);
    QVERIFY(sourceFile.open(QIODevice::WriteOnly));
    sourceFile.write("version-one");
    sourceFile.close();

    AudioPeakCacheEntry written;
    written.peaks.resize(AudioConstants::kPeakBucketCount, 0.5F);
    written.durationMs = 5000;
    written.sampleRateHz = 48000;
    written.channelCount = 2;
    QVERIFY(AudioPeakCache::write(sourcePath, written));

    QVERIFY(sourceFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    sourceFile.write("version-two-longer");
    sourceFile.close();

    AudioPeakCacheEntry readBack;
    QVERIFY(!AudioPeakCache::read(sourcePath, readBack));
}

QTEST_MAIN(TestAudioPeakCache)
