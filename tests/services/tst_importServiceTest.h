#ifndef TST_IMPORTSERVICETEST_H
#define TST_IMPORTSERVICETEST_H

#include <QTemporaryDir>
#include <QTest>
#include <memory>

#include "AppSettings.h"
#include "ImportService.h"
#include "SettingsConfigProvider.h"
#include "SqliteArtistRepository.h"
#include "SqliteConnection.h"
#include "SqliteMediaFileRepository.h"
#include "SqliteSongRepository.h"
#include "SqliteTuningRepository.h"

class TestImportService : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void testImportGuitarProFile();
    void testImportDuplicateIsSkipped();
    void testImportUnsupportedExtensionFails();
    void testImportDirectoryCollectsSupportedFiles();
    void testImportDirectoryEmitsProgress();
    void testImportPathsImportsMultipleFiles();
    void testImportPathsCollectsFromDirectory();
    void testImportPathsSetsBusy();
    void testImportFinishedClearsBusyBeforeSignal();
    void testImportWithCopyStrategyCreatesManagedFile();
    void testImportDirectoryCopiesIntoNamedSubfolder();
    void testImportDirectoryStoresRelativePaths();
    void testImportPathsStoresRelativePathsForMultipleSubfolders();
    void testCollectSupportedFilesPreservesMultipleSubfolderPaths();
    void testImportAudioFileHasNoVideoFlag();

  private:
    void createImportService();

    LibraryManager m_libraryManager;

    SqliteConnection m_connector{"ImportServiceTestDb"};
    SqliteArtistRepository m_artistRepo{m_connector};
    SqliteTuningRepository m_tuningRepo{m_connector};
    SqliteSongRepository m_songRepo{m_connector};
    SqliteMediaFileRepository m_mediaFileRepo{m_connector};

    std::unique_ptr<QTemporaryDir> m_dbDir;
    QString m_dbPath;
    QString m_settingsPath;
    std::unique_ptr<AppSettings> m_appSettings;
    std::unique_ptr<SettingsConfigProvider> m_config;
    std::unique_ptr<ImportService> m_importService;

    QString m_testGp3Path;
};

#endif // TST_IMPORTSERVICETEST_H
