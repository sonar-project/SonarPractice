/**
 * @file CatalogViewCache.cpp
 * @brief Builds view-model rows from catalog snapshots on a background thread.
 */

#include "CatalogViewCache.h"

#include "Artist.h"
#include "LinkGroup.h"
#include "MediaFile.h"
#include "MediaFileMapping.h"
#include "Song.h"

#include <QDir>
#include <QSet>

namespace {

    QString folderPathFromRelative(const QString &relativePath) {
        const QString normalized = QDir::fromNativeSeparators(relativePath);
        const int lastSlash = static_cast<int>(normalized.lastIndexOf(QLatin1Char('/')));
        if (lastSlash < 0) {
            return QStringLiteral("/");
        }
        return normalized.left(lastSlash);
    }

    bool pathIsUnderPrefix(const QString &folderPath, const QString &prefix) {
        if (prefix.isEmpty()) {
            return true;
        }

        if (folderPath == prefix) {
            return true;
        }

        return folderPath.startsWith(prefix + QLatin1Char('/'));
    }

    QVariantList buildAssetSummary(const QList<MediaFile> &files) {
        QHash<QString, int> counts;

        for (const MediaFile &file : files) {
            const QString kind = MediaFileMapping::mediaKindToString(file.mediaKind);
            counts[kind] = counts.value(kind) + 1;
        }

        QVariantList summary;
        for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
            summary.append(QVariantMap{
                {QStringLiteral("kind"), it.key()},
                {QStringLiteral("count"), it.value()},
            });
        }

        return summary;
    }

    QString buildSongSearchHaystack(const SongListRowData &row) {
        return QStringLiteral("%1 %2 %3 %4 %5")
            .arg(row.title, row.displayTitle, row.linkGroupTitle, row.artistName, row.tuningName)
            .toLower();
    }

    QString buildLibrarySearchHaystack(const LibraryLinkRowData &row) {
        QString haystack = row.title;
        if (!row.artistName.isEmpty()) {
            haystack += QLatin1Char(' ') + row.artistName;
        }
        if (!row.sourceRelativePath.isEmpty()) {
            haystack += QLatin1Char(' ') + row.sourceRelativePath;
        }
        if (!row.mediaKind.isEmpty()) {
            haystack += QLatin1Char(' ') + row.mediaKind;
        }
        if (!row.linkGroupTitle.isEmpty()) {
            haystack += QLatin1Char(' ') + row.linkGroupTitle;
        }
        return haystack.toLower();
    }

    SongListRowData buildSongRow(const Song &song, const CatalogSnapshot &snapshot) {
        SongListRowData row;
        row.id = song.id;
        row.title = song.title;
        row.baseBpm = song.baseBpm;
        row.artistId = song.artistId;
        if (const std::optional<Artist> artist = snapshot.artistById(song.artistId)) {
            row.artistName = artist->name;
        }
        row.tuningId = song.tuningId;
        row.tuningName = song.tuningName;

        const std::optional<LinkGroup> linkGroup = snapshot.groupByPrimarySong(song.id);
        if (!linkGroup.has_value()) {
            const QList<MediaFile> files = snapshot.mediaBySongId(song.id);
            if (!files.isEmpty()) {
                if (const std::optional<LinkGroup> secondaryGroup =
                        snapshot.groupBySecondaryMedia(files.first().id)) {
                    row.linkGroupId = secondaryGroup->id;
                }
            }
        } else {
            row.linkGroupId = linkGroup->id;
        }

        const QList<MediaFile> groupMedia = snapshot.groupMediaFor(song.id);
        row.assetSummary = buildAssetSummary(groupMedia);
        row.linkedMediaCount = groupMedia.size();
        row.hubSongId = snapshot.resolveHubSongId(song.id);

        if (linkGroup.has_value()) {
            row.isLinkedGroup = row.linkedMediaCount > 1;
            row.linkGroupTitle = linkGroup->title;
            row.displayTitle = linkGroup->title;
        } else {
            row.displayTitle = song.title;
        }

        row.searchHaystack = buildSongSearchHaystack(row);
        return row;
    }

    LibraryLinkRowData buildLibraryRow(const Song &song, const CatalogSnapshot &snapshot) {
        const QList<MediaFile> files = snapshot.mediaBySongId(song.id);
        if (files.isEmpty()) {
            return {};
        }

        const MediaFile &media = files.first();
        LibraryLinkRowData row;
        row.songId = song.id;
        row.title = song.title;
        row.mediaId = media.id;
        row.mediaKind = MediaFileMapping::mediaKindToString(media.mediaKind);
        row.importRoot = media.importRoot;
        row.sourceRelativePath = media.sourceRelativePath;
        row.folderPath = folderPathFromRelative(media.sourceRelativePath);

        if (const std::optional<Artist> artist = snapshot.artistById(song.artistId)) {
            row.artistName = artist->name;
        }

        if (const std::optional<LinkGroup> primaryGroup = snapshot.groupByPrimarySong(song.id)) {
            row.isPrimary = true;
            row.isLinked = true;
            row.linkGroupTitle = primaryGroup->title;
        } else if (snapshot.isSecondaryMedia(media.id)) {
            if (const std::optional<LinkGroup> group = snapshot.groupBySecondaryMedia(media.id)) {
                row.isLinked = true;
                row.linkGroupTitle = group->title;
            }
        }

        row.searchHaystack = buildLibrarySearchHaystack(row);
        return row;
    }

    QHash<QString, int> buildFolderCountDirect(const QList<LibraryLinkRowData> &rows) {
        QHash<QString, int> counts;
        for (const LibraryLinkRowData &row : rows) {
            ++counts[row.folderPath];
        }
        return counts;
    }

    QHash<QString, int> buildFolderCountRecursive(const QList<LibraryLinkRowData> &rows) {
        QSet<QString> folderPaths;
        for (const LibraryLinkRowData &row : rows) {
            if (!row.folderPath.isEmpty()) {
                folderPaths.insert(row.folderPath);
            }
        }

        QHash<QString, int> counts;
        for (const QString &folderPath : folderPaths) {
            int count = 0;
            for (const LibraryLinkRowData &row : rows) {
                if (pathIsUnderPrefix(row.folderPath, folderPath)) {
                    ++count;
                }
            }
            counts.insert(folderPath, count);
        }

        counts.insert(QString(), static_cast<int>(rows.size()));
        return counts;
    }

} // namespace

CatalogViewCache::CatalogViewCache(std::shared_ptr<const CatalogSnapshot> snapshot,
                                   QList<SongListRowData> songAllRows,
                                   QList<SongListRowData> songSecondaryRows,
                                   QList<LibraryLinkRowData> libraryRows,
                                   QHash<QString, int> folderCountDirect,
                                   QHash<QString, int> folderCountRecursive)
    : m_snapshot(std::move(snapshot)), m_songAllRows(std::move(songAllRows)),
      m_songSecondaryRows(std::move(songSecondaryRows)), m_libraryRows(std::move(libraryRows)),
      m_folderCountDirect(std::move(folderCountDirect)),
      m_folderCountRecursive(std::move(folderCountRecursive)) {}

CatalogViewCache CatalogViewCache::load(const CatalogSnapshot::Dependencies &deps) {
    return fromSnapshot(CatalogSnapshot::load(deps));
}

CatalogViewCache CatalogViewCache::fromSnapshot(CatalogSnapshot snapshot) {
    auto sharedSnapshot = std::make_shared<CatalogSnapshot>(std::move(snapshot));
    const CatalogSnapshot &view = *sharedSnapshot;

    QList<SongListRowData> songAllRows;
    QList<SongListRowData> songSecondaryRows;
    const QList<Song> songs = view.songs();
    songAllRows.reserve(songs.size());

    for (const Song &song : songs) {
        SongListRowData row = buildSongRow(song, view);

        if (view.isSecondarySong(song.id)) {
            row.isContainerMember = true;
            row.displayTitle = song.title;
            row.isLinkedGroup = false;
            if (row.linkGroupId <= 0) {
                const QList<MediaFile> files = view.mediaBySongId(song.id);
                if (!files.isEmpty()) {
                    if (const std::optional<LinkGroup> group =
                            view.groupBySecondaryMedia(files.first().id)) {
                        row.linkGroupId = group->id;
                        row.linkGroupTitle = group->title;
                    }
                }
            }
            row.searchHaystack = buildSongSearchHaystack(row);
            songSecondaryRows.append(row);
            continue;
        }

        songAllRows.append(row);
    }

    QList<LibraryLinkRowData> libraryRows;
    libraryRows.reserve(songs.size());
    for (const Song &song : songs) {
        LibraryLinkRowData row = buildLibraryRow(song, view);
        if (row.songId <= 0) {
            continue;
        }
        libraryRows.append(row);
    }

    return CatalogViewCache(std::move(sharedSnapshot), std::move(songAllRows),
                            std::move(songSecondaryRows), std::move(libraryRows),
                            buildFolderCountDirect(libraryRows),
                            buildFolderCountRecursive(libraryRows));
}

const CatalogSnapshot &CatalogViewCache::snapshot() const { return *m_snapshot; }

std::shared_ptr<const CatalogSnapshot> CatalogViewCache::sharedSnapshot() const {
    return m_snapshot;
}

const QList<SongListRowData> &CatalogViewCache::songAllRows() const { return m_songAllRows; }

const QList<SongListRowData> &CatalogViewCache::songSecondaryRows() const {
    return m_songSecondaryRows;
}

const QList<LibraryLinkRowData> &CatalogViewCache::libraryRows() const { return m_libraryRows; }

int CatalogViewCache::folderCountFor(const QString &folderPath, bool includeSubfolders) const {
    const QHash<QString, int> &counts =
        includeSubfolders ? m_folderCountRecursive : m_folderCountDirect;

    if (folderPath.isEmpty()) {
        return counts.value(QString(), static_cast<int>(m_libraryRows.size()));
    }

    return counts.value(folderPath, 0);
}

const QHash<QString, int> &CatalogViewCache::folderCountDirect() const {
    return m_folderCountDirect;
}

const QHash<QString, int> &CatalogViewCache::folderCountRecursive() const {
    return m_folderCountRecursive;
}
