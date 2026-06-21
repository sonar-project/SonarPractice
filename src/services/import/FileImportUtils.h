#ifndef FILEIMPORTUTILS_H
#define FILEIMPORTUTILS_H

#include "MediaFile.h"

#include <QString>
#include <QStringList>

struct CollectedFile {
    QString absolutePath{};
    QString importRoot{};
    QString sourceRelativePath{};
};

struct PathParams {
    QStringList paths{};
    QStringList extensions{};
};

struct PathParameters {
    const QStringList &filePaths{};
    const QStringList &allowedExtensions{};

    explicit PathParameters(const PathParams &params)
        : filePaths(params.paths), allowedExtensions(params.extensions) {}
};

namespace FileImportUtils {

    [[nodiscard]] QString normalizedExtension(const QString &filePath);

    [[nodiscard]] bool isExtensionAllowed(const QString &extension,
                                          const QStringList &allowedExtensions);

    [[nodiscard]] bool canBePracticed(const MediaKind &kind);

    [[nodiscard]] QStringList collectSupportedFiles(const QString &directoryPath,
                                                    const QStringList &allowedExtensions);

    [[nodiscard]] QList<CollectedFile>
    collectSupportedFilesWithPaths(const QString &directoryPath,
                                   const QStringList &allowedExtensions);

    [[nodiscard]] QList<CollectedFile> collectEntriesFromPaths(const PathParameters &params);

} // namespace FileImportUtils

#endif // FILEIMPORTUTILS_H
