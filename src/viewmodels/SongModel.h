#ifndef SONGMODEL_H
#define SONGMODEL_H

#include <QAbstractListModel>
#include <QSet>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <memory>
#include <qtmetamacros.h>

#include "CatalogSnapshot.h"
#include "CatalogViewCache.h"

class ISongRepository;
class IMediaFileRepository;
class IArtistRepository;
class ILinkGroupRepository;
class IFileRelationRepository;

/**
 * @brief Song list for QML with search and link-group display modes.
 *
 * Primary rows live in m_allRows; linked member songs in m_secondaryRows and appear
 * when their group is expanded or expandAllGroups is enabled.
 */
class SongModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Instances are provided via the 'songModel' context property.")

    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged)
    /** Hides link-group hub rows and shows member songs as top-level entries. */
    Q_PROPERTY(bool hideContainers READ hideContainers WRITE setHideContainers NOTIFY
                   hideContainersChanged)
    /** Shows only link-group hub rows (mutually exclusive with hideContainers). */
    Q_PROPERTY(bool containersOnly READ containersOnly WRITE setContainersOnly NOTIFY
                   containersOnlyChanged)
    /** Reveals all link-group member songs without per-group expand state. */
    Q_PROPERTY(bool expandAllGroups READ expandAllGroups WRITE setExpandAllGroups NOTIFY
                   expandAllGroupsChanged)
    /** True after the first catalog snapshot or view cache was applied. */
    Q_PROPERTY(bool catalogReady READ catalogReady NOTIFY catalogReadyChanged)

  public:
    enum Roles : uint16_t {
        SongIdRole = Qt::UserRole + 1,
        TitleRole,
        BaseBpmRole,
        ArtistIdRole,
        ArtistNameRole,
        TuningIdRole,
        TuningNameRole,
        AssetSummaryRole,
        IsLinkedGroupRole,
        LinkGroupTitleRole,
        LinkedMediaCountRole,
        DisplayTitleRole,
        LinkGroupIdRole,
        IsContainerMemberRole,
        HubSongIdRole
    };
    Q_ENUM(Roles)

    explicit SongModel(ISongRepository &songRepo, IMediaFileRepository &mediaFileRepo,
                       IArtistRepository &artistRepo, ILinkGroupRepository &linkGroupRepo,
                       IFileRelationRepository &fileRelationRepo, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] const QString &searchText() const;
    void setSearchText(const QString &text);
    [[nodiscard]] int totalCount() const;
    [[nodiscard]] bool hideContainers() const;
    void setHideContainers(bool hide);
    [[nodiscard]] bool containersOnly() const;
    void setContainersOnly(bool only);
    [[nodiscard]] bool expandAllGroups() const;
    void setExpandAllGroups(bool expand);
    [[nodiscard]] bool catalogReady() const;

  public slots:
    void reload();
    /** Applies rows built on a worker thread from @p snapshot. */
    void applySnapshot(const CatalogSnapshot &snapshot);
    void applyViewCache(const CatalogViewCache &cache);
    /** Toggles member-song visibility for one link group (ignored while expandAllGroups is on). */
    Q_INVOKABLE void setGroupExpanded(qlonglong groupId, bool expanded);
    Q_INVOKABLE bool isGroupExpanded(qlonglong groupId) const;

  signals:
    void searchTextChanged();
    void totalCountChanged();
    void hideContainersChanged();
    void containersOnlyChanged();
    void expandAllGroupsChanged();
    void expandedGroupsChanged();
    void catalogReadyChanged();

  private:
    using SongRow = SongListRowData;

    [[nodiscard]] CatalogSnapshot::Dependencies repositoryDependencies() const;
    static QString buildSearchHaystack(const SongRow &row);

    void applyPrebuiltRows(QList<SongRow> allRows, QList<SongRow> secondaryRows);
    void applyFilter();
    void applyPendingSearchFilter();
    void ensureFilterCurrent();
    [[nodiscard]] bool matchesSearch(const SongRow &row) const;
    /** Link-group header row (not a member song inside a group). */
    [[nodiscard]] static bool isContainerHub(const SongRow &row);

    ISongRepository &m_songRepo;
    IMediaFileRepository &m_mediaFileRepo;
    IArtistRepository &m_artistRepo;
    ILinkGroupRepository &m_linkGroupRepo;
    IFileRelationRepository &m_fileRelationRepo;
    std::shared_ptr<const CatalogSnapshot> m_snapshot;
    QList<SongRow> m_allRows;
    QList<SongRow> m_secondaryRows;
    QList<SongRow> m_rows;
    QSet<qlonglong> m_expandedGroupIds;
    QString m_searchText;
    bool m_hideContainers{false};
    bool m_containersOnly{false};
    bool m_expandAllGroups{false};
    bool m_catalogReady{false};
    QTimer m_searchDebounce;
};

#endif // SONGMODEL_H
