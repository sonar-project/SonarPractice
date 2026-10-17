#ifndef IMPORTORGANIZERSESSION_H
#define IMPORTORGANIZERSESSION_H

#include "ImportOrganizerTypes.h"

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>
#include <atomic>
#include <cstdint>

/**
 * \brief In-memory source/target trees for the copy/move import organizer.
 *
 * Pure logic (no QObject): scan → dedupe → map/unmap → collect planned entries.
 */
class ImportOrganizerSession {
  public:
    static constexpr qint64 kInvalidNodeId = -1;
    static constexpr qint64 kRootNodeId = 0;

    struct Node {
        qint64 id{kInvalidNodeId};
        qint64 parentId{kRootNodeId};
        QString name{};
        bool isFolder{false};
        bool isExistingManaged{false};
        bool checked{false};
        bool expanded{true};
        OrganizerEntryStatus status{OrganizerEntryStatus::Ready};
        QString absolutePath{};
        QString importRoot{};
        QString sourceDisplayPath{};
        QString fileHash{};
    };

    struct FlatRow {
        qint64 nodeId{kInvalidNodeId};
        qint64 parentId{kRootNodeId};
        int depth{0};
        QString name{};
        bool isFolder{false};
        bool isExistingManaged{false};
        bool checked{false};
        bool expanded{true};
        bool hasChildren{false};
        OrganizerEntryStatus status{OrganizerEntryStatus::Ready};
        QString absolutePath{};
        QString statusLabel{};
    };

    void reset();

    void setExistingHashes(QSet<QString> hashes);
    void setManagedRootName(const QString &name);
    void preloadTargetFolders(const QStringList &relativeFolderPaths);

    [[nodiscard]] bool buildSourceFromScan(const QStringList &paths,
                                           const QStringList &allowedExtensions,
                                           const std::atomic_bool *cancelRequested = nullptr);

    [[nodiscard]] qint64 addTargetFolder(qint64 parentFolderId, const QString &folderName);
    [[nodiscard]] qint64 resolveTargetFolderId(qint64 nodeId) const;
    bool mapNodes(const QList<qint64> &sourceNodeIds, qint64 targetFolderId,
                  bool includeFolderWrapper = true);
    bool unmapNodes(const QList<qint64> &targetNodeIds);
    bool setSourceChecked(qint64 nodeId, bool checked);
    bool setExpanded(bool sourceSide, qint64 nodeId, bool expanded);
    void expandAll(bool sourceSide, bool expanded);

    void setFilterMode(OrganizerFilterMode mode);
    void setSearchText(const QString &text);
    [[nodiscard]] OrganizerFilterMode filterMode() const;
    [[nodiscard]] QString searchText() const;

    [[nodiscard]] QList<FlatRow> sourceFlatRows() const;
    [[nodiscard]] QList<FlatRow> targetFlatRows() const;

    [[nodiscard]] QList<OrganizerPlannedEntry> collectPlannedEntries() const;
    [[nodiscard]] int plannedFileCount() const;
    [[nodiscard]] int sourceFileCount() const;
    [[nodiscard]] QString managedRootName() const;

  private:
    [[nodiscard]] qint64 nextId();
    [[nodiscard]] Node *nodeById(qint64 id);
    [[nodiscard]] const Node *nodeById(qint64 id) const;
    [[nodiscard]] QList<qint64> childIds(bool sourceSide, qint64 parentId) const;
    [[nodiscard]] qint64 ensureFolderPath(bool sourceSide, qint64 parentId,
                                          const QStringList &segments);
    [[nodiscard]] qint64 findChildFolder(bool sourceSide, qint64 parentId,
                                         const QString &name) const;
    void removeNodeRecursive(bool sourceSide, qint64 nodeId);
    void cleanupEmptyFolders(bool sourceSide, qint64 parentId);
    [[nodiscard]] qint64 deepCopyToTarget(qint64 sourceNodeId, qint64 targetParentId,
                                          bool includeFolderWrapper,
                                          QList<qint64> *mappedSourceIds = nullptr);
    void restoreSourcePath(const Node &fileNode);
    void collectPlannedRecursive(qint64 parentId, const QString &relativeDir,
                                 QList<OrganizerPlannedEntry> &out) const;
    void buildFlat(bool sourceSide, qint64 parentId, int depth, QList<FlatRow> &out) const;
    [[nodiscard]] bool nodeMatchesFilter(const Node &node) const;
    [[nodiscard]] bool subtreeHasVisibleContent(bool sourceSide, qint64 nodeId) const;
    [[nodiscard]] static QString statusLabelFor(OrganizerEntryStatus status);

    QHash<qint64, Node> m_sourceNodes;
    QHash<qint64, Node> m_targetNodes;
    QList<qint64> m_sourceRootChildren;
    QList<qint64> m_targetRootChildren;
    QSet<QString> m_existingHashes;
    QString m_managedRootName{QStringLiteral("Library")};
    OrganizerFilterMode m_filterMode{OrganizerFilterMode::All};
    QString m_searchText{};
    qint64 m_nextId{1};
};

#endif // IMPORTORGANIZERSESSION_H
