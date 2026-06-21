#ifndef LIBRARYLINKMODEL_H
#define LIBRARYLINKMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include "CatalogSnapshot.h"
#include "CatalogViewCache.h"

class ISongRepository;
class IMediaFileRepository;
class ILinkGroupRepository;
class IFileRelationRepository;
class IArtistRepository;

class LibraryLinkModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Instances are provided via the 'libraryLinkModel' context property.")

    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(
        QString folderFilter READ folderFilter WRITE setFolderFilter NOTIFY folderFilterChanged)
    Q_PROPERTY(bool hideContainers READ hideContainers WRITE setHideContainers NOTIFY
                   hideContainersChanged)
    Q_PROPERTY(bool containersOnly READ containersOnly WRITE setContainersOnly NOTIFY
                   containersOnlyChanged)
    Q_PROPERTY(
        int visibleUnlinkedCount READ visibleUnlinkedCount NOTIFY visibleUnlinkedCountChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)

  public:
    enum Roles : qint16 {
        SongIdRole = Qt::UserRole + 1,
        TitleRole,
        MediaIdRole,
        MediaKindRole,
        ImportRootRole,
        SourceRelativePathRole,
        FolderPathRole,
        IsPrimaryRole,
        IsLinkedRole,
        LinkGroupTitleRole,
        ArtistNameRole
    };
    Q_ENUM(Roles)

    explicit LibraryLinkModel(ISongRepository &songRepo, IMediaFileRepository &mediaFileRepo,
                              ILinkGroupRepository &linkGroupRepo,
                              IFileRelationRepository &fileRelationRepo,
                              IArtistRepository &artistRepo, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] const QString &searchText() const;
    void setSearchText(const QString &text);
    [[nodiscard]] const QString &folderFilter() const;
    void setFolderFilter(const QString &path);
    [[nodiscard]] bool hideContainers() const;
    void setHideContainers(bool hide);
    [[nodiscard]] bool containersOnly() const;
    void setContainersOnly(bool only);
    [[nodiscard]] int visibleUnlinkedCount();
    [[nodiscard]] bool loaded() const;

    /// Returns the trimmed search text when it is a single plain term (no && or ||).
    Q_INVOKABLE QString defaultLinkTitle() const;
    Q_INVOKABLE QString titleForSong(qlonglong songId) const;
    Q_INVOKABLE bool isSongLinked(qlonglong songId) const;
    Q_INVOKABLE QStringList distinctFolderPaths() const;
    Q_INVOKABLE int fileCountForFolder(const QString &folderPath,
                                       bool includeSubfolders = true) const;
    Q_INVOKABLE QVariantList visibleUnlinkedSongIds();
    Q_INVOKABLE QVariantList orderSongIdsForLinking(const QVariantList &songIds);
    /// Updates link flags for the given songs without reloading the whole library.
    Q_INVOKABLE void updateSongsLinkState(qlonglong groupId, const QVariantList &songIds);
    /// Clears link flags for the given songs without reloading the whole library.
    Q_INVOKABLE void clearSongsLinkState(const QVariantList &songIds);

  public slots:
    void reload();
    /**
     * @brief Fills rows from a pre-loaded catalog snapshot.
     * @param snapshot Batch-loaded catalog data shared with the song list.
     */
    void applySnapshot(const CatalogSnapshot &snapshot);
    void applyViewCache(const CatalogViewCache &cache);

  signals:
    void searchTextChanged();
    void folderFilterChanged();
    void hideContainersChanged();
    void containersOnlyChanged();
    void visibleUnlinkedCountChanged();
    void loadedChanged();

  private:
    using LibraryRow = LibraryLinkRowData;

    void applyFilter();
    void applyPendingSearchFilter();
    void ensureFilterCurrent();
    void rebuildSongIndex();
    [[nodiscard]] CatalogSnapshot::Dependencies repositoryDependencies() const;
    void applyPrebuiltRows(QList<LibraryRow> allRows, const QHash<QString, int> &folderCountDirect,
                           const QHash<QString, int> &folderCountRecursive);
    [[nodiscard]] bool matchesFilter(const LibraryRow &row) const;
    [[nodiscard]] bool folderPathMatchesFilter(const QString &folderPath) const;
    [[nodiscard]] static QString buildSearchHaystack(const LibraryRow &row);
    [[nodiscard]] const LibraryRow *rowForSong(qlonglong songId) const;
    [[nodiscard]] LibraryRow *mutableRowForSong(qlonglong songId);
    [[nodiscard]] int linkPriorityForKind(const QString &mediaKind) const;
    [[nodiscard]] int cachedFolderCount(const QString &folderPath, bool includeSubfolders) const;

    ISongRepository &m_songRepo;
    IMediaFileRepository &m_mediaFileRepo;
    ILinkGroupRepository &m_linkGroupRepo;
    IFileRelationRepository &m_fileRelationRepo;
    IArtistRepository &m_artistRepo;
    QList<LibraryRow> m_allRows{};
    QList<LibraryRow> m_rows{};
    QHash<qlonglong, int> m_songIndex{};
    QHash<QString, int> m_folderCountDirect{};
    QHash<QString, int> m_folderCountRecursive{};
    QString m_searchText{};
    QString m_folderFilter{};
    bool m_hideContainers{false};
    bool m_containersOnly{false};
    bool m_loaded{false};
    QTimer m_searchDebounce{};
};

#endif // LIBRARYLINKMODEL_H
