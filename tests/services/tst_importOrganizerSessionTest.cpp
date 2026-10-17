#include "tst_importOrganizerSessionTest.h"

#include "AppSettings.h"
#include "ApplicationErrorLog.h"
#include "DatabaseSchema.h"
#include "ImportOrganizerSession.h"
#include "ImportService.h"
#include "LibraryManager.h"
#include "SettingsConfigProvider.h"
#include "SqliteArtistRepository.h"
#include "SqliteConnection.h"
#include "SqliteMediaFileRepository.h"
#include "SqliteSongRepository.h"
#include "SqliteTuningRepository.h"
#include "fnv1a.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>

namespace {

    bool writeTextFile(const QString &path, const QByteArray &content) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return false;
        }
        return file.write(content) == content.size();
    }

    qint64 findSourceNodeByName(const ImportOrganizerSession &session, const QString &name) {
        for (const auto &row : session.sourceFlatRows()) {
            if (row.name == name) {
                return row.nodeId;
            }
        }
        return ImportOrganizerSession::kInvalidNodeId;
    }

    qint64 findTargetNodeByName(const ImportOrganizerSession &session, const QString &name) {
        for (const auto &row : session.targetFlatRows()) {
            if (row.name == name) {
                return row.nodeId;
            }
        }
        return ImportOrganizerSession::kInvalidNodeId;
    }

} // namespace

void TestImportOrganizerSession::testScanAndMapSingleFileToFolder() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString filePath = tempDir.filePath(QStringLiteral("Agent Brutality.gp3"));
    QVERIFY(writeTextFile(filePath, QByteArrayLiteral("gp-content-a")));

    ImportOrganizerSession session;
    session.reset();
    session.setManagedRootName(QStringLiteral("Library"));
    QVERIFY(session.buildSourceFromScan({filePath}, {QStringLiteral("gp3")}));

    const qint64 kursId = session.addTargetFolder(ImportOrganizerSession::kRootNodeId,
                                                  QStringLiteral("Kurse"));
    QVERIFY(kursId > 0);
    const qint64 artistId = session.addTargetFolder(kursId, QStringLiteral("Kreator"));
    QVERIFY(artistId > 0);

    const qint64 sourceId = findSourceNodeByName(session, QStringLiteral("Agent Brutality.gp3"));
    QVERIFY(sourceId > 0);
    QVERIFY(session.mapNodes({sourceId}, artistId));

    const auto planned = session.collectPlannedEntries();
    QCOMPARE(planned.size(), 1);
    QCOMPARE(planned.front().sourceRelativePath,
             QStringLiteral("Kurse/Kreator/Agent Brutality.gp3"));
    QCOMPARE(planned.front().absolutePath, QFileInfo(filePath).absoluteFilePath());
}

void TestImportOrganizerSession::testMapFolderPreservesStructure() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString courseDir = tempDir.filePath(QStringLiteral("Dan Mumm"));
    const QString nested =
        courseDir + QStringLiteral("/Magnum Opus/Tabs/lesson1.gp3");
    QVERIFY(QDir().mkpath(QFileInfo(nested).absolutePath()));
    QVERIFY(writeTextFile(nested, QByteArrayLiteral("lesson-1")));

    ImportOrganizerSession session;
    session.reset();
    QVERIFY(session.buildSourceFromScan({courseDir}, {QStringLiteral("gp3")}));

    const qint64 kurse =
        session.addTargetFolder(ImportOrganizerSession::kRootNodeId, QStringLiteral("Kurse"));
    const qint64 sourceRoot = findSourceNodeByName(session, QStringLiteral("Dan Mumm"));
    QVERIFY(sourceRoot > 0);
    QVERIFY(session.mapNodes({sourceRoot}, kurse, true));

    const auto planned = session.collectPlannedEntries();
    QCOMPARE(planned.size(), 1);
    QCOMPARE(planned.front().sourceRelativePath,
             QStringLiteral("Kurse/Dan Mumm/Magnum Opus/Tabs/lesson1.gp3"));
}

void TestImportOrganizerSession::testMapContentsOnlyStripsWrapper() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString courseDir = tempDir.filePath(QStringLiteral("Guitar"));
    const QString nested = courseDir + QStringLiteral("/Dan Mumm/song.gp3");
    QVERIFY(QDir().mkpath(QFileInfo(nested).absolutePath()));
    QVERIFY(writeTextFile(nested, QByteArrayLiteral("song")));

    ImportOrganizerSession session;
    session.reset();
    QVERIFY(session.buildSourceFromScan({courseDir}, {QStringLiteral("gp3")}));

    const qint64 sourceRoot = findSourceNodeByName(session, QStringLiteral("Guitar"));
    QVERIFY(sourceRoot > 0);
    QVERIFY(session.mapNodes({sourceRoot}, ImportOrganizerSession::kRootNodeId, false));

    const auto planned = session.collectPlannedEntries();
    QCOMPARE(planned.size(), 1);
    QCOMPARE(planned.front().sourceRelativePath, QStringLiteral("Dan Mumm/song.gp3"));
}

void TestImportOrganizerSession::testBatchDuplicateChecksOnlyOne() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QByteArray content = QByteArrayLiteral("same-bytes");
    const QString a = tempDir.filePath(QStringLiteral("a/song.gp3"));
    const QString b = tempDir.filePath(QStringLiteral("b/song.gp3"));
    QVERIFY(QDir().mkpath(QFileInfo(a).absolutePath()));
    QVERIFY(QDir().mkpath(QFileInfo(b).absolutePath()));
    QVERIFY(writeTextFile(a, content));
    QVERIFY(writeTextFile(b, content));

    ImportOrganizerSession session;
    session.reset();
    QVERIFY(session.buildSourceFromScan({tempDir.path()}, {QStringLiteral("gp3")}));

    int checkedDuplicates = 0;
    int uncheckedDuplicates = 0;
    for (const auto &row : session.sourceFlatRows()) {
        if (row.isFolder || row.status != OrganizerEntryStatus::DuplicateInBatch) {
            continue;
        }
        if (row.checked) {
            ++checkedDuplicates;
        } else {
            ++uncheckedDuplicates;
        }
    }
    QCOMPARE(checkedDuplicates, 1);
    QCOMPARE(uncheckedDuplicates, 1);
}

void TestImportOrganizerSession::testAlreadyInDatabaseHiddenFromAllFilter() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(QStringLiteral("known.gp3"));
    QVERIFY(writeTextFile(path, QByteArrayLiteral("known")));
    const QString hash = FNV1a::calculate(path);
    QVERIFY(!hash.isEmpty());

    ImportOrganizerSession session;
    session.reset();
    session.setExistingHashes({hash});
    QVERIFY(session.buildSourceFromScan({path}, {QStringLiteral("gp3")}));

    session.setFilterMode(OrganizerFilterMode::All);
    QCOMPARE(session.sourceFlatRows().size(), 0);

    session.setFilterMode(OrganizerFilterMode::Errors);
    // Defect filter should not show AlreadyInDatabase
    QCOMPARE(session.sourceFlatRows().size(), 0);
}

void TestImportOrganizerSession::testUnmapRestoresSource() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(QStringLiteral("restore.gp3"));
    QVERIFY(writeTextFile(path, QByteArrayLiteral("restore-me")));

    ImportOrganizerSession session;
    session.reset();
    QVERIFY(session.buildSourceFromScan({path}, {QStringLiteral("gp3")}));
    const qint64 sourceId = findSourceNodeByName(session, QStringLiteral("restore.gp3"));
    QVERIFY(session.mapNodes({sourceId}, ImportOrganizerSession::kRootNodeId));
    QCOMPARE(session.plannedFileCount(), 1);
    QCOMPARE(session.sourceFileCount(), 0);

    const qint64 targetId = findTargetNodeByName(session, QStringLiteral("restore.gp3"));
    QVERIFY(targetId > 0);
    QVERIFY(session.unmapNodes({targetId}));
    QCOMPARE(session.plannedFileCount(), 0);
    QCOMPARE(session.sourceFileCount(), 1);
}

void TestImportOrganizerSession::testUsesOrganizerFalseForLink() {
    QTemporaryFile settingsFile;
    QVERIFY(settingsFile.open());
    const QString settingsPath = settingsFile.fileName();
    settingsFile.close();

    AppSettings settings(settingsPath);
    settings.ensureDefaults();
    settings.setStorageStrategy(QStringLiteral("link"));
    settings.saveConfiguration();
    settings.reload();

    QTemporaryDir dbDir;
    QVERIFY(dbDir.isValid());
    const QString dbPath = dbDir.filePath(QStringLiteral("org.db"));
    SqliteConnection connector(QStringLiteral("OrganizerUsesLinkDb"));
    QVERIFY(connector.open(dbPath));
    DatabaseSchema schema(connector);
    QVERIFY(schema.createAllTables());

    SqliteArtistRepository artistRepo(connector);
    SqliteTuningRepository tuningRepo(connector);
    SqliteSongRepository songRepo(connector);
    SqliteMediaFileRepository mediaRepo(connector);
    LibraryManager libraryManager;
    SettingsConfigProvider config(settings);
    ApplicationErrorLog errorLog(dbDir.filePath(QStringLiteral("err.log")));

    ImportService service(ImportService::Dependencies{
        artistRepo, tuningRepo, songRepo, mediaRepo, libraryManager, config, settings, dbPath,
        &errorLog});
    QVERIFY(!service.usesOrganizer());

    settings.setStorageStrategy(QStringLiteral("copy"));
    settings.saveConfiguration();
    settings.reload();
    QVERIFY(service.usesOrganizer());

    connector.close();
    QSqlDatabase::removeDatabase(QStringLiteral("OrganizerUsesLinkDb"));
}

void TestImportOrganizerSession::testImportEntriesUsesPlannedRelativePath() {
    QTemporaryFile settingsFile;
    QVERIFY(settingsFile.open());
    const QString settingsPath = settingsFile.fileName();
    settingsFile.close();

    QTemporaryDir storageDir;
    QVERIFY(storageDir.isValid());

    AppSettings settings(settingsPath);
    settings.ensureDefaults();
    settings.setStorageStrategy(QStringLiteral("copy"));
    settings.setManagedStorageRoot(storageDir.path());
    settings.saveConfiguration();
    settings.reload();

    QTemporaryDir dbDir;
    QVERIFY(dbDir.isValid());
    const QString dbPath = dbDir.filePath(QStringLiteral("planned.db"));
    SqliteConnection connector(QStringLiteral("OrganizerPlannedDb"));
    QVERIFY(connector.open(dbPath));
    DatabaseSchema schema(connector);
    QVERIFY(schema.createAllTables());

    SqliteArtistRepository artistRepo(connector);
    SqliteTuningRepository tuningRepo(connector);
    SqliteSongRepository songRepo(connector);
    SqliteMediaFileRepository mediaRepo(connector);
    LibraryManager libraryManager;
    SettingsConfigProvider config(settings);
    ApplicationErrorLog errorLog(dbDir.filePath(QStringLiteral("err.log")));

    ImportService service(ImportService::Dependencies{
        artistRepo, tuningRepo, songRepo, mediaRepo, libraryManager, config, settings, dbPath,
        &errorLog});

    const QString sourcePath = dbDir.filePath(QStringLiteral("Agent Brutality.gp3"));
    // Use testdata GP3 if available so Guitar Pro parse succeeds; otherwise a dummy may fail.
    const QString testGp3 =
        QStringLiteral(TESTDATA_DIR) + QStringLiteral("/testfile.gp3");
    QVERIFY(QFile::copy(testGp3, sourcePath));

    ImportFileEntry entry;
    entry.absolutePath = sourcePath;
    entry.sourceRelativePath = QStringLiteral("Kurse/Kreator/Agent Brutality.gp3");

    QSignalSpy finishedSpy(&service, &ImportService::importFinished);
    service.importEntries({entry}, StorageStrategy::Copy);
    QVERIFY(finishedSpy.wait(10000));
    QCOMPARE(service.lastImportedCount(), 1);

    const auto mediaFiles = mediaRepo.getAllMediaFiles();
    QCOMPARE(mediaFiles.size(), 1);
    QCOMPARE(mediaFiles.front().sourceRelativePath,
             QStringLiteral("Kurse/Kreator/Agent Brutality.gp3"));
    QCOMPARE(mediaFiles.front().filePath, QStringLiteral("Kurse/Kreator/Agent Brutality.gp3"));
    QVERIFY(QFileInfo::exists(storageDir.filePath(QStringLiteral("Kurse/Kreator/Agent Brutality.gp3"))));

    connector.close();
    QSqlDatabase::removeDatabase(QStringLiteral("OrganizerPlannedDb"));
}

QTEST_MAIN(TestImportOrganizerSession)
