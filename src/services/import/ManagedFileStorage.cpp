#include "ManagedFileStorage.h"

#include "fnv1a.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace {

    constexpr int kMaxDuplicateFileNameAttempts = 10000;

    struct ResolvedDestination {
        QString fileName;
        bool duplicateContent{false};
    };

    QString hashOfFileAt(const QDir &destinationDir, const QString &fileName) {
        return FNV1a::calculate(destinationDir.filePath(fileName));
    }

    ResolvedDestination resolveDestinationFileName(const QDir &destinationDir, const QString &fileName,
                                                 const QString &fileHash) {
        if (!destinationDir.exists(fileName)) {
            return {.fileName = fileName, .duplicateContent = false};
        }

        if (hashOfFileAt(destinationDir, fileName) == fileHash) {
            return {.fileName = fileName, .duplicateContent = true};
        }

        const QFileInfo info(fileName);
        const QString baseName = info.completeBaseName();
        const QString suffix = info.completeSuffix();

        for (int counter = 1; counter < kMaxDuplicateFileNameAttempts; ++counter) {
            const QString candidate =
                suffix.isEmpty()
                    ? QStringLiteral("%1_ver%2").arg(baseName).arg(counter)
                    : QStringLiteral("%1_ver%2.%3").arg(baseName).arg(counter).arg(suffix);

            if (!destinationDir.exists(candidate)) {
                return {.fileName = candidate, .duplicateContent = false};
            }

            if (hashOfFileAt(destinationDir, candidate) == fileHash) {
                return {.fileName = candidate, .duplicateContent = true};
            }
        }

        return {.fileName = fileName, .duplicateContent = false};
    }

} // namespace

ManagedFileResult ManagedFileStorage::storeFile(StorageStrategy strategy,
                                                const StorageParameters &params) {
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

    if (params.fileHash.trimmed().isEmpty()) {
        result.message = tr("File hash is empty");
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

    const ResolvedDestination resolved =
        resolveDestinationFileName(destinationDir, sourceInfo.fileName(), params.fileHash);
    const QString relativePath = relativePrefix + resolved.fileName;
    const QString destinationPath = rootDir.filePath(relativePath);

    if (resolved.duplicateContent) {
        result.success = true;
        result.storedPath = destinationPath;
        result.relativePath = relativePath;
        result.isManaged = true;
        result.duplicateContent = true;
        return result;
    }

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
