/**
 * @file SongModel.cpp
 * @brief Filters and exposes catalog song rows to QML.
 */

#include "SongModel.h"

#include "CatalogViewCache.h"
#include "interfaces/IArtistRepository.h"
#include "interfaces/IFileRelationRepository.h"
#include "interfaces/ILinkGroupRepository.h"
#include "interfaces/IMediaFileRepository.h"
#include "interfaces/ISongRepository.h"

namespace {

    constexpr int kSearchDebounceMs = 200;

} // namespace

SongModel::SongModel(ISongRepository &songRepo, IMediaFileRepository &mediaFileRepo,
                     IArtistRepository &artistRepo, ILinkGroupRepository &linkGroupRepo,
                     IFileRelationRepository &fileRelationRepo, QObject *parent)
    : QAbstractListModel(parent), m_songRepo(songRepo), m_mediaFileRepo(mediaFileRepo),
      m_artistRepo(artistRepo), m_linkGroupRepo(linkGroupRepo),
      m_fileRelationRepo(fileRelationRepo) {
    m_searchDebounce.setSingleShot(true);
    m_searchDebounce.setInterval(kSearchDebounceMs);
    connect(&m_searchDebounce, &QTimer::timeout, this, &SongModel::applyPendingSearchFilter);
}

int SongModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_rows.size());
}

QVariant SongModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }

    const SongRow &row = m_rows.at(index.row());

    switch (role) {
    case SongIdRole:
        return row.id;
    case TitleRole:
        return row.title;
    case BaseBpmRole:
        return row.baseBpm;
    case ArtistIdRole:
        return row.artistId;
    case ArtistNameRole:
        return row.artistName;
    case TuningIdRole:
        return row.tuningId;
    case TuningNameRole:
        return row.tuningName.isEmpty() ? tr("Unknown") : row.tuningName;
    case AssetSummaryRole:
        return row.assetSummary;
    case IsLinkedGroupRole:
        return row.isLinkedGroup;
    case LinkGroupTitleRole:
        return row.linkGroupTitle;
    case LinkedMediaCountRole:
        return row.linkedMediaCount;
    case DisplayTitleRole:
        return row.displayTitle;
    case LinkGroupIdRole:
        return row.linkGroupId;
    case IsContainerMemberRole:
        return row.isContainerMember;
    case HubSongIdRole:
        return row.hubSongId > 0 ? row.hubSongId : row.id;
    default:
        return {};
    }
}

QHash<int, QByteArray> SongModel::roleNames() const {
    return {
        {SongIdRole, "songId"},
        {TitleRole, "title"},
        {BaseBpmRole, "baseBpm"},
        {ArtistIdRole, "artistId"},
        {ArtistNameRole, "artistName"},
        {TuningIdRole, "tuningId"},
        {TuningNameRole, "tuningName"},
        {AssetSummaryRole, "assetSummary"},
        {IsLinkedGroupRole, "isLinkedGroup"},
        {LinkGroupTitleRole, "linkGroupTitle"},
        {LinkedMediaCountRole, "linkedMediaCount"},
        {DisplayTitleRole, "displayTitle"},
        {LinkGroupIdRole, "linkGroupId"},
        {IsContainerMemberRole, "isContainerMember"},
        {HubSongIdRole, "hubSongId"},
    };
}

const QString &SongModel::searchText() const { return m_searchText; }

void SongModel::setSearchText(const QString &text) {
    if (m_searchText == text) {
        return;
    }

    m_searchText = text;
    m_searchDebounce.start();
    emit searchTextChanged();
}

int SongModel::totalCount() const { return m_allRows.size() + m_secondaryRows.size(); }

bool SongModel::hideContainers() const { return m_hideContainers; }

void SongModel::setHideContainers(bool hide) {
    if (m_hideContainers == hide) {
        return;
    }

    m_hideContainers = hide;
    if (hide) {
        m_containersOnly = false;
        emit containersOnlyChanged();
        if (m_expandAllGroups) {
            m_expandAllGroups = false;
            emit expandAllGroupsChanged();
        }
    }
    ensureFilterCurrent();
    applyFilter();
    emit hideContainersChanged();
}

bool SongModel::containersOnly() const { return m_containersOnly; }

void SongModel::setContainersOnly(bool only) {
    if (m_containersOnly == only) {
        return;
    }

    m_containersOnly = only;
    if (only) {
        m_hideContainers = false;
        emit hideContainersChanged();
        if (m_expandAllGroups) {
            m_expandAllGroups = false;
            emit expandAllGroupsChanged();
        }
    }
    ensureFilterCurrent();
    applyFilter();
    emit containersOnlyChanged();
}

bool SongModel::expandAllGroups() const { return m_expandAllGroups; }

void SongModel::setExpandAllGroups(bool expand) {
    if (m_expandAllGroups == expand) {
        return;
    }

    m_expandAllGroups = expand;
    if (expand) {
        m_expandedGroupIds.clear();
    }
    ensureFilterCurrent();
    applyFilter();
    emit expandAllGroupsChanged();
    emit expandedGroupsChanged();
}

bool SongModel::catalogReady() const { return m_catalogReady; }

void SongModel::setGroupExpanded(qlonglong groupId, bool expanded) {
    if (groupId <= 0) {
        return;
    }

    if (m_expandAllGroups && !expanded) {
        m_expandAllGroups = false;
        emit expandAllGroupsChanged();
    }

    const bool wasExpanded = m_expandedGroupIds.contains(groupId);
    if (expanded) {
        m_expandedGroupIds.insert(groupId);
    } else {
        m_expandedGroupIds.remove(groupId);
    }

    if (wasExpanded != expanded) {
        ensureFilterCurrent();
        applyFilter();
        emit expandedGroupsChanged();
    }
}

bool SongModel::isGroupExpanded(qlonglong groupId) const {
    return groupId > 0 && (m_expandAllGroups || m_expandedGroupIds.contains(groupId));
}

CatalogSnapshot::Dependencies SongModel::repositoryDependencies() const {
    return CatalogSnapshot::Dependencies{
        .songRepo = m_songRepo,
        .mediaFileRepo = m_mediaFileRepo,
        .artistRepo = m_artistRepo,
        .linkGroupRepo = m_linkGroupRepo,
        .fileRelationRepo = m_fileRelationRepo,
    };
}

void SongModel::reload() { applyViewCache(CatalogViewCache::load(repositoryDependencies())); }

void SongModel::applySnapshot(const CatalogSnapshot &snapshot) {
    applyViewCache(CatalogViewCache::fromSnapshot(snapshot));
}

void SongModel::applyViewCache(const CatalogViewCache &cache) {
    m_snapshot = cache.sharedSnapshot();
    applyPrebuiltRows(cache.songAllRows(), cache.songSecondaryRows());

    const bool wasReady = m_catalogReady;
    m_catalogReady = true;
    if (!wasReady) {
        emit catalogReadyChanged();
    }
}

void SongModel::applyPrebuiltRows(QList<SongRow> allRows, QList<SongRow> secondaryRows) {
    const int previousTotal = totalCount();
    m_allRows = std::move(allRows);
    m_secondaryRows = std::move(secondaryRows);

    ensureFilterCurrent();
    applyFilter();

    if (previousTotal != totalCount()) {
        emit totalCountChanged();
    }
}

void SongModel::applyFilter() {
    beginResetModel();
    m_rows.clear();

    const auto appendIfMatches = [this](const SongRow &row) {
        if (matchesSearch(row)) {
            m_rows.append(row);
        }
    };

    if (m_containersOnly) {
        for (const SongRow &row : m_allRows) {
            if (isContainerHub(row)) {
                appendIfMatches(row);
            }
        }
    } else if (m_hideContainers) {
        for (const SongRow &row : m_allRows) {
            if (!isContainerHub(row)) {
                appendIfMatches(row);
            }
        }
        for (const SongRow &row : m_secondaryRows) {
            appendIfMatches(row);
        }
    } else {
        for (const SongRow &row : m_allRows) {
            appendIfMatches(row);
        }

        for (const SongRow &row : m_secondaryRows) {
            if (m_expandAllGroups ||
                (row.linkGroupId > 0 && m_expandedGroupIds.contains(row.linkGroupId))) {
                appendIfMatches(row);
            }
        }
    }

    endResetModel();
}

void SongModel::applyPendingSearchFilter() { applyFilter(); }

void SongModel::ensureFilterCurrent() {
    if (m_searchDebounce.isActive()) {
        m_searchDebounce.stop();
    }
}

bool SongModel::isContainerHub(const SongRow &row) {
    return row.isLinkedGroup && row.linkGroupId > 0 && !row.isContainerMember;
}

bool SongModel::matchesSearch(const SongRow &row) const {
    const QString needle = m_searchText.trimmed().toLower();
    if (needle.isEmpty()) {
        return true;
    }

    return row.searchHaystack.contains(needle);
}

QString SongModel::buildSearchHaystack(const SongRow &row) {
    return QStringLiteral("%1 %2 %3 %4 %5")
        .arg(row.title, row.displayTitle, row.linkGroupTitle, row.artistName, row.tuningName)
        .toLower();
}
