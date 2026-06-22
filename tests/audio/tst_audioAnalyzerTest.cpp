#include "tst_audioAnalyzerTest.h"

#include "AudioAnalyzer.h"

#include <QDir>
#include <QFileInfo>
#include <QTest>

void TestAudioAnalyzer::jsonOutputPathReplacesExtension() {
    const QString mp3Path = QDir(QStringLiteral("music")).filePath(QStringLiteral("track.mp3"));
    QCOMPARE(AudioAnalyzer::jsonOutputPathFor(mp3Path),
             QDir(QStringLiteral("music")).filePath(QStringLiteral("track.json")));
}

void TestAudioAnalyzer::analyzeRejectsMissingFile() {
    AudioAnalyzer analyzer;
    const auto result = analyzer.analyze(QStringLiteral("/path/does/not/exist.mp3"));
    QVERIFY(!result.success);
    QVERIFY(!result.errorMessage.isEmpty());
    QVERIFY(result.notes.isEmpty());
}

void TestAudioAnalyzer::analyzeAndSaveRejectsMissingFile() {
    AudioAnalyzer analyzer;
    QString errorMessage;
    QVERIFY(!analyzer.analyzeAndSave(QStringLiteral("/path/does/not/exist.mp3"), errorMessage));
    QVERIFY(!errorMessage.isEmpty());
}

QTEST_MAIN(TestAudioAnalyzer)
