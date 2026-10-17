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
 * The bank is preloaded into memory at application start. The player page (qrc)
 * fetches it via sp-soundfont:// — avoiding huge base64 transfers through
 * runJavaScript. Changing the SoundFont path requires an app restart so the
 * cache can be rebuilt before WebEngine starts serving requests.
 */
class SoundFontSchemeHandler : public QWebEngineUrlSchemeHandler {
  public:
    static void registerScheme();
    [[nodiscard]] static QString loadUrl();

    explicit SoundFontSchemeHandler(QObject *parent = nullptr);

    /** Load @p path into the in-memory cache (call at startup). */
    void setFilePath(const QString &path);
    void clear();
    [[nodiscard]] QString filePath() const;
    [[nodiscard]] bool hasCachedData() const;
    [[nodiscard]] qsizetype cachedByteCount() const;
    /** True when cache matches the currently configured custom SoundFont path. */
    [[nodiscard]] bool matchesPath(const QString &path) const;

    void requestStarted(QWebEngineUrlRequestJob *request) override;

  private:
    QString m_filePath;
    QByteArray m_cachedData;
};

#endif // SONARPRACTICE_HAS_WEBENGINE

#endif // SOUNDFONTSCHEMEHANDLER_H
