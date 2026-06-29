#include "ImportService.h"
#include "FileImportUtils.h"
#include "ImportTypes.h"
#include "ManagedFileStorage.h"
#include "MediaStreamProbe.h"
#include "fnv1a.h"

#include <QSet>
#include <utility>

#include "SqliteArtistRepository.h"
#include "SqliteConnection.h"
#include "SqliteMediaFileRepository.h"
#include "SqliteSongRepository.h"
#include "SqliteTuningRepository.h"

#include <QFileInfo>
#include <QMetaObject>
#include <QSqlDatabase>
#include <QThread>
#include <functional>

namespace {

    struct ImportEnvironment {
        QString storageRoot;
        StorageStrategy defaultStrategy{StorageStrategy::Link};
        QStringList allowedFileTypes;
        QList<ImportExtensionCategory> extensionCategories;
    };

    struct ImportExecutionContext {
        IArtistRepository &artistRepo;
        ITuningRepository &tuningRepo;
        ISongRepository &songRepo;
        IMediaFileRepository &mediaFileRepo;
        LibraryManager &libraryManager;
        ImportEnvironment environment;
    };

    struct ImportBatchJob {
        ImportEnvironment environment;
        QString databasePath;
        QList<ImportFileEntry> files;
        StorageStrategy strategy;
    };

    /**
     * @brief Construct import environment from dependencies.
     * @param[in] dependencies Service dependencies.
     * @return Environment with storage root, strategy, allowed types, and categories.
     */
    ImportEnvironment buildImportEnvironment(const ImportService::Dependencies &dependencies) {
        return {
            .storageRoot = dependencies.appSettings.managedStorageRoot(),
            .defaultStrategy = dependencies.appSettings.storageStrategyEnum(),
            .allowedFileTypes = dependencies.config.allowedFileTypes(),
            .extensionCategories = dependencies.appSettings.extensionCategories(),
        };
    }

    /**
     * @brief Determine media kind from file extension.
     * @param[in] environment Current import environment.
     * @param[in] extension File extension.
     * @return Corresponding MediaKind or Unknown.
     */
    MediaKind mediaKindForExtension(const ImportEnvironment &environment,
                                    const QString &extension) {
        const QString normalized = extension.trimmed().toLower();
        for (const ImportExtensionCategory &category : environment.extensionCategories) {
            if (category.extensions.contains(normalized)) {
                return category.mediaKind;
            }
        }
        return MediaKind::Unknown;
    }

    /**
     * @brief Resolve artist ID, creating artist if necessary.
     * @param[in,out] ctx Execution context.
     * @param[in] name Artist name.
     * @return Existing or newly created artist ID.
     */
    std::optional<qlonglong> resolveArtistId(ImportExecutionContext &ctx, const QString &name) {
        if (auto existing = ctx.artistRepo.findArtistByName(name)) {
            return existing->id;
        }

        Artist artist;
        artist.name = name;
        return ctx.artistRepo.createArtist(artist);
    }

    /**
     * @brief Resolve tuning ID, creating tuning if necessary.
     * @param[in,out] ctx Execution context.
     * @param[in] name Tuning name.
     * @return Existing or newly created tuning ID.
     */
    std::optional<qlonglong> resolveTuningId(ImportExecutionContext &ctx, const QString &name) {
        if (auto existing = ctx.tuningRepo.findTuningByName(name)) {
            return existing->id;
        }

        Tuning tuning;
        tuning.name = name;
        return ctx.tuningRepo.createTuning(tuning);
    }

    /**
     * @brief Import a single file synchronously.
     * @param[in,out] ctx Execution context.
     * @param[in] fileEntry File entry to import.
     * @param[in] strategy File storage strategy.
     * @return Result of the import attempt.
     */
    ImportResult importFileInternal(ImportExecutionContext &ctx, const ImportFileEntry &fileEntry,
                                    StorageStrategy strategy) {
        ImportResult result;
        const QString &absolutePath = fileEntry.absolutePath;
        result.sourcePath = absolutePath;

        const QFileInfo fileInfo(absolutePath);
        if (!fileInfo.exists() || !fileInfo.isFile()) {
            result.message = ImportService::tr("File does not exist");
            return result;
        }

        const QString extension = FileImportUtils::normalizedExtension(absolutePath);
        if (!FileImportUtils::isExtensionAllowed(extension, ctx.environment.allowedFileTypes)) {
            result.message = ImportService::tr("Unsupported file extension");
            return result;
        }

        const QString fileHash = FNV1a::calculate(absolutePath);
        if (fileHash.isEmpty()) {
            result.message = ImportService::tr("Could not calculate file hash");
            return result;
        }

        if (ctx.mediaFileRepo.findByHash(fileHash)) {
            result.status = ImportStatus::Skipped;
            result.message = ImportService::tr("Duplicate file hash");
            return result;
        }

        if (strategy == StorageStrategy::Link &&
            ctx.mediaFileRepo.findByPath(fileInfo.absoluteFilePath())) {
            result.status = ImportStatus::Skipped;
            result.message = ImportService::tr("File path already imported");
            return result;
        }

        StorageParameters storeParams{.fileHash = fileHash,
                                      .extension = extension,
                                      .sourcePath = absolutePath,
                                      .relativeSubPath =
                                          fileEntry.sourceRelativePath.isEmpty()
                                              ? QString()
                                              : QFileInfo(fileEntry.sourceRelativePath).path(),
                                      .managedRoot = ctx.environment.storageRoot};

        const ManagedFileResult storedFile = ManagedFileStorage::storeFile(strategy, storeParams);

        if (!storedFile.success) {
            result.message = storedFile.message;
            return result;
        }

        if (storedFile.duplicateContent) {
            result.status = ImportStatus::Skipped;
            result.message = ImportService::tr("Duplicate file hash");
            return result;
        }

        const MediaKind extensionKind = mediaKindForExtension(ctx.environment, extension);

        QString songTitle = fileInfo.completeBaseName();
        int baseBpm = 0;
        std::optional<qlonglong> artistId;
        std::optional<qlonglong> tuningId;

        if (extensionKind == MediaKind::GuitarPro) {
            const QString parsePath = storedFile.storedPath;
            const std::optional<SongMetadata> metadata =
                ctx.libraryManager.parseGuitarProFile(parsePath);

            if (!metadata.has_value()) {
                result.message = ImportService::tr("Could not parse Guitar Pro file");
                return result;
            }

            songTitle = metadata->title.isEmpty() ? songTitle : metadata->title;
            baseBpm = metadata->bpm;

            if (!metadata->artist.isEmpty()) {
                artistId = resolveArtistId(ctx, metadata->artist);
            }
            if (!metadata->defaultUiTuning.isEmpty()) {
                tuningId = resolveTuningId(ctx, metadata->defaultUiTuning);
            }
        }

        Song song;
        song.title = songTitle;
        song.baseBpm = baseBpm;
        song.artistId = artistId.value_or(0);
        song.tuningId = tuningId.value_or(0);

        const std::optional<qlonglong> songId = ctx.songRepo.createSong(song);
        if (!songId.has_value()) {
            result.message = ImportService::tr("Could not create song");
            return result;
        }

        const MediaStreamProbeResult streamInfo =
            MediaStreamProbe::probeOrInfer(storedFile.storedPath, extensionKind, extension);
        const MediaKind resolvedKind = classifyFromProbe(streamInfo, extensionKind);

        MediaFile mediaFile;
        mediaFile.songId = *songId;
        mediaFile.filePath = storedFile.isManaged ? storedFile.relativePath : storedFile.storedPath;
        mediaFile.fileType = extension;
        mediaFile.mediaKind = resolvedKind;
        mediaFile.hasVideo = streamInfo.hasVideo;
        mediaFile.hasAudio = streamInfo.hasAudio;
        mediaFile.fileSize = fileInfo.size();
        mediaFile.fileHash = fileHash;
        mediaFile.sourceType = MediaSourceType::Local;
        mediaFile.isManaged = storedFile.isManaged;
        mediaFile.canBePracticed = FileImportUtils::canBePracticed(resolvedKind);
        mediaFile.importRoot = fileEntry.importRoot;
        mediaFile.sourceRelativePath = fileEntry.sourceRelativePath;

        const std::optional<qlonglong> mediaFileId = ctx.mediaFileRepo.createMediaFile(mediaFile);

        if (!mediaFileId.has_value()) {
            ctx.songRepo.deleteSong(*songId);
            result.message = ImportService::tr("Could not create media file entry");
            return result;
        }

        result.status = ImportStatus::Imported;
        result.songId = *songId;
        result.mediaFileId = *mediaFileId;
        result.importRoot = fileEntry.importRoot;
        result.sourceRelativePath = fileEntry.sourceRelativePath;
        result.mediaKind = resolvedKind;
        result.songTitle = songTitle;
        result.message = ImportService::tr("Imported successfully");
        return result;
    }

    /**
     * @brief Run a batch import synchronously in a worker thread.
     * @param[in] job Batch job description.
     * @param[in,out] cancelRequested Flag to abort import.
     * @param[in] onProgress Progress callback.
     * @return Summary of the batch import or std::nullopt on failure.
     */
    std::optional<ImportSummary> runBatchImportSync(
        const ImportBatchJob &job, std::atomic_bool *cancelRequested,
        const std::function<void(int, int, const QString &, const ImportResult &)> &onProgress) {
        const QString connectionName =
            QStringLiteral("ImportWorker_%1")
                .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));

        SqliteConnection connection(connectionName);
        if (!connection.open(job.databasePath)) {
            return std::nullopt;
        }

        SqliteArtistRepository artistRepo(connection);
        SqliteTuningRepository tuningRepo(connection);
        SqliteSongRepository songRepo(connection);
        SqliteMediaFileRepository mediaFileRepo(connection);
        LibraryManager libraryManager;

        ImportExecutionContext ctx{
            .artistRepo = artistRepo,
            .tuningRepo = tuningRepo,
            .songRepo = songRepo,
            .mediaFileRepo = mediaFileRepo,
            .libraryManager = libraryManager,
            .environment = job.environment,
        };

        ImportSummary summary;
        const int total = static_cast<int>(job.files.size());

        for (int index = 0; index < total; ++index) {
            if (cancelRequested != nullptr && cancelRequested->load(std::memory_order_acquire)) {
                break;
            }

            const ImportFileEntry &fileEntry = job.files.at(index);
            const ImportResult result = importFileInternal(ctx, fileEntry, job.strategy);

            switch (result.status) {
            case ImportStatus::Imported:
                ++summary.importedCount;
                break;
            case ImportStatus::Skipped:
                ++summary.skippedCount;
                break;
            case ImportStatus::Failed:
                ++summary.failedCount;
                break;
            }

            if (onProgress) {
                onProgress(index + 1, total, fileEntry.absolutePath, result);
            }
        }

        connection.close();
        QSqlDatabase::removeDatabase(connectionName);
        return summary;
    }

} // namespace

/**
 * @brief Construct ImportService with dependencies.
 * @param[in] dependencies Service dependencies.
 * @param[in] parent Parent QObject (default nullptr).
 */
ImportService::ImportService(Dependencies dependencies, QObject *parent)
    : QObject(parent), m_dependencies(std::move(dependencies)) {
    qRegisterMetaType<ImportResult>();
    qRegisterMetaType<ImportSummary>();
}

/**
 * @brief Check if an import is in progress.
 * @return true if busy, otherwise false.
 */
bool ImportService::isBusy() const { return m_busy.load(std::memory_order_acquire); }

/**
 * @brief Get the current status message.
 * @return Status message string.
 */
const QString &ImportService::statusMessage() const { return m_statusMessage; }

/**
 * @brief Get current progress step.
 * @return Current progress index (0‑based).
 */
int ImportService::progressCurrent() const { return m_progressCurrent; }

/**
 * @brief Get total progress steps.
 * @return Total number of steps.
 */
int ImportService::progressTotal() const { return m_progressTotal; }

/**
 * @brief Get name of file currently being processed.
 * @return Current file name.
 */
const QString &ImportService::progressFileName() const { return m_progressFileName; }

/**
 * @brief Get number of successfully imported items in last batch.
 * @return Imported count.
 */
int ImportService::lastImportedCount() const { return m_lastImportedCount; }

/**
 * @brief Get number of skipped items in last batch.
 * @return Skipped count.
 */
int ImportService::lastSkippedCount() const { return m_lastSkippedCount; }

/**
 * @brief Get number of failed items in last batch.
 * @return Failed count.
 */
int ImportService::lastFailedCount() const { return m_lastFailedCount; }

/**
 * @brief Clear the current status message.
 */
void ImportService::clearStatusMessage() {
    if (m_statusMessage.isEmpty()) {
        return;
    }

    m_statusMessage.clear();
    emit statusMessageChanged();
}

/**
 * @brief Import a single file synchronously.
 * @param[in] absolutePath Full path of the file to import.
 * @param[in] strategy Desired storage strategy.
 * @return Result of the import.
 */
ImportResult ImportService::importFile(const QString &absolutePath, StorageStrategy strategy) {
    if (strategy == StorageStrategy::Link &&
        m_dependencies.appSettings.storageStrategyEnum() != StorageStrategy::Link) {
        strategy = m_dependencies.appSettings.storageStrategyEnum();
    }
    if (!tryBeginImport()) {
        ImportResult result;
        result.status = ImportStatus::Failed;
        result.sourcePath = absolutePath;
        result.message = tr("Import already running");
        setStatusMessage(result.message);
        emit errorOccurred(result.message);
        return result;
    }

    setProgress(0, 1, absolutePath);

    ImportExecutionContext ctx{
        .artistRepo = m_dependencies.artistRepo,
        .tuningRepo = m_dependencies.tuningRepo,
        .songRepo = m_dependencies.songRepo,
        .mediaFileRepo = m_dependencies.mediaFileRepo,
        .libraryManager = m_dependencies.libraryManager,
        .environment = buildImportEnvironment(m_dependencies),
    };
    const ImportFileEntry fileEntry{
        .absolutePath = absolutePath,
        .importRoot = {},
        .sourceRelativePath = QFileInfo(absolutePath).fileName(),
    };
    const ImportResult result = importFileInternal(ctx, fileEntry, strategy);
    setProgress(1, 1, absolutePath);
    emit fileImported(result);
    emit importProgress(1, 1, absolutePath);

    ImportSummary summary;
    switch (result.status) {
    case ImportStatus::Imported:
        summary.importedCount = 1;
        break;
    case ImportStatus::Skipped:
        summary.skippedCount = 1;
        break;
    case ImportStatus::Failed:
        summary.failedCount = 1;
        if (!result.message.isEmpty()) {
            emit errorOccurred(result.message);
        }
        break;
    }

    endImport();
    emitImportFinished(summary);
    return result;
}

/**
 * @brief Import all supported files inside a directory.
 * @param[in] directoryPath Path to the directory.
 * @param[in] strategy Desired storage strategy.
 */
void ImportService::importDirectory(const QString &directoryPath, StorageStrategy strategy) {
    if (!tryBeginImport()) {
        setStatusMessage(tr("Import already running"));
        emit errorOccurred(tr("Import already running"));
        return;
    }

    const QList<CollectedFile> collected = FileImportUtils::collectSupportedFilesWithPaths(
        directoryPath, m_dependencies.config.allowedFileTypes());

    if (collected.isEmpty()) {
        setStatusMessage(tr("No supported files found in directory"));
        emit errorOccurred(tr("No supported files found in directory"));
        endImport();
        emitImportFinished({});
        return;
    }

    QList<ImportFileEntry> entries;
    entries.reserve(collected.size());
    for (const CollectedFile &file : collected) {
        entries.append({.absolutePath = file.absolutePath,
                        .importRoot = file.importRoot,
                        .sourceRelativePath = file.sourceRelativePath});
    }

    startBatchImport(entries, strategy);
}

/**
 * @brief Import files located at the given paths.
 * @param[in] paths List of file or directory paths.
 * @param[in] strategy Desired storage strategy.
 */
void ImportService::importPaths(const QStringList &paths, StorageStrategy strategy) {
    if (paths.isEmpty()) {
        return;
    }

    if (strategy == StorageStrategy::Link &&
        m_dependencies.appSettings.storageStrategyEnum() != StorageStrategy::Link) {
        strategy = m_dependencies.appSettings.storageStrategyEnum();
    }

    if (!tryBeginImport()) {
        setStatusMessage(tr("Import already running"));
        emit errorOccurred(tr("Import already running"));
        return;
    }

    PathParams pathParams;
    pathParams.paths.append(paths);
    pathParams.extensions = m_dependencies.config.allowedFileTypes();

    PathParameters params(pathParams);

    const QList<CollectedFile> collected = FileImportUtils::collectEntriesFromPaths(params);

    if (collected.isEmpty()) {
        setStatusMessage(tr("No supported files found"));
        emit errorOccurred(tr("No supported files found"));
        endImport();
        emitImportFinished({});
        return;
    }

    QList<ImportFileEntry> entries;
    entries.reserve(collected.size());
    for (const CollectedFile &file : collected) {
        entries.append({.absolutePath = file.absolutePath,
                        .importRoot = file.importRoot,
                        .sourceRelativePath = file.sourceRelativePath});
    }

    QSet<QString> seenPaths;
    QList<ImportFileEntry> uniqueEntries;
    uniqueEntries.reserve(entries.size());
    for (const ImportFileEntry &entry : entries) {
        if (seenPaths.contains(entry.absolutePath)) {
            continue;
        }
        seenPaths.insert(entry.absolutePath);
        uniqueEntries.append(entry);
    }

    startBatchImport(uniqueEntries, strategy);
}

/**
 * @brief Request cancellation of an ongoing import.
*/
void ImportService::cancelImport() { m_cancelRequested.store(true, std::memory_order_release); }

/**
 * @brief Start a batch import in a worker thread.
 * @param[in] files Files to import.
 * @param[in] strategy Storage strategy.
 */
void ImportService::startBatchImport(const QList<ImportFileEntry> &files,
                                     StorageStrategy strategy) {
    ImportBatchJob job{
        .environment = buildImportEnvironment(m_dependencies),
        .databasePath = m_dependencies.databasePath,
        .files = files,
        .strategy = strategy,
    };

    if (job.strategy == StorageStrategy::Link &&
        job.environment.defaultStrategy != StorageStrategy::Link) {
        job.strategy = job.environment.defaultStrategy;
    }

    m_lastImportedCount = 0;
    m_lastSkippedCount = 0;
    m_lastFailedCount = 0;
    setProgress(0, static_cast<int>(files.size()), QString());

    QThread *thread = QThread::create([this, job]() {
        const std::optional<ImportSummary> summary = runBatchImportSync(
            job, &m_cancelRequested,
            [this](int current, int total, const QString &currentFile, const ImportResult &result) {
                QMetaObject::invokeMethod(this, "handleBatchProgress", Qt::QueuedConnection,
                                          Q_ARG(int, current), Q_ARG(int, total),
                                          Q_ARG(QString, currentFile), Q_ARG(ImportResult, result));
            });

        if (summary.has_value()) {
            QMetaObject::invokeMethod(this, "handleBatchFinished", Qt::QueuedConnection,
                                      Q_ARG(ImportSummary, *summary));
        } else {
            QMetaObject::invokeMethod(
                this, "handleBatchFailed", Qt::QueuedConnection,
                Q_ARG(QString, ImportService::tr("Could not open database for import")));
        }
    });

    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

/**
 * @brief Handle progress updates from a batch import.
 * @param[in] current Current step number.
 * @param[in] total Total steps.
 * @param[in] currentFile Path of the file currently processed.
 * @param[in] result Result of the processed file.
 */
void ImportService::handleBatchProgress(int current, int total, const QString &currentFile,
                                        const ImportResult &result) {
    setProgress(current, total, currentFile);
    emit fileImported(result);
    emit importProgress(current, total, currentFile);

    if (result.status == ImportStatus::Failed && !result.message.isEmpty()) {
        emit errorOccurred(result.message);
    }
}

/**
 * @brief Handle completion of a batch import.
 * @param[in] summary Import summary.
 */
void ImportService::handleBatchFinished(const ImportSummary &summary) {
    endImport();
    emitImportFinished(summary);
}

/**
 * @brief Handle failure to start a batch import.
 * @param[in] message Error message.
 */
void ImportService::handleBatchFailed(const QString &message) {
    setStatusMessage(message);
    emit errorOccurred(message);
    endImport();
    emitImportFinished({});
}

/**
 * @brief Check if a cancellation request was issued.
 * @return true if cancel requested, otherwise false.
 */
bool ImportService::isCancelled() const {
    return m_cancelRequested.load(std::memory_order_acquire);
}

/**
 * @brief Attempt to begin an import operation.
 * @return true if import started, otherwise false.
 */
bool ImportService::tryBeginImport() {
    if (m_busy.exchange(true, std::memory_order_acq_rel)) {
        return false;
    }

    m_cancelRequested.store(false, std::memory_order_release);
    m_progressCurrent = 0;
    m_progressTotal = 0;
    m_progressFileName.clear();
    emit progressChanged();
    emit busyChanged();
    return true;
}

/**
 * @brief Mark import as finished.
 */
void ImportService::endImport() {
    m_busy.store(false, std::memory_order_release);
    emit busyChanged();
}

/**
 * @brief Set current status message.
 * @param[in] message New status message.
 */
void ImportService::setStatusMessage(const QString &message) {
    if (m_statusMessage == message) {
        return;
    }

    m_statusMessage = message;
    emit statusMessageChanged();
}

/**
 * @brief Update progress state.
 * @param[in] current Current step.
 * @param[in] total Total steps.
 * @param[in] fileName Current file name.
 */
void ImportService::setProgress(int current, int total, const QString &fileName) {
    const bool changed =
        m_progressCurrent != current || m_progressTotal != total || m_progressFileName != fileName;
    if (!changed) {
        return;
    }

    m_progressCurrent = current;
    m_progressTotal = total;
    m_progressFileName = fileName;
    emit progressChanged();
}

/**
 * @brief Emit final summary after import.
 * @param[in] summary Import summary.
 */
void ImportService::emitImportFinished(const ImportSummary &summary) {
    m_lastImportedCount = summary.importedCount;
    m_lastSkippedCount = summary.skippedCount;
    m_lastFailedCount = summary.failedCount;
    emit lastSummaryChanged();

    const QString message = tr("%1 imported, %2 skipped, %3 failed")
                                .arg(summary.importedCount)
                                .arg(summary.skippedCount)
                                .arg(summary.failedCount);
    setStatusMessage(message);
    emit importFinished(summary);
}
