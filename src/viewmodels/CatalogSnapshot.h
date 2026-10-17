#ifndef CATALOGSNAPSHOT_H
#define CATALOGSNAPSHOT_H

#include "Artist.h"
#include "LinkGroup.h"
#include "MediaFile.h"
#include "Song.h"

#include <QHash>
#include <QList>
#include <QSet>

class IArtistRepository;
class IFileRelationRepository;
class ILinkGroupRepository;
class IMediaFileRepository;
class ISongRepository;

/**
 * @brief Holds all catalog data loaded in one batch from the database.
 *
 * View models read from this snapshot instead of running a query
 * for each song. Built on a worker thread, applied on the main thread.
 */
class CatalogSnapshot {
  public:
    struct Dependencies {
        ISongRepository &songRepo;
        IMediaFileRepository &mediaFileRepo;
        IArtistRepository &artistRepo;
        ILinkGroupRepository &linkGroupRepo;
        IFileRelationRepository &fileRelationRepo;
    };

    /**
     * @brief Loads the full catalog in a small number of database queries.
     * @param deps Repository references that share one open connection.
     * @return Snapshot with lookup maps for songs, media, and link groups.
     */
    [[nodiscard]] static CatalogSnapshot load(const Dependencies &deps);

    [[nodiscard]] const QList<Song> &songs() const;
    [[nodiscard]] std::optional<Artist> artistById(qlonglong artistId) const;
    [[nodiscard]] std::optional<LinkGroup> groupByPrimarySong(qlonglong songId) const;
    [[nodiscard]] std::optional<LinkGroup> groupBySecondaryMedia(qlonglong mediaId) const;
    [[nodiscard]] QList<MediaFile> mediaBySongId(qlonglong songId) const;
    [[nodiscard]] std::optional<MediaFile> mediaById(qlonglong mediaId) const;
    [[nodiscard]] bool isSecondaryMedia(qlonglong mediaId) const;
    [[nodiscard]] std::optional<qlonglong> primaryMediaId(qlonglong mediaId) const;
    [[nodiscard]] QList<MediaFile> linkedMedia(qlonglong primaryMediaId) const;
    [[nodiscard]] QList<MediaFile> groupMediaFor(qlonglong songId) const;
    [[nodiscard]] qlonglong resolveHubSongId(qlonglong songId) const;
    [[nodiscard]] bool isSecondarySong(qlonglong songId) const;

  private:
    QList<Song> m_songs{};
    QHash<qlonglong, Artist> m_artists{};
    QHash<qlonglong, LinkGroup> m_groupByPrimarySong{};
    QHash<qlonglong, LinkGroup> m_groupByPrimaryMedia{};
    QHash<qlonglong, LinkGroup> m_groupBySecondaryMedia{};
    QHash<qlonglong, QList<MediaFile>> m_mediaBySong{};
    QHash<qlonglong, MediaFile> m_mediaById{};
    QSet<qlonglong> m_secondaryMediaIds{};
    QHash<qlonglong, qlonglong> m_primaryBySecondary{};
    QHash<qlonglong, QList<MediaFile>> m_linkedByPrimary{};
};

#endif // CATALOGSNAPSHOT_H
