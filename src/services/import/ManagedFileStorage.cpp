#include "ManagedFileStorage.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace {

    constexpr int kMaxDuplicateFileNameAttempts = 10000;

    QString uniqueFileName(const QDir &rootDir, const QString &fileName) {
        if (!rootDir.exists(fileName)) {
            return fileName;
        }

        const QFileInfo info(fileName);
        const QString baseName = info.completeBaseName();
        const QString suffix = info.completeSuffix();

        for (int counter = 1; counter < kMaxDuplicateFileNameAttempts; ++counter) {
            const QString candidate =
                suffix.isEmpty()
                    ? QStringLiteral("%1_ver%2").arg(baseName).arg(counter)
                    : QStringLiteral("%1_ver%2.%3").arg(baseName).arg(counter).arg(suffix);

            if (!rootDir.exists(candidate)) {
                return candidate;
            }
        }

        return fileName;
    }

} // namespace

ManagedFileResult ManagedFileStorage::storeFile(StorageStrategy strategy,
                                                const StorageParameters &params) {
    Q_UNUSED(params.fileHash)
    Q_UNUSED(params.extension)

    ManagedFileResult result;
    const QFileInfo sourceInfo(params.sourcePath);

    if (!sourceInfo.exists() || !sourceInfo.isFile()) {
        result.message = tr("File does not exist");
        return result;
    }

    if (strategy == StorageStrategy::Link) {
        result.success = true;
        result.storedPath = sourceInfo.absoluteFilePath();
        result.relativePath = result.storedPath;
        result.isManaged = false;
        return result;
    }

    if (params.managedRoot.trimmed().isEmpty()) {
        result.message = tr("Managed storage root is empty");
        return result;
    }

    const QDir rootDir(params.managedRoot);

    if (!rootDir.exists() && !QDir().mkpath(params.managedRoot)) {
        result.message = tr("Could not create managed storage root");
        return result;
    }

    QDir destinationDir = rootDir;
    QString relativePrefix;
    if (!params.relativeSubPath.trimmed().isEmpty()) {
        const QString normalizedSubPath =
            QDir::fromNativeSeparators(params.relativeSubPath.trimmed());
        relativePrefix = normalizedSubPath.endsWith(QLatin1Char('/'))
                             ? normalizedSubPath
                             : normalizedSubPath + QLatin1Char('/');
        const QString destinationRoot = rootDir.filePath(normalizedSubPath);
        if (!QDir().mkpath(destinationRoot)) {
            result.message = tr("Could not create managed storage subfolder");
            return result;
        }
        destinationDir = QDir(destinationRoot);
    }

    const QString relativePath =
        relativePrefix + uniqueFileName(destinationDir, sourceInfo.fileName());
    const QString destinationPath = rootDir.filePath(relativePath);

    if (strategy == StorageStrategy::Copy) {
        if (!QFile::copy(params.sourcePath, destinationPath)) {
            result.message = tr("Could not copy file");
            return result;
        }
    } else if (strategy == StorageStrategy::Move) {
        if (!QFile::rename(params.sourcePath, destinationPath)) {
            result.message = tr("Could not move file");
            return result;
        }
    } else {
        result.message = tr("Unsupported storage strategy");
        return result;
    }

    result.success = true;
    result.storedPath = destinationPath;
    result.relativePath = relativePath;
    result.isManaged = true;
    return result;
}
