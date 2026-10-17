/**
 * @file CatalogSnapshot.cpp
 * @brief Batch loader for song catalog data used by view models.
 */

#include "CatalogSnapshot.h"

#include "interfaces/IArtistRepository.h"
#include "interfaces/IFileRelationRepository.h"
#include "interfaces/ILinkGroupRepository.h"
#include "interfaces/IMediaFileRepository.h"
#include "interfaces/ISongRepository.h"

CatalogSnapshot CatalogSnapshot::load(const Dependencies &deps) {
    CatalogSnapshot snapshot;

    snapshot.m_songs = deps.songRepo.getAllSongs();

    for (const Artist &artist : deps.artistRepo.getAllArtists()) {
        snapshot.m_artists.insert(artist.id, artist);
    }

    for (const LinkGroup &group : deps.linkGroupRepo.getAllGroups()) {
        snapshot.m_groupByPrimarySong.insert(group.primarySongId, group);
        snapshot.m_groupByPrimaryMedia.insert(group.primaryMediaId, group);
    }

    for (const MediaFile &media : deps.mediaFileRepo.getAllMediaFiles()) {
        snapshot.m_mediaById.insert(media.id, media);
        snapshot.m_mediaBySong[media.songId].append(media);
    }

    for (const auto &[primaryId, secondaryId] : deps.fileRelationRepo.getAllRelationPairs()) {
        snapshot.m_secondaryMediaIds.insert(secondaryId);
        snapshot.m_primaryBySecondary.insert(secondaryId, primaryId);

        const auto mediaIt = snapshot.m_mediaById.constFind(secondaryId);
        if (mediaIt != snapshot.m_mediaById.constEnd()) {
            snapshot.m_linkedByPrimary[primaryId].append(*mediaIt);
        }

        const auto groupIt = snapshot.m_groupByPrimaryMedia.constFind(primaryId);
        if (groupIt != snapshot.m_groupByPrimaryMedia.constEnd()) {
            snapshot.m_groupBySecondaryMedia.insert(secondaryId, *groupIt);
        }
    }

    return snapshot;
}

const QList<Song> &CatalogSnapshot::songs() const { return m_songs; }

std::optional<Artist> CatalogSnapshot::artistById(qlonglong artistId) const {
    const auto it = m_artists.constFind(artistId);
    if (it == m_artists.constEnd()) {
        return std::nullopt;
    }
    return *it;
}

std::optional<LinkGroup> CatalogSnapshot::groupByPrimarySong(qlonglong songId) const {
    const auto it = m_groupByPrimarySong.constFind(songId);
    if (it == m_groupByPrimarySong.constEnd()) {
        return std::nullopt;
    }
    return *it;
}

std::optional<LinkGroup> CatalogSnapshot::groupBySecondaryMedia(qlonglong mediaId) const {
    const auto it = m_groupBySecondaryMedia.constFind(mediaId);
    if (it == m_groupBySecondaryMedia.constEnd()) {
        return std::nullopt;
    }
    return *it;
}

QList<MediaFile> CatalogSnapshot::mediaBySongId(qlonglong songId) const {
    return m_mediaBySong.value(songId);
}

std::optional<MediaFile> CatalogSnapshot::mediaById(qlonglong mediaId) const {
    const auto it = m_mediaById.constFind(mediaId);
    if (it == m_mediaById.constEnd()) {
        return std::nullopt;
    }
    return *it;
}

bool CatalogSnapshot::isSecondaryMedia(qlonglong mediaId) const {
    return m_secondaryMediaIds.contains(mediaId);
}

std::optional<qlonglong> CatalogSnapshot::primaryMediaId(qlonglong mediaId) const {
    const auto it = m_primaryBySecondary.constFind(mediaId);
    if (it == m_primaryBySecondary.constEnd()) {
        return std::nullopt;
    }
    return *it;
}

QList<MediaFile> CatalogSnapshot::linkedMedia(qlonglong primaryMediaId) const {
    return m_linkedByPrimary.value(primaryMediaId);
}

QList<MediaFile> CatalogSnapshot::groupMediaFor(qlonglong songId) const {
    const qlonglong hubSongId = resolveHubSongId(songId);
    const QList<MediaFile> ownFiles = mediaBySongId(hubSongId);
    if (ownFiles.isEmpty()) {
        return {};
    }

    const MediaFile &primaryMedia = ownFiles.first();
    const QList<MediaFile> linked = linkedMedia(primaryMedia.id);
    if (linked.isEmpty() && ownFiles.size() > 1) {
        return ownFiles;
    }

    QList<MediaFile> allFiles;
    allFiles.append(primaryMedia);
    allFiles.append(linked);
    return allFiles;
}

qlonglong CatalogSnapshot::resolveHubSongId(qlonglong songId) const {
    const QList<MediaFile> files = mediaBySongId(songId);
    if (files.isEmpty()) {
        return songId;
    }

    const std::optional<qlonglong> primaryId = primaryMediaId(files.first().id);
    if (!primaryId.has_value()) {
        return songId;
    }

    const std::optional<MediaFile> primaryMedia = mediaById(*primaryId);
    if (!primaryMedia.has_value()) {
        return songId;
    }

    return primaryMedia->songId;
}

bool CatalogSnapshot::isSecondarySong(qlonglong songId) const {
    const QList<MediaFile> files = mediaBySongId(songId);
    if (files.isEmpty()) {
        return false;
    }
    return isSecondaryMedia(files.first().id);
}
