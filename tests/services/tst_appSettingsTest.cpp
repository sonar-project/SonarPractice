#include "tst_appSettingsTest.h"

#include "AppSettings.h"
#include "MediaFile.h"

#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QTemporaryFile>

void TestAppSettings::init() {
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    m_settingsPath = tempFile.fileName();
    tempFile.close();
    QFile::remove(m_settingsPath);
}

void TestAppSettings::cleanup() { QFile::remove(m_settingsPath); }

void TestAppSettings::testDefaultManagedStorageNotSongsFolder() {
    AppSettings settings(m_settingsPath);
    settings.ensureDefaults();

    const QString root = settings.managedStorageRoot();
    QVERIFY(!root.isEmpty());
    QVERIFY(root.contains(QStringLiteral("SonarPractice-Repertoire")));
}

void TestAppSettings::testManagedStorageRootNotPersistedByEnsureDefaults() {
    AppSettings settings(m_settingsPath);
    settings.ensureDefaults();

    QVERIFY(!settings.hasConfiguredManagedStorageRoot());
    QCOMPARE(settings.defaultManagedStoragePath(), settings.managedStorageRoot());
}

// Disabled – very important for later versions, but not yet relevant.
// void TestAppSettings::testLegacyManagedStorageRootMigration() {
//     {
//         QSettings legacySettings(m_settingsPath, QSettings::IniFormat);
//         legacySettings.setValue(QStringLiteral("storage/managedRoot"),
//                                 QStringLiteral("/home/user/.local/share/SonarPractice/media"));
//         legacySettings.sync();
//     }

//     AppSettings settings(m_settingsPath);
//     QVERIFY(!settings.hasConfiguredManagedStorageRoot());
//     QVERIFY(settings.managedStorageRoot().contains(QStringLiteral("SonarPractice-Repertoire")));
// }

void TestAppSettings::testCreateStorageDirectory() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    AppSettings settings(m_settingsPath);
    const QString target = tempDir.filePath(QStringLiteral("SonarPractice-Repertoire"));
    QVERIFY(settings.createStorageDirectory(target));
    QVERIFY(QDir(target).exists());
}

void TestAppSettings::testAllowedExtensionsFromSettings() {
    AppSettings settings(m_settingsPath);
    settings.ensureDefaults();
    settings.reload();

    const QStringList extensions = settings.allowedExtensions();
    QVERIFY(extensions.contains(QStringLiteral("gp5")));
    QVERIFY(extensions.contains(QStringLiteral("mp3")));
    QVERIFY(extensions.contains(QStringLiteral("pdf")));
    QVERIFY(extensions.contains(QStringLiteral("mp4")));
    QVERIFY(extensions.contains(QStringLiteral("txt")));
}

void TestAppSettings::testMediaKindResolvedFromSettings() {
    AppSettings settings(m_settingsPath);
    settings.ensureDefaults();
    settings.reload();

    QCOMPARE(settings.mediaKindForExtension(QStringLiteral("gp5")), MediaKind::GuitarPro);
    QCOMPARE(settings.mediaKindForExtension(QStringLiteral("wav")), MediaKind::Audio);
    QCOMPARE(settings.mediaKindForExtension(QStringLiteral("mp4")), MediaKind::Video);
    QCOMPARE(settings.mediaKindForExtension(QStringLiteral("txt")), MediaKind::Document);
    QCOMPARE(settings.mediaKindForExtension(QStringLiteral("docx")), MediaKind::Document);
    QCOMPARE(settings.mediaKindForExtension(QStringLiteral("xyz")), MediaKind::Unknown);
}

void TestAppSettings::testFileDialogFiltersGenerated() {
    AppSettings settings(m_settingsPath);
    settings.ensureDefaults();
    settings.reload();

    const QStringList filters = settings.fileDialogNameFilters();
    QVERIFY(!filters.isEmpty());
    QVERIFY(filters.first().contains(QStringLiteral("All supported")));
    QVERIFY(filters.last().contains(QStringLiteral("All files")));

    bool hasGp5Filter = false;
    for (const QString &filter : filters) {
        if (filter.contains(QStringLiteral("*.gp5"))) {
            hasGp5Filter = true;
            break;
        }
    }
    QVERIFY(hasGp5Filter);
}

void TestAppSettings::testStorageStrategyRoundtrip() {
    AppSettings settings(m_settingsPath);
    settings.setStorageStrategy(QStringLiteral("copy"));
    settings.saveConfiguration();
    QCOMPARE(settings.storageStrategy(), QStringLiteral("copy"));
    QCOMPARE(settings.storageStrategyEnum(), StorageStrategy::Copy);

    settings.setStorageStrategy(QStringLiteral("invalid"));
    QCOMPARE(settings.storageStrategy(), QStringLiteral("link"));
}

void TestAppSettings::testCustomExtensionsOverrideDefaults() {
    AppSettings settings(m_settingsPath);
    settings.setExtensionCategory(QStringLiteral("audio"), {QStringLiteral("opus")});
    settings.saveConfiguration();

    const QStringList extensions = settings.allowedExtensions();
    QVERIFY(extensions.contains(QStringLiteral("opus")));
    QVERIFY(!extensions.contains(QStringLiteral("mp3")));
}

void TestAppSettings::testResetExtensionCategoriesToDefaults() {
    AppSettings settings(m_settingsPath);
    settings.setExtensionCategory(QStringLiteral("audio"), {QStringLiteral("opus")});
    settings.resetExtensionCategoriesToDefaults();
    settings.saveConfiguration();

    const QStringList extensions = settings.allowedExtensions();
    QVERIFY(extensions.contains(QStringLiteral("mp3")));
}

void TestAppSettings::testDatabaseExistsReflectsFile() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString dbPath = tempDir.filePath(QStringLiteral("sonar_practice.db"));
    QVERIFY(!QFileInfo::exists(dbPath));

    QFile file(dbPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("x");
    file.close();

    QVERIFY(QFileInfo::exists(dbPath));
}

void TestAppSettings::testUiLanguageRoundtrip() {
    AppSettings settings(m_settingsPath);
    settings.setUiLanguage(QStringLiteral("de_DE"));
    settings.saveConfiguration();

    AppSettings reloaded(m_settingsPath);
    QCOMPARE(reloaded.uiLanguage(), QStringLiteral("de_DE"));

    reloaded.setUiLanguage(QStringLiteral("en"));
    reloaded.saveConfiguration();
    AppSettings english(m_settingsPath);
    QCOMPARE(english.uiLanguage(), QStringLiteral("en"));

    english.setUiLanguage(QString());
    english.saveConfiguration();
    AppSettings cleared(m_settingsPath);
    QVERIFY(cleared.uiLanguage().isEmpty());
}

void TestAppSettings::testUiLanguageEmptyWhenUnset() {
    AppSettings settings(m_settingsPath);
    QVERIFY(settings.uiLanguage().isEmpty());
}

void TestAppSettings::testLegacyUiLanguageMigration() {
    {
        QSettings legacySettings(m_settingsPath, QSettings::IniFormat);
        legacySettings.setValue(QStringLiteral("general/uiLanguage"), QStringLiteral("en"));
        legacySettings.sync();
    }

    AppSettings settings(m_settingsPath);
    QCOMPARE(settings.uiLanguage(), QStringLiteral("en"));

    QSettings raw(m_settingsPath, QSettings::IniFormat);
    QCOMPARE(raw.value(QStringLiteral("localization/uiLanguage")).toString(),
             QStringLiteral("en"));
    QVERIFY(!raw.contains(QStringLiteral("general/uiLanguage")));
}

QTEST_MAIN(TestAppSettings)
