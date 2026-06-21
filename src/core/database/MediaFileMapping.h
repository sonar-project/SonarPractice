#ifndef MEDIAFILEMAPPING_H
#define MEDIAFILEMAPPING_H

#include "MediaFile.h"

class QString;

namespace MediaFileMapping {

    inline QString sourceTypeToString(MediaSourceType type) {
        switch (type) {
        case MediaSourceType::Local:
            return QStringLiteral("LOCAL");
        case MediaSourceType::Url:
            return QStringLiteral("URL");
        }
        return QStringLiteral("LOCAL");
    }

    inline MediaSourceType sourceTypeFromString(const QString &value) {
        if (value.compare(QStringLiteral("URL"), Qt::CaseInsensitive) == 0) {
            return MediaSourceType::Url;
        }
        return MediaSourceType::Local;
    }

    inline QString mediaKindToString(MediaKind kind) {
        switch (kind) {
        case MediaKind::GuitarPro:
            return QStringLiteral("guitarpro");
        case MediaKind::Audio:
            return QStringLiteral("audio");
        case MediaKind::Video:
            return QStringLiteral("video");
        case MediaKind::Image:
            return QStringLiteral("image");
        case MediaKind::Document:
            return QStringLiteral("document");
        case MediaKind::Unknown:
            return QStringLiteral("unknown");
        }
        return QStringLiteral("unknown");
    }

    inline MediaKind mediaKindFromString(const QString &value) {
        const QString normalized = value.toLower();
        if (normalized == QLatin1String("guitarpro")) {
            return MediaKind::GuitarPro;
        }
        if (normalized == QLatin1String("audio")) {
            return MediaKind::Audio;
        }
        if (normalized == QLatin1String("video")) {
            return MediaKind::Video;
        }
        if (normalized == QLatin1String("image")) {
            return MediaKind::Image;
        }
        if (normalized == QLatin1String("document")) {
            return MediaKind::Document;
        }
        return MediaKind::Unknown;
    }

} // namespace MediaFileMapping

#endif // MEDIAFILEMAPPING_H
