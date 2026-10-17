#include "ImportOrganizerController.h"

#include "AppSettings.h"
#include "ImportService.h"
#include "MediaFile.h"
#include "interfaces/IMediaFileRepository.h"
#include "interfaces/iConfigProvider.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMetaObject>
#include <QThread>

ImportOrganizerController::ImportOrganizerController(Dependencies dependencies, QObject *parent)
    : QObject(parent), m_dependencies(dependencies), m_sourceModel(this), m_targetModel(this) {}

bool ImportOrganizerController::active() const { return m_active; }

bool ImportOrganizerController::scanning() const { return m_scanning; }

bool ImportOrganizerController::canCommit() const { return m_session.plannedFileCount() > 0; }

int ImportOrganizerController::plannedCount() const { return m_session.plannedFileCount(); }

int ImportOrganizerController::sourceCount() const { return m_session.sourceFileCount(); }

QString ImportOrganizerController::statusMessage() const { return m_statusMessage; }

QString ImportOrganizerController::managedRootName() const { return m_session.managedRootName(); }

int ImportOrganizerController::filterMode() const {
    return static_cast<int>(m_session.filterMode());
}

void ImportOrganizerController::setFilterMode(int mode) {
    const auto typed = static_cast<OrganizerFilterMode>(mode);
    if (typed == m_session.filterMode()) {
        return;
    }
    m_session.setFilterMode(typed);
    refreshModels();
    emit filterChanged();
}

QString ImportOrganizerController::searchText() const { return m_session.searchText(); }

void ImportOrganizerController::setSearchText(const QString &text) {
    if (m_session.searchText() == text.trimmed()) {
        return;
    }
    m_session.setSearchText(text);
    refreshModels();
    emit filterChanged();
}

ImportOrganizerTreeModel *ImportOrganizerController::sourceModel() { return &m_sourceModel; }

ImportOrganizerTreeModel *ImportOrganizerController::targetModel() { return &m_targetModel; }

qint64 ImportOrganizerController::selectedSourceNodeId() const { return m_selectedSourceNodeId; }

void ImportOrganizerController::setSelectedSourceNodeId(qint64 nodeId) {
    if (m_selectedSourceNodeId == nodeId) {
        return;
    }
    m_selectedSourceNodeId = nodeId;
    emit selectionChanged();
}

qint64 ImportOrganizerController::selectedTargetNodeId() const { return m_selectedTargetNodeId; }

void ImportOrganizerController::setSelectedTargetNodeId(qint64 nodeId) {
    if (m_selectedTargetNodeId == nodeId) {
        return;
    }
    m_selectedTargetNodeId = nodeId;
    emit selectionChanged();
}

void ImportOrganizerController::begin(const QStringList &paths) {
    if (paths.isEmpty() || m_scanning) {
        return;
    }

    m_scanCancel.store(false, std::memory_order_release);
    m_session.reset();

    const QString managedRoot = m_dependencies.appSettings.managedStorageRoot();
    const QString rootName = QFileInfo(managedRoot).fileName();
    m_session.setManagedRootName(rootName.isEmpty() ? QStringLiteral("Library") : rootName);
    m_session.setExistingHashes(existingHashes());
    m_session.preloadTargetFolders(existingManagedFolderPaths());

    m_selectedSourceNodeId = ImportOrganizerSession::kInvalidNodeId;
    m_selectedTargetNodeId = ImportOrganizerSession::kRootNodeId;
    setActive(true);
    setScanning(true);
    setStatusMessage(tr("Searching for files…"));
    refreshModels();
    emit selectionChanged();

    const QStringList allowed = m_dependencies.config.allowedFileTypes();
    const QStringList pathsCopy = paths;

    QThread *thread = QThread::create([this, pathsCopy, allowed]() {
        const bool ok =
            m_session.buildSourceFromScan(pathsCopy, allowed, &m_scanCancel);
        const QString message =
            ok ? QString()
               : (m_scanCancel.load(std::memory_order_acquire)
                      ? tr("Scan cancelled")
                      : tr("No supported files found"));
        QMetaObject::invokeMethod(this, "handleScanFinished", Qt::QueuedConnection,
                                  Q_ARG(bool, ok), Q_ARG(QString, message));
    });
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void ImportOrganizerController::cancel() {
    if (m_scanning) {
        m_scanCancel.store(true, std::memory_order_release);
        return;
    }
    m_session.reset();
    setActive(false);
    setStatusMessage({});
    refreshModels();
    emit planChanged();
}

void ImportOrganizerController::commit() {
    if (!canCommit() || m_scanning || m_dependencies.importService.isBusy()) {
        return;
    }

    const QList<OrganizerPlannedEntry> planned = m_session.collectPlannedEntries();
    QList<ImportFileEntry> entries;
    entries.reserve(planned.size());
    for (const OrganizerPlannedEntry &entry : planned) {
        entries.append(toImportFileEntry(entry));
    }

    setActive(false);
    setStatusMessage({});
    m_session.reset();
    refreshModels();
    emit planChanged();

    m_dependencies.importService.importEntries(entries,
                                               m_dependencies.appSettings.storageStrategyEnum());
}

void ImportOrganizerController::mapSelection(bool includeFolderWrapper) {
    if (m_scanning || m_selectedSourceNodeId == ImportOrganizerSession::kInvalidNodeId) {
        return;
    }
    if (m_session.mapNodes({m_selectedSourceNodeId}, m_selectedTargetNodeId,
                           includeFolderWrapper)) {
        m_selectedSourceNodeId = ImportOrganizerSession::kInvalidNodeId;
        refreshModels();
        emit planChanged();
        emit selectionChanged();
    }
}

void ImportOrganizerController::mapSelectionContentsOnly() { mapSelection(false); }

void ImportOrganizerController::unmapSelection() {
    if (m_scanning || m_selectedTargetNodeId == ImportOrganizerSession::kInvalidNodeId ||
        m_selectedTargetNodeId == ImportOrganizerSession::kRootNodeId) {
        return;
    }
    if (m_session.unmapNodes({m_selectedTargetNodeId})) {
        m_selectedTargetNodeId = ImportOrganizerSession::kRootNodeId;
        refreshModels();
        emit planChanged();
        emit selectionChanged();
    }
}

void ImportOrganizerController::addFolder(const QString &folderName) {
    if (m_scanning) {
        return;
    }
    const qint64 parentId = m_session.resolveTargetFolderId(m_selectedTargetNodeId);
    const qint64 created = m_session.addTargetFolder(parentId, folderName);
    if (created == ImportOrganizerSession::kInvalidNodeId) {
        return;
    }
    m_selectedTargetNodeId = created;
    refreshModels();
    emit selectionChanged();
    emit planChanged();
}

void ImportOrganizerController::setChecked(qint64 nodeId, bool checked) {
    if (m_session.setSourceChecked(nodeId, checked)) {
        refreshModels();
        emit planChanged();
    }
}

void ImportOrganizerController::toggleExpanded(bool sourceSide, qint64 nodeId) {
    // Read current expanded state from the visible model rows is awkward; flip via session.
    // Session stores expanded on the node — toggle by setting opposite of current flat row.
    const auto &model = sourceSide ? m_sourceModel : m_targetModel;
    bool currentlyExpanded = true;
    for (int row = 0; row < model.rowCount(); ++row) {
        const QModelIndex index = model.index(row, 0);
        if (model.data(index, ImportOrganizerTreeModel::NodeIdRole).toLongLong() == nodeId) {
            currentlyExpanded =
                model.data(index, ImportOrganizerTreeModel::ExpandedRole).toBool();
            break;
        }
    }
    if (m_session.setExpanded(sourceSide, nodeId, !currentlyExpanded)) {
        refreshModels();
    }
}

void ImportOrganizerController::expandAll(bool sourceSide) {
    m_session.expandAll(sourceSide, true);
    refreshModels();
}

void ImportOrganizerController::collapseAll(bool sourceSide) {
    m_session.expandAll(sourceSide, false);
    refreshModels();
}

void ImportOrganizerController::handleScanFinished(bool ok, const QString &message) {
    setScanning(false);
    if (!ok) {
        setStatusMessage(message);
        emit scanFailed(message);
        if (m_scanCancel.load(std::memory_order_acquire)) {
            setActive(false);
        }
        refreshModels();
        emit planChanged();
        return;
    }
    setStatusMessage(tr("%1 file(s) ready to organize").arg(m_session.sourceFileCount()));
    refreshModels();
    emit planChanged();
}

void ImportOrganizerController::refreshModels() {
    m_sourceModel.setRows(m_session.sourceFlatRows());
    m_targetModel.setRows(m_session.targetFlatRows());
}

void ImportOrganizerController::setActive(bool active) {
    if (m_active == active) {
        return;
    }
    m_active = active;
    emit activeChanged();
}

void ImportOrganizerController::setScanning(bool scanning) {
    if (m_scanning == scanning) {
        return;
    }
    m_scanning = scanning;
    emit scanningChanged();
}

void ImportOrganizerController::setStatusMessage(const QString &message) {
    if (m_statusMessage == message) {
        return;
    }
    m_statusMessage = message;
    emit statusMessageChanged();
}

QStringList ImportOrganizerController::existingManagedFolderPaths() const {
    QSet<QString> folders;
    const QString managedRoot = m_dependencies.appSettings.managedStorageRoot();
    if (!managedRoot.trimmed().isEmpty()) {
        QDirIterator it(managedRoot, QDir::Dirs | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            const QString relative = QDir(managedRoot).relativeFilePath(it.filePath());
            if (!relative.isEmpty() && relative != QLatin1String(".")) {
                folders.insert(QDir::fromNativeSeparators(relative));
            }
        }
    }

    for (const MediaFile &media : m_dependencies.mediaFileRepo.getAllMediaFiles()) {
        if (!media.isManaged) {
            continue;
        }
        const QString relative =
            !media.sourceRelativePath.isEmpty() ? media.sourceRelativePath : media.filePath;
        const QString folder = QFileInfo(QDir::fromNativeSeparators(relative)).path();
        if (!folder.isEmpty() && folder != QLatin1String(".")) {
            folders.insert(folder);
        }
    }

    return QStringList(folders.begin(), folders.end());
}

QSet<QString> ImportOrganizerController::existingHashes() const {
    QSet<QString> hashes;
    for (const MediaFile &media : m_dependencies.mediaFileRepo.getAllMediaFiles()) {
        if (!media.fileHash.trimmed().isEmpty()) {
            hashes.insert(media.fileHash);
        }
    }
    return hashes;
}
