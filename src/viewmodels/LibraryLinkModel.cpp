#include "LibraryLinkModel.h"

#include "CatalogViewCache.h"
#include "LibrarySearchExpression.h"
#include "LinkGroup.h"
#include "interfaces/IArtistRepository.h"
#include "interfaces/IFileRelationRepository.h"
#include "interfaces/ILinkGroupRepository.h"
#include "interfaces/IMediaFileRepository.h"
#include "interfaces/ISongRepository.h"

#include <QSet>

#include <algorithm>

namespace {

    constexpr int kUnknownLinkPriority = 99;

    bool pathIsUnderPrefix(const QString &folderPath, const QString &prefix) {
        if (prefix.isEmpty()) {
            return true;
        }

        if (folderPath == prefix) {
            return true;
        }

        return folderPath.startsWith(prefix + QLatin1Char('/'));
    }

} // namespace

LibraryLinkModel::LibraryLinkModel(ISongRepository &songRepo, IMediaFileRepository &mediaFileRepo,
                                   ILinkGroupRepository &linkGroupRepo,
                                   IFileRelationRepository &fileRelationRepo,
                                   IArtistRepository &artistRepo, QObject *parent)
    : QAbstractListModel(parent), m_songRepo(songRepo), m_mediaFileRepo(mediaFileRepo),
      m_linkGroupRepo(linkGroupRepo), m_fileRelationRepo(fileRelationRepo),
      m_artistRepo(artistRepo) {
    m_searchDebounce.setSingleShot(true);
    m_searchDebounce.setInterval(200);
    connect(&m_searchDebounce, &QTimer::timeout, this, &LibraryLinkModel::applyPendingSearchFilter);
}

int LibraryLinkModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_rows.size());
}

QVariant LibraryLinkModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }

    const LibraryRow &row = m_rows.at(index.row());

    switch (role) {
    case SongIdRole:
        return row.songId;
    case TitleRole:
        if (m_containersOnly && row.isPrimary && !row.linkGroupTitle.isEmpty()) {
            return row.linkGroupTitle;
        }
        return row.title;
    case MediaIdRole:
        return row.mediaId;
    case MediaKindRole:
        return row.mediaKind;
    case ImportRootRole:
        return row.importRoot;
    case SourceRelativePathRole:
        return row.sourceRelativePath;
    case FolderPathRole:
        return row.folderPath;
    case IsPrimaryRole:
        return row.isPrimary;
    case IsLinkedRole:
        return row.isLinked;
    case LinkGroupTitleRole:
        return row.linkGroupTitle;
    case ArtistNameRole:
        return row.artistName;
    default:
        return {};
    }
}

QHash<int, QByteArray> LibraryLinkModel::roleNames() const {
    return {
        {SongIdRole, "songId"},         {TitleRole, "title"},
        {MediaIdRole, "mediaId"},       {MediaKindRole, "mediaKind"},
        {ImportRootRole, "importRoot"}, {SourceRelativePathRole, "sourceRelativePath"},
        {FolderPathRole, "folderPath"}, {IsPrimaryRole, "isPrimary"},
        {IsLinkedRole, "isLinked"},     {LinkGroupTitleRole, "linkGroupTitle"},
        {ArtistNameRole, "artistName"},
    };
}

const QString &LibraryLinkModel::searchText() const { return m_searchText; }

void LibraryLinkModel::setSearchText(const QString &text) {
    if (m_searchText == text) {
        return;
    }

    m_searchText = text;
    emit searchTextChanged();
    m_searchDebounce.start();
}

const QString &LibraryLinkModel::folderFilter() const { return m_folderFilter; }

void LibraryLinkModel::setFolderFilter(const QString &path) {
    if (m_folderFilter == path) {
        return;
    }

    m_folderFilter = path;
    applyFilter();
    emit folderFilterChanged();
}

bool LibraryLinkModel::hideContainers() const { return m_hideContainers; }

void LibraryLinkModel::setHideContainers(bool hide) {
    if (m_hideContainers == hide) {
        return;
    }

    m_hideContainers = hide;
    if (hide) {
        m_containersOnly = false;
        emit containersOnlyChanged();
    }
    applyFilter();
    emit hideContainersChanged();
}

bool LibraryLinkModel::containersOnly() const { return m_containersOnly; }

void LibraryLinkModel::setContainersOnly(bool only) {
    if (m_containersOnly == only) {
        return;
    }

    m_containersOnly = only;
    if (only) {
        m_hideContainers = false;
        emit hideContainersChanged();
    }
    applyFilter();
    emit containersOnlyChanged();
}

CatalogSnapshot::Dependencies LibraryLinkModel::repositoryDependencies() const {
    return CatalogSnapshot::Dependencies{
        .songRepo = m_songRepo,
        .mediaFileRepo = m_mediaFileRepo,
        .artistRepo = m_artistRepo,
        .linkGroupRepo = m_linkGroupRepo,
        .fileRelationRepo = m_fileRelationRepo,
    };
}

void LibraryLinkModel::reload() {
    applyViewCache(CatalogViewCache::load(repositoryDependencies()));
}

void LibraryLinkModel::applySnapshot(const CatalogSnapshot &snapshot) {
    applyViewCache(CatalogViewCache::fromSnapshot(snapshot));
}

void LibraryLinkModel::applyViewCache(const CatalogViewCache &cache) {
    applyPrebuiltRows(cache.libraryRows(), cache.folderCountDirect(), cache.folderCountRecursive());

    const bool wasLoaded = m_loaded;
    m_loaded = true;
    if (!wasLoaded) {
        emit loadedChanged();
    }
}

void LibraryLinkModel::applyPrebuiltRows(QList<LibraryRow> allRows,
                                         const QHash<QString, int> &folderCountDirect,
                                         const QHash<QString, int> &folderCountRecursive) {
    m_allRows = std::move(allRows);
    m_folderCountDirect = folderCountDirect;
    m_folderCountRecursive = folderCountRecursive;
    rebuildSongIndex();
    applyFilter();
}

bool LibraryLinkModel::loaded() const { return m_loaded; }

void LibraryLinkModel::applyPendingSearchFilter() { applyFilter(); }

void LibraryLinkModel::ensureFilterCurrent() {
    if (m_searchDebounce.isActive()) {
        m_searchDebounce.stop();
        applyFilter();
    }
}

void LibraryLinkModel::rebuildSongIndex() {
    m_songIndex.clear();
    m_songIndex.reserve(m_allRows.size());

    for (int index = 0; index < m_allRows.size(); ++index) {
        m_songIndex.insert(m_allRows.at(index).songId, index);
    }
}

void LibraryLinkModel::applyFilter() {
    beginResetModel();
    m_rows.clear();

    m_rows.reserve(m_allRows.size());
    for (const LibraryRow &row : m_allRows) {
        if (matchesFilter(row)) {
            m_rows.append(row);
        }
    }

    endResetModel();
    emit visibleUnlinkedCountChanged();
}

bool LibraryLinkModel::folderPathMatchesFilter(const QString &folderPath) const {
    return pathIsUnderPrefix(folderPath, m_folderFilter);
}

bool LibraryLinkModel::matchesFilter(const LibraryRow &row) const {
    if (!folderPathMatchesFilter(row.folderPath)) {
        return false;
    }

    if (m_containersOnly) {
        if (!row.isLinked || !row.isPrimary) {
            return false;
        }
    } else if (m_hideContainers && row.isLinked) {
        return false;
    }

    return LibrarySearchExpression::matches(m_searchText, row.searchHaystack);
}

QString LibraryLinkModel::buildSearchHaystack(const LibraryRow &row) {
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

const LibraryLinkModel::LibraryRow *LibraryLinkModel::rowForSong(qlonglong songId) const {
    const auto index = m_songIndex.constFind(songId);
    if (index == m_songIndex.cend()) {
        return nullptr;
    }
    return &m_allRows.at(*index);
}

LibraryLinkModel::LibraryRow *LibraryLinkModel::mutableRowForSong(qlonglong songId) {
    const auto index = m_songIndex.find(songId);
    if (index == m_songIndex.end()) {
        return nullptr;
    }
    return &m_allRows[*index];
}

int LibraryLinkModel::linkPriorityForKind(const QString &mediaKind) const {
    if (mediaKind == QStringLiteral("guitarpro")) {
        return 0;
    }
    if (mediaKind == QStringLiteral("document")) {
        return 1;
    }
    if (mediaKind == QStringLiteral("video")) {
        return 2;
    }
    if (mediaKind == QStringLiteral("audio")) {
        return 3;
    }
    return 4;
}

QVariantList LibraryLinkModel::visibleUnlinkedSongIds() {
    ensureFilterCurrent();

    QVariantList songIds;
    songIds.reserve(m_rows.size());

    for (const LibraryRow &row : m_rows) {
        if (!row.isLinked) {
            songIds.append(row.songId);
        }
    }

    return songIds;
}

QVariantList LibraryLinkModel::orderSongIdsForLinking(const QVariantList &songIds) {
    ensureFilterCurrent();

    struct SortEntry {
        qlonglong songId{0};
        int priority{kUnknownLinkPriority};
        int originalIndex{0};
    };

    QList<SortEntry> entries;
    entries.reserve(songIds.size());

    for (int index = 0; index < songIds.size(); ++index) {
        const qlonglong songId = songIds.at(index).toLongLong();
        SortEntry entry;
        entry.songId = songId;
        entry.originalIndex = index;

        if (const LibraryRow *row = rowForSong(songId)) {
            entry.priority = linkPriorityForKind(row->mediaKind);
        }

        entries.append(entry);
    }

    std::stable_sort(entries.begin(), entries.end(), [](const SortEntry &a, const SortEntry &b) {
        if (a.priority != b.priority) {
            return a.priority < b.priority;
        }
        return a.originalIndex < b.originalIndex;
    });

    QVariantList ordered;
    ordered.reserve(entries.size());
    for (const SortEntry &entry : entries) {
        ordered.append(entry.songId);
    }

    return ordered;
}

QString LibraryLinkModel::defaultLinkTitle() const {
    QString trimmed = m_searchText.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    if (trimmed.contains(QStringLiteral("&&")) || trimmed.contains(QStringLiteral("||"))) {
        return {};
    }

    return trimmed;
}

QString LibraryLinkModel::titleForSong(qlonglong songId) const {
    if (const LibraryRow *row = rowForSong(songId)) {
        return row->title;
    }
    return {};
}

bool LibraryLinkModel::isSongLinked(qlonglong songId) const {
    if (const LibraryRow *row = rowForSong(songId)) {
        return row->isLinked;
    }
    return false;
}

void LibraryLinkModel::updateSongsLinkState(qlonglong groupId, const QVariantList &songIds) {
    const std::optional<LinkGroup> group = m_linkGroupRepo.getGroup(groupId);
    if (!group.has_value()) {
        reload();
        return;
    }

    bool anyChanged = false;

    for (const QVariant &value : songIds) {
        const qlonglong songId = value.toLongLong();
        if (songId <= 0) {
            continue;
        }

        LibraryRow *row = mutableRowForSong(songId);
        if (row == nullptr) {
            continue;
        }

        row->isLinked = true;
        row->linkGroupTitle = group->title;
        row->isPrimary = (songId == group->primarySongId);
        row->searchHaystack = buildSearchHaystack(*row);
        anyChanged = true;
    }

    if (anyChanged) {
        applyFilter();
    }
}

void LibraryLinkModel::clearSongsLinkState(const QVariantList &songIds) {
    bool anyChanged = false;

    for (const QVariant &value : songIds) {
        const qlonglong songId = value.toLongLong();
        if (songId <= 0) {
            continue;
        }

        LibraryRow *row = mutableRowForSong(songId);
        if (row == nullptr) {
            continue;
        }

        row->isLinked = false;
        row->isPrimary = false;
        row->linkGroupTitle.clear();
        row->searchHaystack = buildSearchHaystack(*row);
        anyChanged = true;
    }

    if (anyChanged) {
        applyFilter();
    }
}

QStringList LibraryLinkModel::distinctFolderPaths() const {
    QSet<QString> folders;
    for (const LibraryRow &row : m_allRows) {
        if (!row.folderPath.isEmpty()) {
            folders.insert(row.folderPath);
        }
    }

    QStringList paths = folders.values();
    paths.sort();
    return paths;
}

int LibraryLinkModel::fileCountForFolder(const QString &folderPath, bool includeSubfolders) const {
    return cachedFolderCount(folderPath, includeSubfolders);
}

int LibraryLinkModel::cachedFolderCount(const QString &folderPath, bool includeSubfolders) const {
    const QHash<QString, int> &counts =
        includeSubfolders ? m_folderCountRecursive : m_folderCountDirect;

    if (folderPath.isEmpty()) {
        return counts.value(QString(), static_cast<int>(m_allRows.size()));
    }

    return counts.value(folderPath, 0);
}

int LibraryLinkModel::visibleUnlinkedCount() {
    ensureFilterCurrent();

    int count = 0;
    for (const LibraryRow &row : m_rows) {
        if (!row.isLinked) {
            ++count;
        }
    }
    return count;
}
