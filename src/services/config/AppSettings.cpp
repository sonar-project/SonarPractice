#include "AppSettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>

namespace {

     /**
      * @brief Joins a list of file extensions into a semicolon-separated string.
      * @param[in] extensions List of file extensions to join.
      * @return Semicolon-separated string of extensions.
     */
    QString joinExtensions(const QStringList &extensions) {
        return extensions.join(QLatin1Char(';'));
    }

     /**
      * @brief Splits a semicolon-separated string into a list of normalized extensions.
      * @param[in] value Semicolon-separated string of extensions.
      * @return List of trimmed, lowercase extensions with empty entries removed.
     */
    QStringList splitExtensions(const QString &value) {
        QStringList parts = value.split(QLatin1Char(';'), Qt::SkipEmptyParts);
        for (QString &part : parts) {
            part = part.trimmed().toLower();
        }
        parts.removeAll(QString());
        return parts;
    }

    /**
     * @brief Normalizes a storage strategy string to canonical form.
     * @param[in] strategy Input strategy string (case-insensitive).
     * @return Normalized strategy: "copy", "move", or defaults to "link".
     */
    QString normalizeStrategy(const QString &strategy) {
        QString normalized = strategy.trimmed().toLower();
        if (normalized == QStringLiteral("copy") || normalized == QStringLiteral("move")) {
            return normalized;
        }
        return QStringLiteral("link");
    }

} // namespace

/**
 * @brief Constructs application settings with default configuration location.
 * @param[in] parent Parent QObject for ownership management.
 * @note Migrates legacy settings and ensures defaults on initialization.
 */
AppSettings::AppSettings(QObject *parent)
    : QObject(parent),
      m_settings(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::organizationName(),
                 QCoreApplication::applicationName()) {
    migrateLegacyUiLanguageKey();
    if (databaseExists()) {
        ensureDefaults();
    }
    reload();
}

/**
 * @brief Constructs application settings with custom configuration file.
 * @param[in] settingsFilePath Path to custom settings file.
 * @param[in] parent Parent QObject for ownership management.
 * @note Migrates legacy settings and ensures defaults on initialization.
 */
AppSettings::AppSettings(const QString &settingsFilePath, QObject *parent)
    : QObject(parent), m_settings(settingsFilePath, QSettings::IniFormat) {
    migrateLegacyUiLanguageKey();
    if (databaseExists()) {
        ensureDefaults();
    }
    reload();
}

/**
 * @brief Computes the full path to the application's SQLite database.
 * @return Absolute path to sonar_practice.db in AppLocalDataLocation.
 */
QString AppSettings::databasePath() {
    const QString appDataPath =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return QDir(appDataPath).filePath(QStringLiteral("sonar_practice.db"));
}

/**
 * @brief Checks if the application database file exists.
 * @return true if database file exists, false otherwise.
 */
bool AppSettings::databaseExists() { return QFileInfo::exists(databasePath()); }

/**
 * @brief Provides default file extension categories for import functionality.
 * @return List of predefined extension categories with labels and media kinds.
 */
QList<ImportExtensionCategory> AppSettings::defaultExtensionCategories() {
    return {
        {.key = QStringLiteral("guitarpro"),
         .label = tr("Guitar Pro"),
         .mediaKind = MediaKind::GuitarPro,
         .extensions = {QStringLiteral("gp"), QStringLiteral("gp3"), QStringLiteral("gp4"),
                        QStringLiteral("gp5"), QStringLiteral("gpx")}},
        {.key = QStringLiteral("audio"),
         .label = tr("Audio"),
         .mediaKind = MediaKind::Audio,
         .extensions = {QStringLiteral("wav"), QStringLiteral("wave"), QStringLiteral("aiff"),
                        QStringLiteral("aif"), QStringLiteral("mp3"), QStringLiteral("flac"),
                        QStringLiteral("ogg"), QStringLiteral("wma"), QStringLiteral("mid"),
                        QStringLiteral("m4a")}},
        {.key = QStringLiteral("video"),
         .label = tr("Video"),
         .mediaKind = MediaKind::Video,
         .extensions = {QStringLiteral("mp4"), QStringLiteral("m4v"), QStringLiteral("mov"),
                        QStringLiteral("mkv"), QStringLiteral("webm"), QStringLiteral("avi"),
                        QStringLiteral("wmv"), QStringLiteral("mpg"), QStringLiteral("mpeg"),
                        QStringLiteral("ogv"), QStringLiteral("3gp"), QStringLiteral("flv")}},
        {.key = QStringLiteral("image"),
         .label = tr("Images"),
         .mediaKind = MediaKind::Image,
         .extensions = {QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
                        QStringLiteral("webp")}},
        {.key = QStringLiteral("document"),
         .label = tr("Documents"),
         .mediaKind = MediaKind::Document,
         .extensions = {QStringLiteral("pdf"), QStringLiteral("txt"), QStringLiteral("md"),
                        QStringLiteral("rtf"), QStringLiteral("doc"), QStringLiteral("docx"),
                        QStringLiteral("odt"), QStringLiteral("ods"), QStringLiteral("odp"),
                        QStringLiteral("xls"), QStringLiteral("xlsx"), QStringLiteral("ppt"),
                        QStringLiteral("pptx")}},
    };
}

/**
 * @brief Computes the default managed storage root directory.
 * @return Absolute path to SonarPractice-Repertoire in user's home directory.
 */
QString AppSettings::defaultManagedStorageRoot() const {
    const QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    return QDir(home).filePath(QStringLiteral("SonarPractice-Repertoire"));
}

/**
  @brief Migrates legacy UI language setting to standardized key.
  @note Removes obsolete keys after migration.
 */
void AppSettings::migrateLegacyUiLanguageKey() {
    QString value = m_settings.value(Keys::UiLanguage).toString().trimmed();
    if (value.isEmpty()) {
        value = m_settings.value(Keys::LegacyUiLanguage).toString().trimmed();
    }
    if (value.isEmpty()) {
        value = m_settings.value(QStringLiteral("uiLanguage")).toString().trimmed();
    }
    if (value.isEmpty()) {
        return;
    }

    m_settings.setValue(Keys::UiLanguage, value);
    m_settings.remove(Keys::LegacyUiLanguage);
    m_settings.remove(QStringLiteral("uiLanguage"));
    m_settings.sync();
}

/**
 * @brief Ensures default values exist for critical settings.
 * @note Called when database exists to initialize missing settings.
 */
void AppSettings::ensureDefaults() {
    if (!m_settings.contains(Keys::StorageStrategy)) {
        m_settings.setValue(Keys::StorageStrategy, QStringLiteral("link"));
    }

    writeMissingDefaultExtensionCategories();
    m_settings.sync();
}

/**
 * @brief Adds missing extensions to existing categories in settings.
 * @note Preserves user-configured extensions while ensuring defaults are present.
 */
void AppSettings::writeMissingDefaultExtensionCategories() {
    for (const ImportExtensionCategory &category : defaultExtensionCategories()) {
        const QString key = QString::fromLatin1(Keys::ExtensionCategoryPrefix) + category.key;
        if (!m_settings.contains(key)) {
            m_settings.setValue(key, joinExtensions(category.extensions));
            continue;
        }

        QStringList existing = splitExtensions(m_settings.value(key).toString());
        bool changed = false;
        for (const QString &extension : category.extensions) {
            if (!existing.contains(extension)) {
                existing.append(extension);
                changed = true;
            }
        }
        if (changed) {
            m_settings.setValue(key, joinExtensions(existing));
        }
    }
}

/**
 * @brief Reloads extension categories from settings and notifies listeners.
 * @note Emits settingsChanged() signal after reload.
 */
void AppSettings::reload() {
    loadExtensionCategories();
    emit settingsChanged();
}

/**
 * @brief Gets the effective managed storage root directory.
 * @return Configured path if set, otherwise default path.
 */
QString AppSettings::managedStorageRoot() const {
    const QString configured = configuredManagedStorageRoot();
    return configured.isEmpty() ? defaultManagedStorageRoot() : configured;
}

/**
 * @brief Gets the user-configured managed storage root.
 * @return Configured path string (may be empty if not set).
 */
QString AppSettings::configuredManagedStorageRoot() const {
    return m_settings.value(Keys::ManagedStorageRoot).toString().trimmed();
}

/**
 * @brief Checks if managed storage root is explicitly configured.
 * @return true if non-empty configured path exists, false otherwise.
 */
bool AppSettings::hasConfiguredManagedStorageRoot() const {
    return !configuredManagedStorageRoot().isEmpty();
}

/**
 * @brief Gets the default managed storage path (alias for defaultManagedStorageRoot).
 * @return Default managed storage directory path.
 */
QString AppSettings::defaultManagedStoragePath() const { return defaultManagedStorageRoot(); }

/**
 * @brief Attempts to create a storage directory if it doesn't exist.
 * @param[in] path Directory path to create.
 * @return true if directory exists or was successfully created, false otherwise.
 */
bool AppSettings::createStorageDirectory(const QString &path) {
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    return QDir().mkpath(trimmed);
}

/**
 * @brief Sets the managed storage root directory.
 * @param[in] path Directory path to use for managed storage.
 * @note Trims input and ignores empty strings.
 */
void AppSettings::setManagedStorageRoot(const QString &path) {
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    m_settings.setValue(Keys::ManagedStorageRoot, trimmed);
}

/**
 * @brief Gets the current storage strategy setting.
 * @return Normalized strategy string: "copy", "move", or "link".
 */
QString AppSettings::storageStrategy() const {
    return normalizeStrategy(
        m_settings.value(Keys::StorageStrategy, QStringLiteral("link")).toString());
}

/**
 * @brief Sets the storage strategy for file operations.
 * @param[in] strategy Strategy string (case-insensitive: "copy", "move", or other).
 * @note Value is normalized before storage.
 */
void AppSettings::setStorageStrategy(const QString &strategy) {
    m_settings.setValue(Keys::StorageStrategy, normalizeStrategy(strategy));
}

/**
 * @brief Gets the storage strategy as a typed enumeration.
 * @return StorageStrategy enum value corresponding to current setting.
 */
StorageStrategy AppSettings::storageStrategyEnum() const {
    const QString strategy = storageStrategy();
    if (strategy == QStringLiteral("copy")) {
        return StorageStrategy::Copy;
    }
    if (strategy == QStringLiteral("move")) {
        return StorageStrategy::Move;
    }
    return StorageStrategy::Link;
}

/**
 * @brief Gets the list of configured import extension categories.
 * @return Reference to internal list of extension categories.
 */
const QList<ImportExtensionCategory> &AppSettings::extensionCategories() const {
    return m_categories;
}

/**
 * @brief Reloads extension categories from settings into the internal cache.
 * @return void
 * @note Clears the existing m_categories list and rebuilds it from the default templates and the current settings values.
 */
void AppSettings::loadExtensionCategories() {
    m_categories.clear();

    for (const ImportExtensionCategory &templateCategory : defaultExtensionCategories()) {
        const QString key =
            QString::fromLatin1(Keys::ExtensionCategoryPrefix) + templateCategory.key;
        ImportExtensionCategory category = templateCategory;
        category.extensions = splitExtensions(m_settings.value(key).toString());
        if (!category.extensions.isEmpty()) {
            m_categories.append(category);
        }
    }
}

/**
 * @brief Provides a sorted list of all allowed file extensions from the configured categories.
 * @return Sorted list of unique allowed extensions.
 * @note Duplicate extensions are removed and the list is alphabetized.
 */
QStringList AppSettings::allowedExtensions() const {
    QStringList extensions;
    for (const ImportExtensionCategory &category : m_categories) {
        for (const QString &extension : category.extensions) {
            if (!extensions.contains(extension)) {
                extensions.append(extension);
            }
        }
    }
    extensions.sort();
    return extensions;
}

/**
 * @brief Generates file dialog name filters based on the configured extension categories.
 * @return List of filters for use in file dialogs, including a combined "All supported" filter, per‑category filters, and a catch‑all "*".
 * @note The first filter is "All supported ( *.ext … )", followed by category‑specific filters, and ends with "All files (*)".
 */
QStringList AppSettings::fileDialogNameFilters() const {
    QStringList filters;

    const QStringList allExtensions = allowedExtensions();
    if (!allExtensions.isEmpty()) {
        QStringList allPatterns;
        allPatterns.reserve(allExtensions.size());
        for (const QString &extension : allExtensions) {
            allPatterns.append(QStringLiteral("*.%1").arg(extension));
        }
        filters.append(QStringLiteral("%1 (%2)").arg(tr("All supported"),
                                                     allPatterns.join(QLatin1Char(' '))));
    }

    for (const ImportExtensionCategory &category : m_categories) {
        if (category.extensions.isEmpty()) {
            continue;
        }

        QStringList patterns;
        patterns.reserve(category.extensions.size());
        for (const QString &extension : category.extensions) {
            patterns.append(QStringLiteral("*.%1").arg(extension));
        }

        filters.append(
            QStringLiteral("%1 (%2)").arg(category.label, patterns.join(QLatin1Char(' '))));
    }

    filters.append(tr("All files (*)"));

    return filters;
}

/**
 * @brief Determines the media kind based on a file extension.
 * Searches through the loaded extension categories to map the provided extension to its corresponding MediaKind.
 * @param[in] extension The file extension to check.
 * @return The matching MediaKind, or MediaKind::Unknown if no match is found.
 * @note The extension is normalized to lowercase before comparison.
 */
MediaKind AppSettings::mediaKindForExtension(const QString &extension) const {
    const QString normalized = extension.trimmed().toLower();
    for (const ImportExtensionCategory &category : m_categories) {
        if (category.extensions.contains(normalized)) {
            return category.mediaKind;
        }
    }
    return MediaKind::Unknown;
}

/**
 * @brief Checks if a file extension is allowed for import.
 * @param[in] extension The file extension to check.
 * @return True if the extension is allowed, false otherwise.
 * @note Delegates to mediaKindForExtension() for the check.
 */
bool AppSettings::isExtensionAllowed(const QString &extension) const {
    return mediaKindForExtension(extension) != MediaKind::Unknown;
}

/**
 * @brief Gets the currently configured UI language.
 * @return The trimmed UI language locale code, or empty string if not set.
 * @note Returns the value directly from settings without default fallback.
 */
QString AppSettings::uiLanguage() const {
    return m_settings.value(Keys::UiLanguage).toString().trimmed();
}

/**
 * @brief Sets the UI language locale.
 * @param[in] localeCode The locale code to set.
 * @note Triggers settingsChanged() signal and syncs to disk.
 */
void AppSettings::setUiLanguage(const QString &localeCode) {
    m_settings.setValue(Keys::UiLanguage, localeCode.trimmed());
    m_settings.sync();
    emit settingsChanged();
}

/**
 * @brief Gets the file path of the settings file as a QUrl.
 * @return QUrl pointing to the settings file location.
 */
QUrl AppSettings::settingsFileLocation() const {
    return QUrl::fromLocalFile(m_settings.fileName());
}

/**
 * @brief Sets the extensions for a specific category.
 * @param[in] categoryKey The category key to update.
 * @param[in] extensions List of extensions to associate with the category.
 * @note Overwrites any existing extensions for the category.
 */
void AppSettings::setExtensionCategory(const QString &categoryKey, const QStringList &extensions) {
    const QString key = QString::fromLatin1(Keys::ExtensionCategoryPrefix) + categoryKey.trimmed();
    m_settings.setValue(key, joinExtensions(extensions));
}

/**
 * @brief Resets all extension categories to their default values.
 * Overwrites any user-modified extension categories with defaults.
 */
void AppSettings::resetExtensionCategoriesToDefaults() {
    for (const ImportExtensionCategory &category : defaultExtensionCategories()) {
        const QString key = QString::fromLatin1(Keys::ExtensionCategoryPrefix) + category.key;
        m_settings.setValue(key, joinExtensions(category.extensions));
    }
}

/**
 * @brief Saves the current configuration to disk.
 * Syncs settings to storage and reloads the internal state.
 */
void AppSettings::saveConfiguration() {
    m_settings.sync();
    reload();
}

/**
 * @brief Gets default extension categories formatted for QML consumption.
 * @return QVariantList of maps with key, label, and extensions for each default category.
*/
QVariantList AppSettings::defaultExtensionCategoriesForUi() const {
    QVariantList list;
    for (const ImportExtensionCategory &category : defaultExtensionCategories()) {
        QVariantMap entry;
        entry.insert(QStringLiteral("key"), category.key);
        entry.insert(QStringLiteral("label"), category.label);
        entry.insert(QStringLiteral("extensions"), joinExtensions(category.extensions));
        list.append(entry);
    }
    return list;
}

/**
 * @brief Gets current extension categories formatted for QML consumption.
 * @return QVariantList of maps with key, label, and extensions for each loaded category.
 */
QVariantList AppSettings::extensionCategoriesForUi() const {
    QVariantList list;
    for (const ImportExtensionCategory &category : m_categories) {
        QVariantMap entry;
        entry.insert(QStringLiteral("key"), category.key);
        entry.insert(QStringLiteral("label"), category.label);
        entry.insert(QStringLiteral("extensions"), joinExtensions(category.extensions));
        list.append(entry);
    }
    return list;
}

/**
 * @brief Applies extension categories from QML/UI input.
 * @param[in] categories QVariantList of category maps from UI.
 * @note Each map must contain "key" and "extensions" fields. Empty keys are skipped.
*/
void AppSettings::applyExtensionCategoriesFromUi(const QVariantList &categories) {
    for (const QVariant &value : categories) {
        const QVariantMap entry = value.toMap();
        const QString key = entry.value(QStringLiteral("key")).toString();
        const QString extensions = entry.value(QStringLiteral("extensions")).toString();
        if (key.isEmpty()) {
            continue;
        }
        m_settings.setValue(QString::fromLatin1(Keys::ExtensionCategoryPrefix) + key,
                            extensions.trimmed());
    }
}
