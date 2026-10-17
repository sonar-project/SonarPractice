#include "tst_appSettingsTest.h"

#include "AppSettings.h"
#include "MediaFile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
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
    QCOMPARE(settings.mediaKindForExtension(QStringLiteral("mid")), MediaKind::Unknown);
    QCOMPARE(settings.mediaKindForExtension(QStringLiteral("midi")), MediaKind::Unknown);
}

void TestAppSettings::testMidiExtensionIsNotAudio() {
    {
        QSettings stored(m_settingsPath, QSettings::IniFormat);
        stored.setValue(QStringLiteral("import/extensions/audio"),
                        QStringLiteral("mp3;mid;midi;wav"));
        stored.sync();
    }

    AppSettings settings(m_settingsPath);
    settings.ensureDefaults();
    settings.reload();

    QVERIFY(!settings.allowedExtensions().contains(QStringLiteral("mid")));
    QVERIFY(!settings.allowedExtensions().contains(QStringLiteral("midi")));
    QCOMPARE(settings.mediaKindForExtension(QStringLiteral("mid")), MediaKind::Unknown);
    QCOMPARE(settings.mediaKindForExtension(QStringLiteral("mp3")), MediaKind::Audio);
    QCOMPARE(settings.mediaKindForExtension(QStringLiteral("wav")), MediaKind::Audio);
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
    QVariantList categories;
    QVariantMap audio;
    audio.insert(QStringLiteral("key"), QStringLiteral("audio"));
    audio.insert(QStringLiteral("extensions"), QStringLiteral("opus"));
    categories.append(audio);
    settings.applyExtensionCategoriesFromUi(categories);
    settings.saveConfiguration();

    const QStringList extensions = settings.allowedExtensions();
    QVERIFY(extensions.contains(QStringLiteral("opus")));
    QVERIFY(!extensions.contains(QStringLiteral("mp3")));
}

void TestAppSettings::testResetExtensionCategoriesToDefaults() {
    AppSettings settings(m_settingsPath);
    QVariantList custom;
    QVariantMap audio;
    audio.insert(QStringLiteral("key"), QStringLiteral("audio"));
    audio.insert(QStringLiteral("extensions"), QStringLiteral("opus"));
    custom.append(audio);
    settings.applyExtensionCategoriesFromUi(custom);

    settings.applyExtensionCategoriesFromUi(settings.defaultExtensionCategoriesForUi());
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

void TestAppSettings::testSoundFontPathDefaultsToBuiltIn() {
    AppSettings settings(m_settingsPath);
    QVERIFY(settings.soundFontPath().isEmpty());
    QVERIFY(settings.usesBuiltInSoundFont());
    QVERIFY(settings.effectiveSoundFontPath().isEmpty());
}

void TestAppSettings::testSoundFontPathRoundtrip() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString soundFontPath = tempDir.filePath(QStringLiteral("GeneralUser.sf2"));
    QFile file(soundFontPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("SF2");
    file.close();

    AppSettings settings(m_settingsPath);
    settings.setSoundFontPath(soundFontPath);
    settings.saveConfiguration();

    AppSettings reloaded(m_settingsPath);
    QCOMPARE(reloaded.soundFontPath(), soundFontPath);
    QCOMPARE(reloaded.effectiveSoundFontPath(), soundFontPath);
    QVERIFY(!reloaded.usesBuiltInSoundFont());
}

void TestAppSettings::testInvalidSoundFontExtensionRejected() {
    AppSettings settings(m_settingsPath);
    settings.setSoundFontPath(QStringLiteral("/tmp/sample.wav"));
    QVERIFY(settings.soundFontPath().isEmpty());
    QVERIFY(settings.usesBuiltInSoundFont());
}

void TestAppSettings::testUsesBuiltInWhenSoundFontFileMissing() {
    AppSettings settings(m_settingsPath);
    settings.setSoundFontPath(QStringLiteral("/tmp/does-not-exist.sf2"));
    QCOMPARE(settings.soundFontPath(), QStringLiteral("/tmp/does-not-exist.sf2"));
    QVERIFY(settings.usesBuiltInSoundFont());
    QVERIFY(settings.effectiveSoundFontPath().isEmpty());
}

void TestAppSettings::testSoundFontDirectoryUsesAppData() {
    const QString directory = AppSettings::soundFontDirectory();
    const QString appData =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QVERIFY(!directory.isEmpty());
    QVERIFY(directory.startsWith(appData));
    QVERIFY(directory.endsWith(QStringLiteral("soundfonts")));
}

void TestAppSettings::testInstallSoundFontCopiesAndKeepsSource() {
    QTemporaryDir sourceDir;
    QTemporaryDir storageDir;
    QVERIFY(sourceDir.isValid());
    QVERIFY(storageDir.isValid());

    const QString sourcePath = sourceDir.filePath(QStringLiteral("GeneralUser.sf2"));
    QFile sourceFile(sourcePath);
    QVERIFY(sourceFile.open(QIODevice::WriteOnly));
    sourceFile.write("SF2-BYTES");
    sourceFile.close();

    AppSettings settings(m_settingsPath);
    const QString installed = settings.installSoundFontInto(sourcePath, storageDir.path());
    QVERIFY(!installed.isEmpty());
    QCOMPARE(QFileInfo(installed).fileName(), QStringLiteral("GeneralUser.sf2"));
    QVERIFY(QFileInfo::exists(sourcePath));
    QVERIFY(installed.startsWith(QFileInfo(storageDir.path()).canonicalFilePath()));
    QCOMPARE(settings.soundFontPath(), installed);
    QCOMPARE(settings.effectiveSoundFontPath(), installed);

    QFile copied(installed);
    QVERIFY(copied.open(QIODevice::ReadOnly));
    QCOMPARE(copied.readAll(), QByteArray("SF2-BYTES"));

    QCOMPARE(settings.installSoundFontInto(sourcePath, storageDir.path()), installed);
    QCOMPARE(QDir(storageDir.path()).entryList(QDir::Files),
             QStringList{QStringLiteral("GeneralUser.sf2")});
}

void TestAppSettings::testInstallSoundFontReplacesPreviousCopy() {
    QTemporaryDir sourceDir;
    QTemporaryDir storageDir;
    QVERIFY(sourceDir.isValid());
    QVERIFY(storageDir.isValid());

    const QString firstSource = sourceDir.filePath(QStringLiteral("first.sf2"));
    const QString secondSource = sourceDir.filePath(QStringLiteral("second.sf3"));
    QFile firstFile(firstSource);
    QVERIFY(firstFile.open(QIODevice::WriteOnly));
    firstFile.write("ONE");
    firstFile.close();
    QFile secondFile(secondSource);
    QVERIFY(secondFile.open(QIODevice::WriteOnly));
    secondFile.write("TWO");
    secondFile.close();

    AppSettings settings(m_settingsPath);
    const QString firstInstalled = settings.installSoundFontInto(firstSource, storageDir.path());
    QVERIFY(QFileInfo::exists(firstInstalled));
    const QString secondInstalled = settings.installSoundFontInto(secondSource, storageDir.path());
    QCOMPARE(QFileInfo(secondInstalled).fileName(), QStringLiteral("second.sf3"));
    QVERIFY(!QFileInfo::exists(firstInstalled));
    QVERIFY(QFileInfo::exists(firstSource));
    QCOMPARE(settings.effectiveSoundFontPath(), secondInstalled);
}

void TestAppSettings::testInstallSoundFontRejectsInvalidOrMissing() {
    QTemporaryDir storageDir;
    QVERIFY(storageDir.isValid());

    AppSettings settings(m_settingsPath);
    QVERIFY(settings.installSoundFontInto(QStringLiteral("/tmp/sample.wav"), storageDir.path())
                .isEmpty());
    QVERIFY(settings.installSoundFontInto(storageDir.filePath(QStringLiteral("missing.sf2")),
                                           storageDir.path())
                .isEmpty());
    QVERIFY(settings.soundFontPath().isEmpty());
    QVERIFY(QDir(storageDir.path()).entryList(QDir::Files).isEmpty());
}

QTEST_MAIN(TestAppSettings)
