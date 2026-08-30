#include "tst_libmanagerTest.h"

#include <AsciiTabRenderer.h>
#include <LibraryManager.h>

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTest>

namespace {

    QString testDataPath(const char *fileName) {
        QDir projectRoot(QFileInfo(QString::fromUtf8(__FILE__)).absolutePath());
        if (!projectRoot.cdUp() || !projectRoot.cdUp()) {
            return {};
        }
        return projectRoot.filePath(
            QStringLiteral("testdata/%1").arg(QString::fromLatin1(fileName)));
    }

    void verifyMetadata(const SongMetadata &meta, const QString &expectedArtist,
                        const QString &expectedTitle, const char *fileLabel) {
        QCOMPARE(meta.artist, expectedArtist);
        QCOMPARE(meta.title, expectedTitle);
        QVERIFY2(meta.bpm > 0, qPrintable(QStringLiteral("%1: expected BPM > 0, got %2")
                                              .arg(QString::fromLatin1(fileLabel))
                                              .arg(meta.bpm)));
        QVERIFY2(!meta.defaultUiTuning.isEmpty(),
                 qPrintable(QStringLiteral("%1: expected non-empty tuning")
                                .arg(QString::fromLatin1(fileLabel))));
    }

    void parseAndVerify(const char *fileName, const QString &expectedArtist,
                        const QString &expectedTitle, const char *fileLabel) {
        LibraryManager manager;
        const QString testFile = testDataPath(fileName);
        QVERIFY2(QFileInfo::exists(testFile),
                 qPrintable(QStringLiteral("Missing test file: %1").arg(testFile)));

        const std::optional<SongMetadata> result = manager.parseGuitarProFile(testFile);
        QVERIFY2(result.has_value(),
                 qPrintable(QStringLiteral("Parser returned no metadata for %1").arg(testFile)));

        verifyMetadata(*result, expectedArtist, expectedTitle, fileLabel);
    }

    void verifyAsciiPreview(const char *fileName, const QString &expectedTitle) {
        const QString testFile = testDataPath(fileName);
        QVERIFY2(QFileInfo::exists(testFile),
                 qPrintable(QStringLiteral("Missing test file: %1").arg(testFile)));

        const std::optional<AsciiTabRenderer::SongPreview> preview =
            AsciiTabRenderer::loadFromFile(testFile);
        QVERIFY2(preview.has_value(),
                 qPrintable(QStringLiteral("ASCII preview failed for %1").arg(testFile)));
        QCOMPARE(preview->title, expectedTitle);
        QVERIFY(!preview->tracks.empty());

        bool foundTab = false;
        for (const AsciiTabRenderer::TrackPreview &track : preview->tracks) {
            if (track.percussion) {
                continue;
            }
            QVERIFY(track.tabText.contains(QLatin1Char('|')));
            QVERIFY(track.tabText.contains(QLatin1Char('-')));
            // At least one fret digit somewhere in the preview.
            static const QRegularExpression digit(QStringLiteral("[0-9]"));
            QVERIFY2(track.tabText.contains(digit),
                     qPrintable(QStringLiteral("Expected fret digits in tab for track '%1':\n%2")
                                    .arg(track.name, track.tabText)));
            foundTab = true;
            break;
        }
        QVERIFY2(foundTab, "Expected at least one non-percussion track with tablature");
    }

} // namespace

QTEST_MAIN(TestLibManager)

void TestLibManager::testParseGPXFile() {
    parseAndVerify("testfile.gpx", QStringLiteral("SonarPractice"),
                   QStringLiteral("Example File 1"), "GPX");
}

void TestLibManager::testParseGP3File() {
    parseAndVerify("testfile.gp3", QStringLiteral("SonarPractice"),
                   QStringLiteral("Example File GP3"), "GP3");
}

void TestLibManager::testParseGP4File() {
    parseAndVerify("testfile.gp4", QStringLiteral("SonarPractice"),
                   QStringLiteral("Example File GP4"), "GP4");
}

void TestLibManager::testParseGP5File() {
    parseAndVerify("testfile.gp5", QStringLiteral("SonarPractice"),
                   QStringLiteral("Example File GP5"), "GP5");
}

void TestLibManager::testParseGPFile() {
    parseAndVerify("testfile.gp", QStringLiteral("SonarPractice"), QStringLiteral("Example File 1"),
                   "GP");
}

void TestLibManager::testAsciiTabPreviewGp5() {
    verifyAsciiPreview("testfile.gp5", QStringLiteral("Example File GP5"));
}

void TestLibManager::testAsciiTabPreviewGpx() {
    verifyAsciiPreview("testfile.gpx", QStringLiteral("Example File 1"));
}

void TestLibManager::testAsciiTabMissingFile() {
    const std::optional<AsciiTabRenderer::SongPreview> preview =
        AsciiTabRenderer::loadFromFile(QStringLiteral("/tmp/does-not-exist-sonar.gp5"));
    QVERIFY(!preview.has_value());
}
