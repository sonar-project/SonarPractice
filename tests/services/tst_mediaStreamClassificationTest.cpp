#include "tst_mediaStreamClassificationTest.h"

#include "MediaStreamProbe.h"

#include <QTest>

void TestMediaStreamClassification::testClassifyAudioOnlyContainer() {
    MediaStreamProbeResult result;
    result.hasAudio = true;
    result.probed = true;

    QCOMPARE(classifyFromProbe(result, MediaKind::Video), MediaKind::Audio);
}

void TestMediaStreamClassification::testClassifyVideoContainer() {
    MediaStreamProbeResult result;
    result.hasAudio = true;
    result.hasVideo = true;
    result.probed = true;

    QCOMPARE(classifyFromProbe(result, MediaKind::Video), MediaKind::Video);
}

void TestMediaStreamClassification::testClassifyVideoOnlyContainer() {
    MediaStreamProbeResult result;
    result.hasVideo = true;
    result.probed = true;

    QCOMPARE(classifyFromProbe(result, MediaKind::Video), MediaKind::Video);
}

void TestMediaStreamClassification::testClassifyNeitherStreamUsesDefault() {
    MediaStreamProbeResult result;
    result.probed = true;

    QCOMPARE(classifyFromProbe(result, MediaKind::Video), MediaKind::Video);
    QCOMPARE(classifyFromProbe(result, MediaKind::Unknown), MediaKind::Unknown);
}

void TestMediaStreamClassification::testIsContainerExtension() {
    QVERIFY(MediaStreamProbe::isContainerExtension(QStringLiteral("mp4")));
    QVERIFY(MediaStreamProbe::isContainerExtension(QStringLiteral("MKV")));
    QVERIFY(!MediaStreamProbe::isContainerExtension(QStringLiteral("mp3")));
    QVERIFY(!MediaStreamProbe::isContainerExtension(QStringLiteral("gp5")));
}

QTEST_MAIN(TestMediaStreamClassification)
