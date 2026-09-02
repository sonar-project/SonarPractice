#ifdef SONARPRACTICE_HAS_WEBENGINE

#include "SoundFontSchemeHandler.h"

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlScheme>

namespace {

    constexpr auto kSchemeName = "sp-soundfont";
    constexpr auto kLoadUrl = "sp-soundfont://local/current.sf2";

} // namespace

void SoundFontSchemeHandler::registerScheme() {
    QWebEngineUrlScheme scheme(kSchemeName);
    scheme.setFlags(QWebEngineUrlScheme::SecureScheme | QWebEngineUrlScheme::LocalScheme
                    | QWebEngineUrlScheme::LocalAccessAllowed
                    | QWebEngineUrlScheme::CorsEnabled);
    QWebEngineUrlScheme::registerScheme(scheme);
}

QString SoundFontSchemeHandler::loadUrl() { return QString::fromLatin1(kLoadUrl); }

SoundFontSchemeHandler::SoundFontSchemeHandler(QObject *parent)
    : QWebEngineUrlSchemeHandler(parent) {}

void SoundFontSchemeHandler::setFilePath(const QString &path) {
    m_filePath = path;
    m_cachedData.clear();

    const QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isFile() || fileInfo.size() <= 0) {
        return;
    }

    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        m_cachedData = file.readAll();
    }
}

void SoundFontSchemeHandler::clear() {
    m_filePath.clear();
    m_cachedData.clear();
}

bool SoundFontSchemeHandler::hasCachedData() const { return !m_cachedData.isEmpty(); }

void SoundFontSchemeHandler::requestStarted(QWebEngineUrlRequestJob *request) {
    if (!request) {
        return;
    }

    if (m_cachedData.isEmpty()) {
        request->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }

    auto *buffer = new QBuffer();
    buffer->setData(m_cachedData);
    buffer->open(QIODevice::ReadOnly);
    request->reply(QByteArrayLiteral("application/octet-stream"), buffer);
}

#endif // SONARPRACTICE_HAS_WEBENGINE
