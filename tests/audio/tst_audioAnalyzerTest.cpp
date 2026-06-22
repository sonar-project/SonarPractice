#include "tst_audioAnalyzerTest.h"

#include "AudioAnalyzer.h"
#include "GuitarTabLayout.h"

#include <QFileInfo>
#include <QTemporaryDir>
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
    const auto result = analyzer.analyzeAndSave(QStringLiteral("/path/does/not/exist.mp3"));
    QVERIFY(!result.success);
    QVERIFY(!result.errorMessage.isEmpty());
}

void TestAudioAnalyzer::loadAndSaveRoundTripPreservesNotes() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString jsonPath = tempDir.filePath(QStringLiteral("test.json"));
    AudioAnalyzer::Note note;
    note.startSec = 0.5;
    note.endSec = 1.0;
    note.frequencyHz = 440.0;

    QString errorMessage;
    QVERIFY(AudioAnalyzer::saveNotesToJson(jsonPath, {note}, errorMessage));
    QVERIFY(errorMessage.isEmpty());

    const QVector<AudioAnalyzer::Note> loaded =
        AudioAnalyzer::loadNotesFromJson(jsonPath, errorMessage);
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(loaded.size(), 1);
    QCOMPARE(loaded.first().startSec, 0.5);
    QCOMPARE(loaded.first().endSec, 1.0);
    QCOMPARE(loaded.first().frequencyHz, 440.0);
    QCOMPARE(AudioAnalyzer::noteLabelFromFrequency(440.0), QStringLiteral("A4"));
}

void TestAudioAnalyzer::guitarTabMapsStandardTuning() {
    const GuitarTabLayout layout = GuitarTabLayout::standardGuitar();
    const TabPosition position = layout.positionForFrequency(440.0);
    QVERIFY(position.isValid());
    QCOMPARE(position.fret, 5);
    QCOMPARE(position.stringIndex, 0);
    QCOMPARE(GuitarTabLayout::fretText(position), QStringLiteral("5"));
}

QTEST_MAIN(TestAudioAnalyzer)
