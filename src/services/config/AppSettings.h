#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include "ImportTypes.h"
#include "MediaFile.h"

#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

struct ImportExtensionCategory {
    QString key{};
    QString label{};
    MediaKind mediaKind{MediaKind::Unknown};
    QStringList extensions{};
};

class AppSettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString managedStorageRoot READ managedStorageRoot WRITE setManagedStorageRoot NOTIFY
                   settingsChanged)
    Q_PROPERTY(QString configuredManagedStorageRoot READ configuredManagedStorageRoot NOTIFY
                   settingsChanged)
    Q_PROPERTY(
        QString defaultManagedStoragePath READ defaultManagedStoragePath NOTIFY settingsChanged)
    Q_PROPERTY(bool hasConfiguredManagedStorageRoot READ hasConfiguredManagedStorageRoot NOTIFY
                   settingsChanged)
    Q_PROPERTY(QString storageStrategy READ storageStrategy WRITE setStorageStrategy NOTIFY
                   settingsChanged)
    Q_PROPERTY(QStringList allowedExtensions READ allowedExtensions NOTIFY settingsChanged)
    Q_PROPERTY(QStringList fileDialogNameFilters READ fileDialogNameFilters NOTIFY settingsChanged)
    Q_PROPERTY(QString uiLanguage READ uiLanguage WRITE setUiLanguage NOTIFY settingsChanged)
    Q_PROPERTY(QUrl settingsFileLocation READ settingsFileLocation CONSTANT)

  public:
    struct Keys {
        static constexpr auto ManagedStorageRoot = "storage/managedRoot";
        static constexpr auto StorageStrategy = "storage/strategy";
        static constexpr auto ExtensionCategoryPrefix = "import/extensions/";
        static constexpr auto UiLanguage = "localization/uiLanguage";
        static constexpr auto LegacyUiLanguage = "general/uiLanguage";
    };

    explicit AppSettings(QObject *parent = nullptr);
    explicit AppSettings(const QString &settingsFilePath, QObject *parent = nullptr);

    [[nodiscard]] static QString databasePath();
    [[nodiscard]] static bool databaseExists();
    [[nodiscard]] static QList<ImportExtensionCategory> defaultExtensionCategories();

    [[nodiscard]] QString managedStorageRoot() const;
    [[nodiscard]] QString configuredManagedStorageRoot() const;
    [[nodiscard]] bool hasConfiguredManagedStorageRoot() const;
    [[nodiscard]] QString defaultManagedStoragePath() const;
    void setManagedStorageRoot(const QString &path);

    Q_INVOKABLE bool createStorageDirectory(const QString &path);

    [[nodiscard]] QString storageStrategy() const;
    void setStorageStrategy(const QString &strategy);
    [[nodiscard]] StorageStrategy storageStrategyEnum() const;

    [[nodiscard]] QStringList allowedExtensions() const;
    [[nodiscard]] QStringList fileDialogNameFilters() const;
    [[nodiscard]] const QList<ImportExtensionCategory> &extensionCategories() const;

    [[nodiscard]] QString uiLanguage() const;
    void setUiLanguage(const QString &languageCode);
    [[nodiscard]] QUrl settingsFileLocation() const;

    [[nodiscard]] MediaKind mediaKindForExtension(const QString &extension) const;
    [[nodiscard]] bool isExtensionAllowed(const QString &extension) const;

    Q_INVOKABLE void ensureDefaults();
    Q_INVOKABLE void setExtensionCategory(const QString &categoryKey,
                                          const QStringList &extensions);
    Q_INVOKABLE void resetExtensionCategoriesToDefaults();
    Q_INVOKABLE void saveConfiguration();
    Q_INVOKABLE void reload();

    Q_INVOKABLE QVariantList defaultExtensionCategoriesForUi() const;
    Q_INVOKABLE QVariantList extensionCategoriesForUi() const;
    Q_INVOKABLE void applyExtensionCategoriesFromUi(const QVariantList &categories);

  signals:
    void settingsChanged();

  private:
    [[nodiscard]] QString defaultManagedStorageRoot() const;
    void migrateLegacyNativeSettingsFile();
    void migrateLegacyUiLanguageKey();
    void migrateLegacyManagedStorageRoot();
    void writeMissingDefaultExtensionCategories();
    void loadExtensionCategories();

    QSettings m_settings{};
    QList<ImportExtensionCategory> m_categories{};
};

#endif // APPSETTINGS_H
