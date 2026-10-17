#ifndef TRANSLATIONMANAGER_H
#define TRANSLATIONMANAGER_H

#include <QObject>
#include <QVariantList>

class AppSettings;
class QQmlApplicationEngine;
class QTranslator;

class TranslationManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString storedLanguage READ storedUiLanguage WRITE setUiLanguage NOTIFY storedLanguageChanged)
    Q_PROPERTY(QString uiLanguage READ effectiveUiLanguage NOTIFY uiLanguageChanged)

  public:
    explicit TranslationManager(AppSettings &settings, QObject *parent = nullptr);
    ~TranslationManager() override;

    void setEngine(QQmlApplicationEngine *engine);

    [[nodiscard]] QString storedUiLanguage() const;
    [[nodiscard]] QString effectiveUiLanguage() const;
    void setUiLanguage(const QString &localeCode);

    Q_INVOKABLE void applySavedLanguage();
    Q_INVOKABLE QVariantList availableUiLanguages() const;

  signals:
    void storedLanguageChanged();
    void uiLanguageChanged();

  private:
    [[nodiscard]] static QStringList catalogLocales();
    [[nodiscard]] static QString sourceLanguageLabel();
    [[nodiscard]] static QString localeLabel(const QString &localeCode);
    [[nodiscard]] static QString normalizeLanguageCode(const QString &localeCode);
    [[nodiscard]] QString resolveSystemLocale() const;
    bool applyLanguage(const QString &localeCode);
    void removeTranslator();

    AppSettings &m_settings;
    QQmlApplicationEngine *m_engine = nullptr;
    QTranslator *m_translator = nullptr;
};

#endif // TRANSLATIONMANAGER_H
