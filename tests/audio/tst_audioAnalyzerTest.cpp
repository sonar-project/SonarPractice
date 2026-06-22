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

void TestAudioAnalyzer::guitarTabMapsDropDAndDStandard() {
    const GuitarTabLayout dropD =
        GuitarTabLayout::fromTuningName(QStringLiteral("Drop D"), GuitarTabLayout::Instrument::Guitar);
    const double lowDHz = dropD.frequencyForPosition(5, 0);
    const TabPosition openLowD = dropD.positionForFrequency(lowDHz);
    QVERIFY(openLowD.isValid());
    QCOMPARE(openLowD.stringIndex, 5);
    QCOMPARE(openLowD.fret, 0);

    const double lowEHz = dropD.frequencyForPosition(5, 2);
    const TabPosition secondFretLow = dropD.positionForFrequency(lowEHz);
    QVERIFY(secondFretLow.isValid());
    QCOMPARE(secondFretLow.stringIndex, 5);
    QCOMPARE(secondFretLow.fret, 2);

    const GuitarTabLayout dStandard = GuitarTabLayout::fromTuningName(
        QStringLiteral("D G C F A D"), GuitarTabLayout::Instrument::Guitar);
    QCOMPARE(dStandard.stringLabels().constLast(), QStringLiteral("D"));
    const TabPosition dStandardOpenLow =
        dStandard.positionForFrequency(dStandard.frequencyForPosition(5, 0));
    QCOMPARE(dStandardOpenLow.fret, 0);
    QCOMPARE(dStandardOpenLow.stringIndex, 5);
}

void TestAudioAnalyzer::guitarTabPrefersThickStringsForPowerChordRoot() {
    const GuitarTabLayout layout = GuitarTabLayout::standardGuitar();
    const double rootHz = layout.frequencyForPosition(4, 7);
    const TabPosition position = layout.positionForFrequency(rootHz);
    QVERIFY(position.isValid());
    QCOMPARE(position.stringIndex, 4);
    QCOMPARE(position.fret, 7);
}

QTEST_MAIN(TestAudioAnalyzer)
