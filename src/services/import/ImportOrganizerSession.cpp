#include "ImportOrganizerSession.h"

#include "FileImportUtils.h"
#include "fnv1a.h"

#include <QDir>
#include <QFileInfo>
#include <algorithm>

namespace {

    QStringList splitRelativePath(const QString &relativePath) {
        const QString normalized = QDir::fromNativeSeparators(relativePath).trimmed();
        if (normalized.isEmpty() || normalized == QLatin1String(".")) {
            return {};
        }
        return normalized.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    }

    QString joinRelative(const QString &dir, const QString &name) {
        if (dir.isEmpty() || dir == QLatin1String(".")) {
            return name;
        }
        if (name.isEmpty()) {
            return dir;
        }
        return dir + QLatin1Char('/') + name;
    }

} // namespace

void ImportOrganizerSession::reset() {
    m_sourceNodes.clear();
    m_targetNodes.clear();
    m_sourceRootChildren.clear();
    m_targetRootChildren.clear();
    m_filterMode = OrganizerFilterMode::All;
    m_searchText.clear();
    m_nextId = 1;

    Node root;
    root.id = kRootNodeId;
    root.parentId = kInvalidNodeId;
    root.name = m_managedRootName;
    root.isFolder = true;
    root.expanded = true;
    m_targetNodes.insert(kRootNodeId, root);
}

void ImportOrganizerSession::setExistingHashes(QSet<QString> hashes) {
    m_existingHashes = std::move(hashes);
}

void ImportOrganizerSession::setManagedRootName(const QString &name) {
    const QString trimmed = name.trimmed();
    m_managedRootName = trimmed.isEmpty() ? QStringLiteral("Library") : trimmed;
    if (auto *root = nodeById(kRootNodeId)) {
        root->name = m_managedRootName;
    }
}

void ImportOrganizerSession::preloadTargetFolders(const QStringList &relativeFolderPaths) {
    for (const QString &relativePath : relativeFolderPaths) {
        const QStringList segments = splitRelativePath(relativePath);
        if (segments.isEmpty()) {
            continue;
        }
        const qint64 folderId = ensureFolderPath(false, kRootNodeId, segments);
        if (Node *folder = nodeById(folderId)) {
            folder->isExistingManaged = true;
        }
    }
}

bool ImportOrganizerSession::buildSourceFromScan(const QStringList &paths,
                                                const QStringList &allowedExtensions,
                                                const std::atomic_bool *cancelRequested) {
    m_sourceNodes.clear();
    m_sourceRootChildren.clear();

    PathParams pathParams;
    pathParams.paths = paths;
    pathParams.extensions = allowedExtensions;
    PathParameters params(pathParams);

    const QList<CollectedFile> collected =
        FileImportUtils::collectEntriesFromPaths(params, cancelRequested);
    if (cancelRequested != nullptr && cancelRequested->load(std::memory_order_acquire)) {
        return false;
    }

    struct ScannedFile {
        CollectedFile collected;
        QString hash;
        OrganizerEntryStatus status{OrganizerEntryStatus::Ready};
    };

    QList<ScannedFile> scanned;
    scanned.reserve(collected.size());
    QHash<QString, int> hashCounts;

    for (const CollectedFile &file : collected) {
        if (cancelRequested != nullptr && cancelRequested->load(std::memory_order_acquire)) {
            return false;
        }

        ScannedFile entry;
        entry.collected = file;
        const QFileInfo info(file.absolutePath);
        if (!info.exists() || !info.isFile() || info.size() <= 0) {
            entry.hash = QStringLiteral("0");
            entry.status = OrganizerEntryStatus::Defect;
        } else {
            entry.hash = FNV1a::calculate(file.absolutePath);
            if (entry.hash.isEmpty()) {
                entry.status = OrganizerEntryStatus::Defect;
            } else if (m_existingHashes.contains(entry.hash)) {
                entry.status = OrganizerEntryStatus::AlreadyInDatabase;
            } else {
                entry.status = OrganizerEntryStatus::Ready;
                hashCounts[entry.hash] += 1;
            }
        }
        scanned.append(entry);
    }

    QHash<QString, bool> firstDuplicateKept;
    for (ScannedFile &entry : scanned) {
        if (entry.status != OrganizerEntryStatus::Ready) {
            continue;
        }
        if (hashCounts.value(entry.hash) > 1) {
            entry.status = OrganizerEntryStatus::DuplicateInBatch;
        }
    }

    for (const ScannedFile &entry : scanned) {
        const QFileInfo info(entry.collected.absolutePath);
        const QStringList segments = splitRelativePath(entry.collected.sourceRelativePath);
        QStringList folderSegments = segments;
        QString fileName = info.fileName();
        if (!folderSegments.isEmpty()) {
            fileName = folderSegments.takeLast();
        }

        const qint64 parentId = ensureFolderPath(true, kRootNodeId, folderSegments);

        Node fileNode;
        fileNode.id = nextId();
        fileNode.parentId = parentId == kInvalidNodeId ? kRootNodeId : parentId;
        // For source, root children use parentId kRootNodeId but root isn't in sourceNodes.
        if (folderSegments.isEmpty()) {
            fileNode.parentId = kRootNodeId;
        }
        fileNode.name = fileName;
        fileNode.isFolder = false;
        fileNode.status = entry.status;
        fileNode.absolutePath = entry.collected.absolutePath;
        fileNode.importRoot = entry.collected.importRoot;
        fileNode.sourceDisplayPath = entry.collected.sourceRelativePath;
        fileNode.fileHash = entry.hash;

        if (entry.status == OrganizerEntryStatus::Ready) {
            fileNode.checked = true;
        } else if (entry.status == OrganizerEntryStatus::DuplicateInBatch) {
            if (!firstDuplicateKept.value(entry.hash, false)) {
                fileNode.checked = true;
                firstDuplicateKept.insert(entry.hash, true);
            } else {
                fileNode.checked = false;
            }
        } else {
            fileNode.checked = false;
        }

        m_sourceNodes.insert(fileNode.id, fileNode);
        if (fileNode.parentId == kRootNodeId) {
            m_sourceRootChildren.append(fileNode.id);
        }
    }

    return !scanned.isEmpty();
}

qint64 ImportOrganizerSession::addTargetFolder(qint64 parentFolderId, const QString &folderName) {
    const QString trimmed = folderName.trimmed();
    if (trimmed.isEmpty()) {
        return kInvalidNodeId;
    }

    qint64 parentId = parentFolderId;
    if (parentId == kInvalidNodeId) {
        parentId = kRootNodeId;
    }
    const Node *parent = nodeById(parentId);
    if (parent == nullptr || !parent->isFolder) {
        return kInvalidNodeId;
    }

    if (const qint64 existing = findChildFolder(false, parentId, trimmed);
        existing != kInvalidNodeId) {
        return existing;
    }

    Node folder;
    folder.id = nextId();
    folder.parentId = parentId;
    folder.name = trimmed;
    folder.isFolder = true;
    folder.expanded = true;
    m_targetNodes.insert(folder.id, folder);
    if (parentId == kRootNodeId) {
        m_targetRootChildren.append(folder.id);
    }
    return folder.id;
}

qint64 ImportOrganizerSession::resolveTargetFolderId(qint64 nodeId) const {
    if (nodeId == kInvalidNodeId) {
        return kRootNodeId;
    }
    const Node *node = nodeById(nodeId);
    if (node == nullptr) {
        return kRootNodeId;
    }
    if (node->isFolder) {
        return nodeId;
    }
    return node->parentId == kInvalidNodeId ? kRootNodeId : node->parentId;
}

bool ImportOrganizerSession::mapNodes(const QList<qint64> &sourceNodeIds, qint64 targetFolderId,
                                      bool includeFolderWrapper) {
    const qint64 targetId = resolveTargetFolderId(targetFolderId);
    const Node *targetFolder = nodeById(targetId);
    if (targetFolder == nullptr || !targetFolder->isFolder) {
        return false;
    }

    bool mappedAny = false;
    QList<qint64> mappedSourceIds;
    for (const qint64 sourceId : sourceNodeIds) {
        if (!m_sourceNodes.contains(sourceId)) {
            continue;
        }

        const Node &source = m_sourceNodes.value(sourceId);
        if (source.isFolder) {
            if (deepCopyToTarget(sourceId, targetId, includeFolderWrapper, &mappedSourceIds) !=
                kInvalidNodeId) {
                mappedAny = true;
            }
        } else if (source.checked && source.status != OrganizerEntryStatus::AlreadyInDatabase &&
                   source.status != OrganizerEntryStatus::Defect) {
            if (deepCopyToTarget(sourceId, targetId, true, &mappedSourceIds) != kInvalidNodeId) {
                mappedAny = true;
            }
        }
    }

    for (const qint64 mappedId : mappedSourceIds) {
        if (m_sourceNodes.contains(mappedId)) {
            removeNodeRecursive(true, mappedId);
        }
    }

    if (mappedAny) {
        cleanupEmptyFolders(true, kRootNodeId);
    }
    return mappedAny;
}

bool ImportOrganizerSession::unmapNodes(const QList<qint64> &targetNodeIds) {
    bool unmappedAny = false;
    QList<qint64> ordered = targetNodeIds;
    // Unmap deeper nodes first by processing files then folders.
    std::ranges::sort(ordered, [this](qint64 a, qint64 b) {
        const Node *na = m_targetNodes.contains(a) ? &m_targetNodes[a] : nullptr;
        const Node *nb = m_targetNodes.contains(b) ? &m_targetNodes[b] : nullptr;
        if (na == nullptr || nb == nullptr) {
            return a > b;
        }
        return (!na->isFolder && nb->isFolder) || a > b;
    });

    for (const qint64 nodeId : ordered) {
        if (!m_targetNodes.contains(nodeId) || nodeId == kRootNodeId) {
            continue;
        }
        Node node = m_targetNodes.value(nodeId);
        if (node.isExistingManaged && node.isFolder) {
            // Keep preloaded empty folders; only unmap staged content under them.
            QList<qint64> children = childIds(false, nodeId);
            for (const qint64 childId : children) {
                if (unmapNodes({childId})) {
                    unmappedAny = true;
                }
            }
            continue;
        }

        if (node.isFolder) {
            QList<qint64> children = childIds(false, nodeId);
            for (const qint64 childId : children) {
                if (unmapNodes({childId})) {
                    unmappedAny = true;
                }
            }
            // Remove folder if it was created in this session and is now empty.
            if (!node.isExistingManaged && childIds(false, nodeId).isEmpty()) {
                removeNodeRecursive(false, nodeId);
                unmappedAny = true;
            }
            continue;
        }

        restoreSourcePath(node);
        removeNodeRecursive(false, nodeId);
        unmappedAny = true;
    }

    if (unmappedAny) {
        cleanupEmptyFolders(false, kRootNodeId);
    }
    return unmappedAny;
}

bool ImportOrganizerSession::setSourceChecked(qint64 nodeId, bool checked) {
    Node *node = m_sourceNodes.contains(nodeId) ? &m_sourceNodes[nodeId] : nullptr;
    if (node == nullptr || node->isFolder) {
        return false;
    }
    if (node->status == OrganizerEntryStatus::AlreadyInDatabase ||
        node->status == OrganizerEntryStatus::Defect) {
        return false;
    }

    if (checked && node->status == OrganizerEntryStatus::DuplicateInBatch &&
        !node->fileHash.isEmpty()) {
        for (auto it = m_sourceNodes.begin(); it != m_sourceNodes.end(); ++it) {
            if (it->isFolder || it.key() == nodeId) {
                continue;
            }
            if (it->fileHash == node->fileHash) {
                it->checked = false;
            }
        }
    }

    node->checked = checked;
    return true;
}

bool ImportOrganizerSession::setExpanded(bool sourceSide, qint64 nodeId, bool expanded) {
    if (!sourceSide && nodeId == kRootNodeId) {
        if (Node *root = nodeById(kRootNodeId)) {
            root->expanded = expanded;
            return true;
        }
        return false;
    }

    QHash<qint64, Node> &nodes = sourceSide ? m_sourceNodes : m_targetNodes;
    auto it = nodes.find(nodeId);
    if (it == nodes.end() || !it->isFolder) {
        return false;
    }
    it->expanded = expanded;
    return true;
}

void ImportOrganizerSession::expandAll(bool sourceSide, bool expanded) {
    QHash<qint64, Node> &nodes = sourceSide ? m_sourceNodes : m_targetNodes;
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        if (it->isFolder) {
            it->expanded = expanded;
        }
    }
    if (!sourceSide) {
        if (Node *root = nodeById(kRootNodeId)) {
            root->expanded = expanded;
        }
    }
}

void ImportOrganizerSession::setFilterMode(OrganizerFilterMode mode) { m_filterMode = mode; }

void ImportOrganizerSession::setSearchText(const QString &text) { m_searchText = text.trimmed(); }

OrganizerFilterMode ImportOrganizerSession::filterMode() const { return m_filterMode; }

QString ImportOrganizerSession::searchText() const { return m_searchText; }

QList<ImportOrganizerSession::FlatRow>
ImportOrganizerSession::sourceFlatRows() const {
    QList<FlatRow> rows;
    buildFlat(true, kRootNodeId, 0, rows);
    return rows;
}

QList<ImportOrganizerSession::FlatRow>
ImportOrganizerSession::targetFlatRows() const {
    QList<FlatRow> rows;
    // Always show root.
    const Node *root = nodeById(kRootNodeId);
    if (root == nullptr) {
        return rows;
    }
    FlatRow rootRow;
    rootRow.nodeId = kRootNodeId;
    rootRow.parentId = kInvalidNodeId;
    rootRow.depth = 0;
    rootRow.name = root->name;
    rootRow.isFolder = true;
    rootRow.expanded = root->expanded;
    rootRow.hasChildren = !m_targetRootChildren.isEmpty();
    rows.append(rootRow);
    if (root->expanded) {
        buildFlat(false, kRootNodeId, 1, rows);
    }
    return rows;
}

QList<OrganizerPlannedEntry> ImportOrganizerSession::collectPlannedEntries() const {
    QList<OrganizerPlannedEntry> entries;
    collectPlannedRecursive(kRootNodeId, QString(), entries);
    return entries;
}

int ImportOrganizerSession::plannedFileCount() const {
    return static_cast<int>(collectPlannedEntries().size());
}

int ImportOrganizerSession::sourceFileCount() const {
    int count = 0;
    for (const Node &node : m_sourceNodes) {
        if (!node.isFolder) {
            ++count;
        }
    }
    return count;
}

QString ImportOrganizerSession::managedRootName() const { return m_managedRootName; }

qint64 ImportOrganizerSession::nextId() { return m_nextId++; }

ImportOrganizerSession::Node *ImportOrganizerSession::nodeById(qint64 id) {
    if (id == kRootNodeId) {
        auto it = m_targetNodes.find(kRootNodeId);
        return it == m_targetNodes.end() ? nullptr : &(*it);
    }
    if (m_sourceNodes.contains(id)) {
        return &m_sourceNodes[id];
    }
    if (m_targetNodes.contains(id)) {
        return &m_targetNodes[id];
    }
    return nullptr;
}

const ImportOrganizerSession::Node *ImportOrganizerSession::nodeById(qint64 id) const {
    if (id == kRootNodeId) {
        auto it = m_targetNodes.constFind(kRootNodeId);
        return it == m_targetNodes.cend() ? nullptr : &(*it);
    }
    auto sourceIt = m_sourceNodes.constFind(id);
    if (sourceIt != m_sourceNodes.cend()) {
        return &(*sourceIt);
    }
    auto targetIt = m_targetNodes.constFind(id);
    if (targetIt != m_targetNodes.cend()) {
        return &(*targetIt);
    }
    return nullptr;
}

QList<qint64> ImportOrganizerSession::childIds(bool sourceSide, qint64 parentId) const {
    if (sourceSide) {
        if (parentId == kRootNodeId) {
            return m_sourceRootChildren;
        }
        QList<qint64> children;
        for (auto it = m_sourceNodes.constBegin(); it != m_sourceNodes.constEnd(); ++it) {
            if (it->parentId == parentId) {
                children.append(it.key());
            }
        }
        std::ranges::sort(children, [this](qint64 a, qint64 b) {
            return m_sourceNodes.value(a).name.localeAwareCompare(m_sourceNodes.value(b).name) < 0;
        });
        return children;
    }

    if (parentId == kRootNodeId) {
        QList<qint64> children = m_targetRootChildren;
        // Also include nodes that claim parent root but weren't tracked (defensive).
        for (auto it = m_targetNodes.constBegin(); it != m_targetNodes.constEnd(); ++it) {
            if (it.key() == kRootNodeId) {
                continue;
            }
            if (it->parentId == kRootNodeId && !children.contains(it.key())) {
                children.append(it.key());
            }
        }
        std::ranges::sort(children, [this](qint64 a, qint64 b) {
            return m_targetNodes.value(a).name.localeAwareCompare(m_targetNodes.value(b).name) < 0;
        });
        return children;
    }

    QList<qint64> children;
    for (auto it = m_targetNodes.constBegin(); it != m_targetNodes.constEnd(); ++it) {
        if (it.key() != kRootNodeId && it->parentId == parentId) {
            children.append(it.key());
        }
    }
    std::ranges::sort(children, [this](qint64 a, qint64 b) {
        return m_targetNodes.value(a).name.localeAwareCompare(m_targetNodes.value(b).name) < 0;
    });
    return children;
}

qint64 ImportOrganizerSession::ensureFolderPath(bool sourceSide, qint64 parentId,
                                                const QStringList &segments) {
    qint64 current = parentId;
    for (const QString &segment : segments) {
        qint64 existing = findChildFolder(sourceSide, current, segment);
        if (existing != kInvalidNodeId) {
            current = existing;
            continue;
        }

        Node folder;
        folder.id = nextId();
        folder.parentId = current;
        folder.name = segment;
        folder.isFolder = true;
        folder.expanded = true;

        if (sourceSide) {
            m_sourceNodes.insert(folder.id, folder);
            if (current == kRootNodeId) {
                m_sourceRootChildren.append(folder.id);
            }
        } else {
            m_targetNodes.insert(folder.id, folder);
            if (current == kRootNodeId) {
                m_targetRootChildren.append(folder.id);
            }
        }
        current = folder.id;
    }
    return current;
}

qint64 ImportOrganizerSession::findChildFolder(bool sourceSide, qint64 parentId,
                                               const QString &name) const {
    const QList<qint64> children = childIds(sourceSide, parentId);
    const QHash<qint64, Node> &nodes = sourceSide ? m_sourceNodes : m_targetNodes;
    for (const qint64 childId : children) {
        const Node &child = nodes.value(childId);
        if (child.isFolder && child.name == name) {
            return childId;
        }
    }
    return kInvalidNodeId;
}

void ImportOrganizerSession::removeNodeRecursive(bool sourceSide, qint64 nodeId) {
    if (!sourceSide && nodeId == kRootNodeId) {
        return;
    }

    const QList<qint64> children = childIds(sourceSide, nodeId);
    for (const qint64 childId : children) {
        removeNodeRecursive(sourceSide, childId);
    }

    QHash<qint64, Node> &nodes = sourceSide ? m_sourceNodes : m_targetNodes;
    QList<qint64> &rootChildren = sourceSide ? m_sourceRootChildren : m_targetRootChildren;

    const auto it = nodes.constFind(nodeId);
    if (it == nodes.cend()) {
        return;
    }
    const qint64 parentId = it->parentId;
    nodes.remove(nodeId);
    rootChildren.removeAll(nodeId);

    if (!sourceSide && parentId != kRootNodeId && parentId != kInvalidNodeId) {
        // parent list is recomputed via parentId scan
        Q_UNUSED(parentId);
    }
}

void ImportOrganizerSession::cleanupEmptyFolders(bool sourceSide, qint64 parentId) {
    QList<qint64> children = childIds(sourceSide, parentId);
    for (const qint64 childId : children) {
        const Node *child = nullptr;
        if (sourceSide) {
            const auto it = m_sourceNodes.constFind(childId);
            child = it == m_sourceNodes.cend() ? nullptr : &(*it);
        } else {
            child = nodeById(childId);
        }
        if (child == nullptr || !child->isFolder) {
            continue;
        }
        if (!sourceSide && child->isExistingManaged) {
            cleanupEmptyFolders(sourceSide, childId);
            continue;
        }
        cleanupEmptyFolders(sourceSide, childId);
        if (childIds(sourceSide, childId).isEmpty()) {
            removeNodeRecursive(sourceSide, childId);
        }
    }
}

qint64 ImportOrganizerSession::deepCopyToTarget(qint64 sourceNodeId, qint64 targetParentId,
                                                bool includeFolderWrapper,
                                                QList<qint64> *mappedSourceIds) {
    if (!m_sourceNodes.contains(sourceNodeId)) {
        return kInvalidNodeId;
    }
    const Node source = m_sourceNodes.value(sourceNodeId);

    if (source.isFolder) {
        qint64 folderTargetId = targetParentId;
        if (includeFolderWrapper) {
            folderTargetId = findChildFolder(false, targetParentId, source.name);
            if (folderTargetId == kInvalidNodeId) {
                Node folder;
                folder.id = nextId();
                folder.parentId = targetParentId;
                folder.name = source.name;
                folder.isFolder = true;
                folder.expanded = true;
                m_targetNodes.insert(folder.id, folder);
                if (targetParentId == kRootNodeId) {
                    m_targetRootChildren.append(folder.id);
                }
                folderTargetId = folder.id;
            }
        }

        bool copiedAny = false;
        for (const qint64 childId : childIds(true, sourceNodeId)) {
            const Node &child = m_sourceNodes.value(childId);
            if (child.isFolder) {
                if (deepCopyToTarget(childId, folderTargetId, true, mappedSourceIds) !=
                    kInvalidNodeId) {
                    copiedAny = true;
                }
            } else if (child.checked && child.status != OrganizerEntryStatus::AlreadyInDatabase &&
                       child.status != OrganizerEntryStatus::Defect) {
                if (deepCopyToTarget(childId, folderTargetId, true, mappedSourceIds) !=
                    kInvalidNodeId) {
                    copiedAny = true;
                }
            }
        }
        return copiedAny ? folderTargetId : kInvalidNodeId;
    }

    // Avoid colliding file names in the same target folder by appending _verN at plan time.
    QString targetName = source.name;
    {
        int counter = 1;
        while (true) {
            bool nameTaken = false;
            for (const qint64 childId : childIds(false, targetParentId)) {
                const Node &child = m_targetNodes.value(childId);
                if (!child.isFolder && child.name == targetName) {
                    nameTaken = true;
                    break;
                }
            }
            if (!nameTaken) {
                break;
            }
            const QFileInfo info(source.name);
            targetName = info.completeSuffix().isEmpty()
                             ? QStringLiteral("%1_ver%2").arg(info.completeBaseName()).arg(counter)
                             : QStringLiteral("%1_ver%2.%3")
                                   .arg(info.completeBaseName())
                                   .arg(counter)
                                   .arg(info.completeSuffix());
            ++counter;
            if (counter > 10000) {
                break;
            }
        }
    }

    Node fileNode = source;
    fileNode.id = nextId();
    fileNode.parentId = targetParentId;
    fileNode.name = targetName;
    fileNode.isFolder = false;
    fileNode.isExistingManaged = false;
    m_targetNodes.insert(fileNode.id, fileNode);
    if (targetParentId == kRootNodeId) {
        m_targetRootChildren.append(fileNode.id);
    }
    if (mappedSourceIds != nullptr) {
        mappedSourceIds->append(sourceNodeId);
    }
    return fileNode.id;
}

void ImportOrganizerSession::restoreSourcePath(const Node &fileNode) {
    const QStringList segments = splitRelativePath(fileNode.sourceDisplayPath);
    QStringList folderSegments = segments;
    QString fileName = fileNode.name;
    if (!folderSegments.isEmpty()) {
        fileName = folderSegments.takeLast();
    } else {
        fileName = QFileInfo(fileNode.absolutePath).fileName();
    }

    const qint64 parentId = ensureFolderPath(true, kRootNodeId, folderSegments);
    Node restored = fileNode;
    restored.id = nextId();
    restored.parentId = folderSegments.isEmpty() ? kRootNodeId : parentId;
    restored.name = fileName;
    restored.isFolder = false;
    restored.checked = true;
    m_sourceNodes.insert(restored.id, restored);
    if (restored.parentId == kRootNodeId) {
        m_sourceRootChildren.append(restored.id);
    }
}

void ImportOrganizerSession::collectPlannedRecursive(qint64 parentId, const QString &relativeDir,
                                                     QList<OrganizerPlannedEntry> &out) const {
    for (const qint64 childId : childIds(false, parentId)) {
        const Node &child = m_targetNodes.value(childId);
        if (child.isFolder) {
            collectPlannedRecursive(childId, joinRelative(relativeDir, child.name), out);
            continue;
        }
        OrganizerPlannedEntry entry;
        entry.absolutePath = child.absolutePath;
        entry.importRoot = child.importRoot;
        entry.sourceRelativePath = joinRelative(relativeDir, child.name);
        entry.fileHash = child.fileHash;
        out.append(entry);
    }
}

void ImportOrganizerSession::buildFlat(bool sourceSide, qint64 parentId, int depth,
                                       QList<FlatRow> &out) const {
    for (const qint64 childId : childIds(sourceSide, parentId)) {
        const Node *child = nullptr;
        if (sourceSide) {
            const auto it = m_sourceNodes.constFind(childId);
            child = it == m_sourceNodes.cend() ? nullptr : &(*it);
        } else {
            child = nodeById(childId);
        }
        if (child == nullptr) {
            continue;
        }

        if (sourceSide) {
            if (child->isFolder) {
                if (!subtreeHasVisibleContent(true, childId)) {
                    continue;
                }
            } else if (!nodeMatchesFilter(*child)) {
                continue;
            }
        }

        FlatRow row;
        row.nodeId = child->id;
        row.parentId = child->parentId;
        row.depth = depth;
        row.name = child->name;
        row.isFolder = child->isFolder;
        row.isExistingManaged = child->isExistingManaged;
        row.checked = child->checked;
        row.expanded = child->expanded;
        row.hasChildren = !childIds(sourceSide, childId).isEmpty();
        row.status = child->status;
        row.absolutePath = child->absolutePath;
        row.statusLabel = child->isFolder ? QString() : statusLabelFor(child->status);
        out.append(row);

        if (child->isFolder && child->expanded) {
            buildFlat(sourceSide, childId, depth + 1, out);
        }
    }
}

bool ImportOrganizerSession::nodeMatchesFilter(const Node &node) const {
    if (!m_searchText.isEmpty() &&
        !node.name.contains(m_searchText, Qt::CaseInsensitive) &&
        !node.sourceDisplayPath.contains(m_searchText, Qt::CaseInsensitive)) {
        return false;
    }

    switch (m_filterMode) {
    case OrganizerFilterMode::All:
        return node.status != OrganizerEntryStatus::AlreadyInDatabase;
    case OrganizerFilterMode::Duplicates:
        return node.status == OrganizerEntryStatus::DuplicateInBatch;
    case OrganizerFilterMode::Errors:
        return node.status == OrganizerEntryStatus::Defect;
    }
    return true;
}

bool ImportOrganizerSession::subtreeHasVisibleContent(bool sourceSide, qint64 nodeId) const {
    for (const qint64 childId : childIds(sourceSide, nodeId)) {
        const Node *child = nullptr;
        if (sourceSide) {
            const auto it = m_sourceNodes.constFind(childId);
            child = it == m_sourceNodes.cend() ? nullptr : &(*it);
        } else {
            child = nodeById(childId);
        }
        if (child == nullptr) {
            continue;
        }
        if (child->isFolder) {
            if (subtreeHasVisibleContent(sourceSide, childId)) {
                return true;
            }
        } else if (nodeMatchesFilter(*child)) {
            return true;
        }
    }
    return false;
}

QString ImportOrganizerSession::statusLabelFor(OrganizerEntryStatus status) {
    switch (status) {
    case OrganizerEntryStatus::Ready:
        return {};
    case OrganizerEntryStatus::DuplicateInBatch:
        return QStringLiteral("duplicate");
    case OrganizerEntryStatus::AlreadyInDatabase:
        return QStringLiteral("in database");
    case OrganizerEntryStatus::Defect:
        return QStringLiteral("error");
    }
    return {};
}
