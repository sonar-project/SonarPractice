/**
 * @file MediaStreamProbe.cpp
 * @brief FFmpeg-based stream detection and extension-based fallbacks for imports.
 */

#include "MediaStreamProbe.h"

#include <QFileInfo>
#include <QSet>

#if defined(SONARPRACTICE_HAS_FFMPEG)
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}
#endif

namespace {

    QSet<QString> containerExtensions() {
        return {
            QStringLiteral("mp4"), QStringLiteral("m4v"),  QStringLiteral("mov"),
            QStringLiteral("mkv"), QStringLiteral("webm"), QStringLiteral("avi"),
            QStringLiteral("wmv"), QStringLiteral("mpg"),  QStringLiteral("mpeg"),
            QStringLiteral("ogv"), QStringLiteral("3gp"),  QStringLiteral("flv"),
        };
    }

    MediaStreamProbeResult fallbackForExtension(MediaKind extensionKind) {
        MediaStreamProbeResult result;
        result.probed = false;

        switch (extensionKind) {
        case MediaKind::Video:
            result.hasVideo = true;
            break;
        case MediaKind::Audio:
            result.hasAudio = true;
            break;
        default:
            break;
        }

        return result;
    }

} // namespace

MediaKind classifyFromProbe(const MediaStreamProbeResult &result, MediaKind extensionDefault) {
    if (result.hasVideo) {
        return MediaKind::Video;
    }
    if (result.hasAudio) {
        return MediaKind::Audio;
    }
    return extensionDefault;
}

bool MediaStreamProbe::isContainerExtension(const QString &extension) {
    return containerExtensions().contains(extension.trimmed().toLower());
}

MediaStreamProbeResult MediaStreamProbe::probeFile(const QString &absolutePath) {
    MediaStreamProbeResult result;

    const QFileInfo fileInfo(absolutePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        result.error = tr("File does not exist");
        return result;
    }

#if defined(SONARPRACTICE_HAS_FFMPEG)
    AVFormatContext *formatContext = avformat_alloc_context();
    if (formatContext == nullptr) {
        result.error = tr("Could not allocate format context");
        return result;
    }

    const QByteArray pathUtf8 = fileInfo.absoluteFilePath().toUtf8();
    const int openStatus =
        avformat_open_input(&formatContext, pathUtf8.constData(), nullptr, nullptr);
    if (openStatus < 0) {
        avformat_free_context(formatContext);
        result.error = tr("Could not open media container");
        return result;
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        avformat_close_input(&formatContext);
        result.error = tr("Could not read stream info");
        return result;
    }

    for (unsigned int streamIndex = 0; streamIndex < formatContext->nb_streams; ++streamIndex) {
        const AVCodecParameters *codecParams = formatContext->streams[streamIndex]->codecpar;
        if (codecParams == nullptr) {
            continue;
        }

        if (codecParams->codec_type == AVMEDIA_TYPE_VIDEO) {
            result.hasVideo = true;
        } else if (codecParams->codec_type == AVMEDIA_TYPE_AUDIO) {
            result.hasAudio = true;
        }
    }

    avformat_close_input(&formatContext);
    result.probed = true;
#else
    Q_UNUSED(absolutePath);
    result.error = tr("FFmpeg probe unavailable");
#endif

    return result;
}

MediaStreamProbeResult MediaStreamProbe::inferFromExtension(MediaKind extensionKind) {
    return fallbackForExtension(extensionKind);
}

MediaStreamProbeResult MediaStreamProbe::probeOrInfer(const QString &absolutePath,
                                                      MediaKind extensionKind,
                                                      const QString &extension) {
    if (isContainerExtension(extension)) {
        MediaStreamProbeResult probed = probeFile(absolutePath);
        if (probed.probed) {
            return probed;
        }

        MediaStreamProbeResult fallback = fallbackForExtension(extensionKind);
        fallback.error = probed.error;
        return fallback;
    }

    if (extensionKind == MediaKind::Audio) {
        MediaStreamProbeResult result;
        result.hasAudio = true;
        result.probed = false;
        return result;
    }

    return inferFromExtension(extensionKind);
}
