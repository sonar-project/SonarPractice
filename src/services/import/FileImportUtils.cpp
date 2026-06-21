#include "FileImportUtils.h"

#include <QDirIterator>
#include <QFileInfo>
#include <algorithm>

namespace FileImportUtils {

    QString normalizedExtension(const QString &filePath) {
        return QFileInfo(filePath).suffix().toLower();
    }

    bool isExtensionAllowed(const QString &extension, const QStringList &allowedExtensions) {
        const QString normalized = extension.toLower();
        return std::ranges::any_of(allowedExtensions, [&](const QString &allowed) {
            return normalized == allowed.toLower();
        });
    }

    bool canBePracticed(const MediaKind &kind) { return kind == MediaKind::GuitarPro; }

    QStringList collectSupportedFiles(const QString &directoryPath,
                                      const QStringList &allowedExtensions) {
        QStringList files;
        const QList<CollectedFile> collected =
            collectSupportedFilesWithPaths(directoryPath, allowedExtensions);
        files.reserve(collected.size());
        for (const CollectedFile &entry : collected) {
            files.append(entry.absolutePath);
        }
        return files;
    }

    QList<CollectedFile> collectSupportedFilesWithPaths(const QString &directoryPath,
                                                        const QStringList &allowedExtensions) {
        QList<CollectedFile> files;
        const QString normalizedRoot = QDir(directoryPath).absolutePath();
        const QString rootFolderName = QFileInfo(normalizedRoot).fileName();
        QDirIterator iterator(normalizedRoot, QDir::Files,
                              QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);

        while (iterator.hasNext()) {
            const QString filePath = iterator.next();
            if (!isExtensionAllowed(normalizedExtension(filePath), allowedExtensions)) {
                continue;
            }

            const QFileInfo fileInfo(filePath);
            const QString pathInsideRoot =
                QDir(normalizedRoot).relativeFilePath(fileInfo.absoluteFilePath());
            CollectedFile entry;
            entry.absolutePath = fileInfo.absoluteFilePath();
            entry.importRoot = normalizedRoot;
            entry.sourceRelativePath = rootFolderName.isEmpty()
                                           ? pathInsideRoot
                                           : rootFolderName + QLatin1Char('/') + pathInsideRoot;
            files.append(entry);
        }

        std::ranges::sort(files, [](const CollectedFile &a, const CollectedFile &b) {
            return a.absolutePath.localeAwareCompare(b.absolutePath) < 0;
        });
        return files;
    }

    QList<CollectedFile> collectEntriesFromPaths(const PathParameters &params) {
        QList<CollectedFile> entries;

        for (const QString &path : params.filePaths) {
            const QFileInfo fileInfo(path);
            if (!fileInfo.exists()) {
                continue;
            }

            if (fileInfo.isDir()) {
                entries.append(collectSupportedFilesWithPaths(fileInfo.absoluteFilePath(),
                                                              params.allowedExtensions));
                continue;
            }

            if (!fileInfo.isFile()) {
                continue;
            }

            if (!isExtensionAllowed(normalizedExtension(fileInfo.absoluteFilePath()),
                                    params.allowedExtensions)) {
                continue;
            }

            CollectedFile entry;
            entry.absolutePath = fileInfo.absoluteFilePath();
            entry.importRoot.clear();
            entry.sourceRelativePath = fileInfo.fileName();
            entries.append(entry);
        }

        return entries;
    }

} // namespace FileImportUtils
