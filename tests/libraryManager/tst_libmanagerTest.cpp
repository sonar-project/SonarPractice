#include "tst_libmanagerTest.h"

#include <LibraryManager.h>

#include <QDir>
#include <QFileInfo>
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
