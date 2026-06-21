#include "DesktopLauncher.h"

#include <QDesktopServices>
#include <QUrl>

bool DesktopLauncher::open(const QString &target, MediaSourceType sourceType) {
    if (target.trimmed().isEmpty()) {
        return false;
    }

    if (sourceType == MediaSourceType::Url) {
        return QDesktopServices::openUrl(QUrl(target));
    }

    return QDesktopServices::openUrl(QUrl::fromLocalFile(target));
}
