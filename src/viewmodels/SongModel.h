#ifndef SONGMODEL_H
#define SONGMODEL_H

#include <QAbstractListModel>
#include <QTimer>
#include <QVariantList>
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
 * when expandAllGroups is enabled.
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
    /** Empty string = all kinds. Values match MediaKind strings (guitarpro, audio, …). */
    Q_PROPERTY(QString mediaKindFilter READ mediaKindFilter WRITE setMediaKindFilter NOTIFY
                   mediaKindFilterChanged)
    /** 0 = all tempos; otherwise exact match on baseBpm. */
    Q_PROPERTY(int bpmFilter READ bpmFilter WRITE setBpmFilter NOTIFY bpmFilterChanged)
    /** 0 = all tunings; otherwise exact match on tuningId. */
    Q_PROPERTY(int tuningIdFilter READ tuningIdFilter WRITE setTuningIdFilter NOTIFY
                   tuningIdFilterChanged)
    Q_PROPERTY(bool favoritesOnly READ favoritesOnly WRITE setFavoritesOnly NOTIFY
                   favoritesOnlyChanged)
    /** Distinct positive baseBpm values from the catalog, sorted ascending. */
    Q_PROPERTY(QVariantList knownBpms READ knownBpms NOTIFY knownFiltersChanged)
    /** Maps {id, name} for tunings present in the catalog, sorted by name. */
    Q_PROPERTY(QVariantList knownTunings READ knownTunings NOTIFY knownFiltersChanged)

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
        HubSongIdRole,
        MediaIdRole,
        IsFavoriteRole
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
    [[nodiscard]] const QString &mediaKindFilter() const;
    void setMediaKindFilter(const QString &kind);
    [[nodiscard]] int bpmFilter() const;
    void setBpmFilter(int bpm);
    [[nodiscard]] int tuningIdFilter() const;
    void setTuningIdFilter(int tuningId);
    [[nodiscard]] bool favoritesOnly() const;
    void setFavoritesOnly(bool only);
    [[nodiscard]] const QVariantList &knownBpms() const;
    [[nodiscard]] const QVariantList &knownTunings() const;

  public slots:
    void reload();
    /** Applies rows built on a worker thread from @p snapshot. */
    void applySnapshot(const CatalogSnapshot &snapshot);
    void applyViewCache(const CatalogViewCache &cache);
    /** Persists favorite flag and updates in-memory catalog rows. */
    bool setFavorite(qlonglong songId, bool favorite);

  signals:
    void searchTextChanged();
    void totalCountChanged();
    void hideContainersChanged();
    void containersOnlyChanged();
    void expandAllGroupsChanged();
    void catalogReadyChanged();
    void mediaKindFilterChanged();
    void bpmFilterChanged();
    void tuningIdFilterChanged();
    void favoritesOnlyChanged();
    void knownFiltersChanged();

  private:
    using SongRow = SongListRowData;

    [[nodiscard]] CatalogSnapshot::Dependencies repositoryDependencies() const;
    static QString buildSearchHaystack(const SongRow &row);

    void applyPrebuiltRows(QList<SongRow> allRows, QList<SongRow> secondaryRows);
    void rebuildKnownFilters();
    void applyFilter();
    void applyPendingSearchFilter();
    void ensureFilterCurrent();
    [[nodiscard]] bool matchesFilters(const SongRow &row) const;
    [[nodiscard]] bool rowHasMediaKind(const SongRow &row, const QString &kind) const;
    void updateFavoriteInRows(qlonglong songId, bool favorite);
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
    QString m_searchText;
    QString m_mediaKindFilter;
    int m_bpmFilter{0};
    int m_tuningIdFilter{0};
    bool m_favoritesOnly{false};
    bool m_hideContainers{false};
    bool m_containersOnly{false};
    bool m_expandAllGroups{false};
    bool m_catalogReady{false};
    QVariantList m_knownBpms;
    QVariantList m_knownTunings;
    QTimer m_searchDebounce;
};

#endif // SONGMODEL_H
