#include "FileImportUtils.h"

#include <QDirIterator>
#include <QFileInfo>
#include <algorithm>

namespace FileImportUtils {

    namespace {

        bool isCancelRequested(const std::atomic_bool *cancelRequested) {
            return cancelRequested != nullptr &&
                   cancelRequested->load(std::memory_order_acquire);
        }

        void notifyFileFound(const FileFoundCallback &onFileFound, int foundCount,
                             const QString &absolutePath) {
            if (onFileFound) {
                onFileFound(foundCount, absolutePath);
            }
        }

    } // namespace

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

    QList<CollectedFile>
    collectSupportedFilesWithPaths(const QString &directoryPath,
                                   const QStringList &allowedExtensions,
                                   const std::atomic_bool *cancelRequested,
                                   const FileFoundCallback &onFileFound) {
        QList<CollectedFile> files;
        const QString normalizedRoot = QDir(directoryPath).absolutePath();
        const QString rootFolderName = QFileInfo(normalizedRoot).fileName();
        QDirIterator iterator(normalizedRoot, QDir::Files,
                              QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);

        while (iterator.hasNext()) {
            if (isCancelRequested(cancelRequested)) {
                return files;
            }

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
            notifyFileFound(onFileFound, static_cast<int>(files.size()), entry.absolutePath);
        }

        if (isCancelRequested(cancelRequested)) {
            return files;
        }

        std::ranges::sort(files, [](const CollectedFile &a, const CollectedFile &b) {
            return a.absolutePath.localeAwareCompare(b.absolutePath) < 0;
        });
        return files;
    }

    QList<CollectedFile> collectEntriesFromPaths(const PathParameters &params,
                                                 const std::atomic_bool *cancelRequested,
                                                 const FileFoundCallback &onFileFound) {
        QList<CollectedFile> entries;
        int foundCount = 0;

        for (const QString &path : params.filePaths) {
            if (isCancelRequested(cancelRequested)) {
                break;
            }

            const QFileInfo fileInfo(path);
            if (!fileInfo.exists()) {
                continue;
            }

            if (fileInfo.isDir()) {
                const int baseCount = foundCount;
                const QList<CollectedFile> directoryEntries = collectSupportedFilesWithPaths(
                    fileInfo.absoluteFilePath(), params.allowedExtensions, cancelRequested,
                    [&](int localCount, const QString &absolutePath) {
                        foundCount = baseCount + localCount;
                        notifyFileFound(onFileFound, foundCount, absolutePath);
                    });
                entries.append(directoryEntries);
                foundCount = baseCount + directoryEntries.size();
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
            ++foundCount;
            notifyFileFound(onFileFound, foundCount, entry.absolutePath);
        }

        return entries;
    }

} // namespace FileImportUtils
