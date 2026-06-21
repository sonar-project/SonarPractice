#include "PathResolver.h"

#include <QDir>

PathResolver::PathResolver(QString managedStorageRoot)
    : m_managedStorageRoot(std::move(managedStorageRoot)) {}

QString PathResolver::resolve(const MediaFile &mediaFile) const {
    if (mediaFile.sourceType == MediaSourceType::Url) {
        return mediaFile.filePath;
    }

    if (mediaFile.isManaged) {
        return QDir(m_managedStorageRoot).filePath(mediaFile.filePath);
    }

    return mediaFile.filePath;
}
