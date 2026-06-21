#ifndef TST_APPSETTINGSTEST_H
#define TST_APPSETTINGSTEST_H

#include <QObject>
#include <QTest>

class TestAppSettings : public QObject {
    Q_OBJECT

  private slots:
    void init();
    void cleanup();

    void testDefaultManagedStorageNotSongsFolder();
    void testManagedStorageRootNotPersistedByEnsureDefaults();
    void testLegacyManagedStorageRootMigration();
    void testCreateStorageDirectory();
    void testAllowedExtensionsFromSettings();
    void testMediaKindResolvedFromSettings();
    void testFileDialogFiltersGenerated();
    void testStorageStrategyRoundtrip();
    void testCustomExtensionsOverrideDefaults();
    void testResetExtensionCategoriesToDefaults();
    void testDatabaseExistsReflectsFile();
    void testUiLanguageRoundtrip();
    void testUiLanguageEmptyWhenUnset();
    void testLegacyUiLanguageMigration();

  private:
    QString m_settingsPath;
};

#endif // TST_APPSETTINGSTEST_H
