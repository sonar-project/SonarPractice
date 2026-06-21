#include "TranslationManager.h"

#include "AppSettings.h"
#include "TranslationCatalog.generated.h"

#include <QCoreApplication>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QTranslator>
#include <QVariantMap>

namespace {

    constexpr auto kTranslationResourcePath = ":/i18n";

    QString migrateLegacyLanguageCode(const QString &languageCode) {
        const QString trimmed = languageCode.trimmed();
        if (trimmed.compare(SonarpTranslationCatalog::sourceLanguage(), Qt::CaseInsensitive) == 0) {
            return SonarpTranslationCatalog::sourceLanguage();
        }

        const QString normalized = trimmed.toLower();
        if (normalized == QStringLiteral("de")) {
            return QStringLiteral("de_DE");
        }

        return trimmed;
    }

    bool isSourceLanguageCode(const QString &localeCode) {
        return localeCode.compare(SonarpTranslationCatalog::sourceLanguage(), Qt::CaseInsensitive) == 0;
    }

} // namespace

TranslationManager::TranslationManager(AppSettings &settings, QObject *parent)
    : QObject(parent), m_settings(settings) {}

TranslationManager::~TranslationManager() { removeTranslator(); }

void TranslationManager::setEngine(QQmlApplicationEngine *engine) { m_engine = engine; }

QString TranslationManager::storedUiLanguage() const { return m_settings.uiLanguage(); }

QString TranslationManager::effectiveUiLanguage() const {
    const QString stored = storedUiLanguage();
    if (!stored.isEmpty()) {
        const QString normalized = normalizeLanguageCode(stored);
        if (!normalized.isEmpty()) {
            return normalized;
        }
    }
    return resolveSystemLocale();
}

void TranslationManager::setUiLanguage(const QString &localeCode) {
    const QString normalized = normalizeLanguageCode(localeCode);
    if (normalized.isEmpty()) {
        return;
    }

    const QString previousEffective = effectiveUiLanguage();
    const bool storedChanged = normalized != storedUiLanguage();

    if (storedChanged) {
        m_settings.setUiLanguage(normalized);
    }

    applyLanguage(normalized);

    if (storedChanged) {
        emit storedLanguageChanged();
    }
    if (previousEffective != effectiveUiLanguage()) {
        emit uiLanguageChanged();
    }
}

void TranslationManager::applySavedLanguage() {
    const QString migrated = migrateLegacyLanguageCode(storedUiLanguage());
    if (migrated != storedUiLanguage()) {
        m_settings.setUiLanguage(migrated);
        emit storedLanguageChanged();
    }
    applyLanguage(effectiveUiLanguage());
    emit uiLanguageChanged();
}

QVariantList TranslationManager::availableUiLanguages() const {
    QVariantList languages;

    const QString sourceLanguage = SonarpTranslationCatalog::sourceLanguage();
    {
        QVariantMap sourceEntry;
        sourceEntry.insert(QStringLiteral("code"), sourceLanguage);
        sourceEntry.insert(QStringLiteral("fileName"), QString());
        sourceEntry.insert(QStringLiteral("label"), sourceLanguageLabel());
        sourceEntry.insert(QStringLiteral("isSource"), true);
        languages.append(sourceEntry);
    }

    const QString baseName = SonarpTranslationCatalog::baseName();
    for (const QString &locale : catalogLocales()) {
        if (isSourceLanguageCode(locale)) {
            continue;
        }

        QVariantMap entry;
        entry.insert(QStringLiteral("code"), locale);
        entry.insert(QStringLiteral("fileName"),
                     QStringLiteral("%1_%2.ts").arg(baseName, locale));
        entry.insert(QStringLiteral("label"), localeLabel(locale));
        entry.insert(QStringLiteral("isSource"), false);
        languages.append(entry);
    }

    return languages;
}

QStringList TranslationManager::catalogLocales() { return SonarpTranslationCatalog::locales(); }

QString TranslationManager::sourceLanguageLabel() {
    return QStringLiteral("English (%1)").arg(SonarpTranslationCatalog::sourceLanguage());
}

QString TranslationManager::localeLabel(const QString &localeCode) {
    const QLocale locale(localeCode);
    const QString nativeName = locale.nativeLanguageName();
    if (nativeName.isEmpty()) {
        return localeCode;
    }
    return QStringLiteral("%1 (%2)").arg(nativeName, localeCode);
}

QString TranslationManager::normalizeLanguageCode(const QString &localeCode) {
    const QString trimmed = localeCode.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }
    if (isSourceLanguageCode(trimmed)) {
        return SonarpTranslationCatalog::sourceLanguage();
    }
    if (catalogLocales().contains(trimmed)) {
        return trimmed;
    }
    return QString();
}

QString TranslationManager::resolveSystemLocale() const {
    const QLocale system = QLocale::system();
    const QString systemName = system.name();
    if (catalogLocales().contains(systemName)) {
        return systemName;
    }

    for (const QString &localeCode : catalogLocales()) {
        if (QLocale(localeCode).language() == system.language()) {
            return localeCode;
        }
    }

    return SonarpTranslationCatalog::sourceLanguage();
}

bool TranslationManager::applyLanguage(const QString &localeCode) {
    removeTranslator();

    if (isSourceLanguageCode(localeCode)) {
        if (m_engine != nullptr) {
            m_engine->retranslate();
        }
        return true;
    }

    if (!catalogLocales().contains(localeCode)) {
        return false;
    }

    auto *translator = new QTranslator(this);
    const QString qmPath =
        QStringLiteral("%1/%2_%3.qm")
            .arg(QString::fromLatin1(kTranslationResourcePath),
                 SonarpTranslationCatalog::baseName(), localeCode);
    if (!translator->load(qmPath)) {
        delete translator;
        return false;
    }

    QCoreApplication::installTranslator(translator);
    m_translator = translator;

    if (m_engine != nullptr) {
        m_engine->retranslate();
    }
    return true;
}

void TranslationManager::removeTranslator() {
    if (m_translator == nullptr) {
        return;
    }

    QCoreApplication::removeTranslator(m_translator);
    delete m_translator;
    m_translator = nullptr;
}
