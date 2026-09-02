#ifndef SOUNDFONTSCHEMEHANDLER_H
#define SOUNDFONTSCHEMEHANDLER_H

#include <QString>

class QObject;

#ifdef SONARPRACTICE_HAS_WEBENGINE

#include <QByteArray>
#include <QWebEngineUrlSchemeHandler>

class QWebEngineUrlRequestJob;

/**
 * @brief Serves the configured custom SoundFont to the AlphaTab WebEngine page.
 *
 * Qt WebEngine blocks reliable XHR access to file:// URLs from qrc content.
 * AlphaTab loads custom banks via fetch/XHR, so we expose the file on sp-soundfont://.
 */
class SoundFontSchemeHandler : public QWebEngineUrlSchemeHandler {
  public:
    static void registerScheme();
    [[nodiscard]] static QString loadUrl();

    explicit SoundFontSchemeHandler(QObject *parent = nullptr);

    void setFilePath(const QString &path);
    void clear();
    [[nodiscard]] bool hasCachedData() const;

    void requestStarted(QWebEngineUrlRequestJob *request) override;

  private:
    QString m_filePath;
    QByteArray m_cachedData;
};

#endif // SONARPRACTICE_HAS_WEBENGINE

#endif // SOUNDFONTSCHEMEHANDLER_H
