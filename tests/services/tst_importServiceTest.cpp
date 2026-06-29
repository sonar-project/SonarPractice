#include "tst_importServiceTest.h"

#include "Artist.h"
#include "DatabaseSchema.h"
#include "FileImportUtils.h"
#include "PathResolver.h"
#include "Song.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTemporaryFile>

void TestImportService::initTestCase() {
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    m_settingsPath = tempFile.fileName();
    tempFile.close();
    QFile::remove(m_settingsPath);
}

void TestImportService::init() {
    m_dbDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dbDir->isValid());
    m_dbPath = m_dbDir->filePath(QStringLiteral("import_test.db"));

    m_appSettings = std::make_unique<AppSettings>(m_settingsPath);
    m_appSettings->ensureDefaults();
    m_appSettings->setStorageStrategy(QStringLiteral("link"));
    m_appSettings->saveConfiguration();
    m_appSettings->reload();

    QVERIFY(m_connector.open(m_dbPath));

    DatabaseSchema schema(m_connector);
    QVERIFY(schema.createAllTables());

    createImportService();

    m_testGp3Path = QStringLiteral(TESTDATA_DIR) + QStringLiteral("/testfile.gp3");
    QVERIFY2(QFileInfo::exists(m_testGp3Path),
             qPrintable(QStringLiteral("Missing test GP3 file: %1").arg(m_testGp3Path)));
}

void TestImportService::createImportService() {
    m_config = std::make_unique<SettingsConfigProvider>(*m_appSettings);
    m_importService = std::make_unique<ImportService>(ImportService::Dependencies{
        m_artistRepo,
        m_tuningRepo,
        m_songRepo,
        m_mediaFileRepo,
        m_libraryManager,
        *m_config,
        *m_appSettings,
        m_dbPath,
    });
}

void TestImportService::cleanup() {
    m_importService.reset();
    m_config.reset();
    m_appSettings.reset();
    m_dbDir.reset();
    m_connector.close();
    QSqlDatabase::removeDatabase("ImportServiceTestDb");
}

void TestImportService::cleanupTestCase() { QFile::remove(m_settingsPath); }

void TestImportService::testImportGuitarProFile() {
    const ImportResult result = m_importService->importFile(m_testGp3Path);

    QCOMPARE(result.status, ImportStatus::Imported);
    QVERIFY(result.songId > 0);
    QVERIFY(result.mediaFileId > 0);

    const std::optional<Song> song = m_songRepo.getSong(result.songId);
    QVERIFY(song.has_value());
    QCOMPARE(song->title, QStringLiteral("Example File GP3"));

    const std::optional<Artist> artist = m_artistRepo.getArtist(song->artistId);
    QVERIFY(artist.has_value());
    QCOMPARE(artist->name, QStringLiteral("SonarPractice"));

    const std::optional<MediaFile> mediaFile = m_mediaFileRepo.getMediaFile(result.mediaFileId);
    QVERIFY(mediaFile.has_value());
    QCOMPARE(mediaFile->filePath, QFileInfo(m_testGp3Path).absoluteFilePath());
    QCOMPARE(mediaFile->sourceType, MediaSourceType::Local);
    QCOMPARE(mediaFile->isManaged, false);
    QVERIFY(mediaFile->canBePracticed);
    QCOMPARE(mediaFile->mediaKind, MediaKind::GuitarPro);
    QVERIFY(!mediaFile->fileHash.isEmpty());
}

void TestImportService::testImportDuplicateIsSkipped() {
    const ImportResult first = m_importService->importFile(m_testGp3Path);
    QCOMPARE(first.status, ImportStatus::Imported);

    const ImportResult second = m_importService->importFile(m_testGp3Path);
    QCOMPARE(second.status, ImportStatus::Skipped);
}

void TestImportService::testImportUnsupportedExtensionFails() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString txtPath = tempDir.filePath(QStringLiteral("unsupported.xyz"));
    QFile file(txtPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("not supported");
    file.close();

    const ImportResult result = m_importService->importFile(txtPath);
    QCOMPARE(result.status, ImportStatus::Failed);
}

void TestImportService::testImportDirectoryCollectsSupportedFiles() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString gpCopy = tempDir.filePath(QStringLiteral("song.gp3"));
    QVERIFY(QFile::copy(m_testGp3Path, gpCopy));

    const QStringList files =
        FileImportUtils::collectSupportedFiles(tempDir.path(), m_config->allowedFileTypes());

    QCOMPARE(files.size(), 1);
    QCOMPARE(files.first(), QFileInfo(gpCopy).absoluteFilePath());
}

void TestImportService::testImportDirectoryEmitsProgress() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString gpCopy = tempDir.filePath(QStringLiteral("song.gp3"));
    QVERIFY(QFile::copy(m_testGp3Path, gpCopy));

    QSignalSpy progressSpy(m_importService.get(), &ImportService::importProgress);
    QSignalSpy finishedSpy(m_importService.get(), &ImportService::importFinished);

    m_importService->importDirectory(tempDir.path());

    QVERIFY(finishedSpy.wait(30000));
    QCOMPARE(progressSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 1);

    const ImportSummary summary = finishedSpy.at(0).at(0).value<ImportSummary>();
    QCOMPARE(summary.importedCount, 1);
}

void TestImportService::testImportPathsImportsMultipleFiles() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString gpCopyA = tempDir.filePath(QStringLiteral("song-a.gp3"));
    const QString gpCopyB = tempDir.filePath(QStringLiteral("song-b.gp3"));
    QVERIFY(QFile::copy(m_testGp3Path, gpCopyA));
    QVERIFY(QFile::copy(m_testGp3Path, gpCopyB));

    QSignalSpy finishedSpy(m_importService.get(), &ImportService::importFinished);

    m_importService->importPaths({gpCopyA, gpCopyB});

    QVERIFY(finishedSpy.wait(30000));
    QCOMPARE(finishedSpy.count(), 1);

    const ImportSummary summary = finishedSpy.at(0).at(0).value<ImportSummary>();
    QCOMPARE(summary.importedCount, 1);
    QCOMPARE(summary.skippedCount, 1);
    QVERIFY(!m_importService->isBusy());
}

void TestImportService::testImportPathsCollectsFromDirectory() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString gpCopy = tempDir.filePath(QStringLiteral("folder-song.gp3"));
    QVERIFY(QFile::copy(m_testGp3Path, gpCopy));

    QSignalSpy finishedSpy(m_importService.get(), &ImportService::importFinished);

    m_importService->importPaths({tempDir.path()});

    QVERIFY(finishedSpy.wait(30000));
    QCOMPARE(finishedSpy.count(), 1);

    const ImportSummary summary = finishedSpy.at(0).at(0).value<ImportSummary>();
    QCOMPARE(summary.importedCount, 1);
}

void TestImportService::testImportPathsSetsBusy() {
    QSignalSpy busySpy(m_importService.get(), &ImportService::busyChanged);
    QSignalSpy finishedSpy(m_importService.get(), &ImportService::importFinished);

    m_importService->importPaths({m_testGp3Path});

    QVERIFY(finishedSpy.wait(30000));
    QVERIFY(busySpy.count() >= 2);
    QVERIFY(!m_importService->isBusy());
}

void TestImportService::testImportFinishedClearsBusyBeforeSignal() {
    bool busyWhenFinished = true;
    const QMetaObject::Connection connection =
        connect(m_importService.get(), &ImportService::importFinished, this,
                [this, &busyWhenFinished]() { busyWhenFinished = m_importService->isBusy(); });

    QSignalSpy finishedSpy(m_importService.get(), &ImportService::importFinished);
    m_importService->importPaths({m_testGp3Path});

    QVERIFY(finishedSpy.wait(30000));
    QVERIFY(!busyWhenFinished);
    disconnect(connection);
}

void TestImportService::testImportWithCopyStrategyCreatesManagedFile() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString managedRoot = tempDir.filePath(QStringLiteral("managed"));
    m_appSettings->setManagedStorageRoot(managedRoot);
    m_appSettings->setStorageStrategy(QStringLiteral("copy"));
    m_appSettings->saveConfiguration();

    const ImportResult result = m_importService->importFile(m_testGp3Path, StorageStrategy::Copy);

    QCOMPARE(result.status, ImportStatus::Imported);

    const std::optional<MediaFile> mediaFile = m_mediaFileRepo.getMediaFile(result.mediaFileId);
    QVERIFY(mediaFile.has_value());
    QVERIFY(mediaFile->isManaged);
    QCOMPARE(QFileInfo(mediaFile->filePath).fileName(), QStringLiteral("testfile.gp3"));

    PathResolver resolver(managedRoot);
    const QString resolvedPath = resolver.resolve(*mediaFile);
    QVERIFY(QFileInfo::exists(resolvedPath));
}

void TestImportService::testImportWithCopyStrategySkipsWhenManagedFileAlreadyExists() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString managedRoot = tempDir.filePath(QStringLiteral("managed"));
    m_appSettings->setManagedStorageRoot(managedRoot);
    m_appSettings->setStorageStrategy(QStringLiteral("copy"));
    m_appSettings->saveConfiguration();
    createImportService();

    const ImportResult first = m_importService->importFile(m_testGp3Path, StorageStrategy::Copy);
    QCOMPARE(first.status, ImportStatus::Imported);

    const QString managedPath = QDir(managedRoot).filePath(QStringLiteral("testfile.gp3"));
    QVERIFY(QFileInfo::exists(managedPath));

    QVERIFY(m_songRepo.deleteSong(first.songId));
    QCOMPARE(m_songRepo.getAllSongs().size(), 0);

    const ImportResult second = m_importService->importFile(m_testGp3Path, StorageStrategy::Copy);
    QCOMPARE(second.status, ImportStatus::Skipped);
    QCOMPARE(QDir(managedRoot).entryList(QDir::Files), QStringList({QStringLiteral("testfile.gp3")}));
}

void TestImportService::testImportDirectoryCopiesIntoNamedSubfolder() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString gpDir = tempDir.filePath(QStringLiteral("GP"));
    QVERIFY(QDir().mkpath(gpDir));

    const QString gpCopy = gpDir + QStringLiteral("/song.gp3");
    QVERIFY(QFile::copy(m_testGp3Path, gpCopy));

    const QString managedRoot = tempDir.filePath(QStringLiteral("managed"));
    m_appSettings->setManagedStorageRoot(managedRoot);
    m_appSettings->setStorageStrategy(QStringLiteral("copy"));
    m_appSettings->saveConfiguration();

    QSignalSpy finishedSpy(m_importService.get(), &ImportService::importFinished);
    m_importService->importPaths({gpDir});

    QVERIFY(finishedSpy.wait(30000));

    const QList<MediaFile> mediaFiles =
        m_mediaFileRepo.getMediaFilesBySongId(m_songRepo.getAllSongs().constFirst().id);
    QVERIFY(!mediaFiles.isEmpty());

    const MediaFile &media = mediaFiles.constFirst();
    QCOMPARE(media.filePath, QStringLiteral("GP/song.gp3"));
    QCOMPARE(media.sourceRelativePath, QStringLiteral("GP/song.gp3"));

    PathResolver resolver(managedRoot);
    const QString resolvedPath = resolver.resolve(media);
    QVERIFY(QFileInfo::exists(resolvedPath));
    QCOMPARE(resolvedPath, QDir(managedRoot).filePath(QStringLiteral("GP/song.gp3")));
}

void TestImportService::testImportDirectoryStoresRelativePaths() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString nestedDir = tempDir.filePath(QStringLiteral("Course/Picking Licks"));
    QVERIFY(QDir().mkpath(nestedDir));

    const QString nestedGp3 = nestedDir + QStringLiteral("/lick.gp3");
    QVERIFY(QFile::copy(m_testGp3Path, nestedGp3));

    QSignalSpy finishedSpy(m_importService.get(), &ImportService::importFinished);
    m_importService->importDirectory(tempDir.path());
    QVERIFY(finishedSpy.wait(30000));

    const QList<MediaFile> songsMedia =
        m_mediaFileRepo.getMediaFilesBySongId(m_songRepo.getAllSongs().constFirst().id);
    QVERIFY(!songsMedia.isEmpty());

    bool foundRelativePath = false;
    for (const MediaFile &media : songsMedia) {
        if (media.sourceRelativePath.contains(QStringLiteral("Picking Licks"))) {
            foundRelativePath = true;
            QCOMPARE(media.importRoot, QDir(tempDir.path()).absolutePath());
            break;
        }
    }
    QVERIFY(foundRelativePath);
}

void TestImportService::testImportPathsStoresRelativePathsForMultipleSubfolders() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString bandADir = tempDir.filePath(QStringLiteral("BandA"));
    const QString bandBDir = tempDir.filePath(QStringLiteral("BandB"));
    QVERIFY(QDir().mkpath(bandADir));
    QVERIFY(QDir().mkpath(bandBDir));

    const QString gp3Path = QStringLiteral(TESTDATA_DIR) + QStringLiteral("/testfile.gp3");
    const QString gp4Path = QStringLiteral(TESTDATA_DIR) + QStringLiteral("/testfile.gp4");
    QVERIFY(QFile::copy(gp3Path, bandADir + QStringLiteral("/song-a.gp3")));
    QVERIFY(QFile::copy(gp4Path, bandBDir + QStringLiteral("/song-b.gp4")));

    QSignalSpy finishedSpy(m_importService.get(), &ImportService::importFinished);
    m_importService->importPaths({tempDir.path()});
    QVERIFY(finishedSpy.wait(30000));

    bool foundBandA = false;
    bool foundBandB = false;
    for (const Song &song : m_songRepo.getAllSongs()) {
        for (const MediaFile &media : m_mediaFileRepo.getMediaFilesBySongId(song.id)) {
            QVERIFY(!media.sourceRelativePath.contains(QLatin1Char('\\')));
            if (media.sourceRelativePath.contains(QStringLiteral("BandA"))) {
                foundBandA = true;
                QCOMPARE(media.importRoot, QDir(tempDir.path()).absolutePath());
            }
            if (media.sourceRelativePath.contains(QStringLiteral("BandB"))) {
                foundBandB = true;
                QCOMPARE(media.importRoot, QDir(tempDir.path()).absolutePath());
            }
        }
    }
    QVERIFY(foundBandA);
    QVERIFY(foundBandB);
}

void TestImportService::testCollectSupportedFilesPreservesMultipleSubfolderPaths() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString bandADir = tempDir.filePath(QStringLiteral("BandA"));
    const QString bandBDir = tempDir.filePath(QStringLiteral("BandB"));
    QVERIFY(QDir().mkpath(bandADir));
    QVERIFY(QDir().mkpath(bandBDir));

    const QString gp3Path = QStringLiteral(TESTDATA_DIR) + QStringLiteral("/testfile.gp3");
    const QString gp4Path = QStringLiteral(TESTDATA_DIR) + QStringLiteral("/testfile.gp4");
    QVERIFY(QFile::copy(gp3Path, bandADir + QStringLiteral("/song-a.gp3")));
    QVERIFY(QFile::copy(gp4Path, bandBDir + QStringLiteral("/song-b.gp4")));

    const QList<CollectedFile> collected = FileImportUtils::collectSupportedFilesWithPaths(
        tempDir.path(), m_config->allowedFileTypes());
    QCOMPARE(collected.size(), 2);

    QSet<QString> subfolders;
    const QString rootFolderName = QFileInfo(QDir(tempDir.path()).absolutePath()).fileName();
    for (const CollectedFile &file : collected) {
        QVERIFY(!file.sourceRelativePath.contains(QLatin1Char('\\')));
        QVERIFY(file.sourceRelativePath.startsWith(rootFolderName + QLatin1Char('/')));

        const QString pathInsideRoot =
            file.sourceRelativePath.mid(rootFolderName.size() + 1);
        const int slashIndex = pathInsideRoot.indexOf(QLatin1Char('/'));
        QVERIFY(slashIndex > 0);
        subfolders.insert(pathInsideRoot.left(slashIndex));
    }
    QCOMPARE(subfolders, QSet<QString>({QStringLiteral("BandA"), QStringLiteral("BandB")}));
}

void TestImportService::testImportAudioFileHasNoVideoFlag() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString mp3Path = tempDir.filePath(QStringLiteral("backing-track.mp3"));
    QFile file(mp3Path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("dummy audio content for import test");
    file.close();

    const ImportResult result = m_importService->importFile(mp3Path);
    QCOMPARE(result.status, ImportStatus::Imported);
    QVERIFY(result.mediaFileId > 0);

    const std::optional<MediaFile> mediaFile = m_mediaFileRepo.getMediaFile(result.mediaFileId);
    QVERIFY(mediaFile.has_value());
    QCOMPARE(mediaFile->mediaKind, MediaKind::Audio);
    QCOMPARE(mediaFile->hasVideo, false);
}

QTEST_MAIN(TestImportService)
