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

#include <QHash>
#include <QSet>
#include <algorithm>

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
    case MediaIdRole:
        return row.mediaId;
    case IsFavoriteRole:
        return row.isFavorite;
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
        {MediaIdRole, "mediaId"},
        {IsFavoriteRole, "isFavorite"},
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
    ensureFilterCurrent();
    applyFilter();
    emit expandAllGroupsChanged();
}

bool SongModel::catalogReady() const { return m_catalogReady; }

const QString &SongModel::mediaKindFilter() const { return m_mediaKindFilter; }

void SongModel::setMediaKindFilter(const QString &kind) {
    const QString normalized = kind.trimmed().toLower();
    if (m_mediaKindFilter == normalized) {
        return;
    }

    m_mediaKindFilter = normalized;
    ensureFilterCurrent();
    applyFilter();
    emit mediaKindFilterChanged();
}

int SongModel::bpmFilter() const { return m_bpmFilter; }

void SongModel::setBpmFilter(int bpm) {
    const int normalized = bpm > 0 ? bpm : 0;
    if (m_bpmFilter == normalized) {
        return;
    }

    m_bpmFilter = normalized;
    ensureFilterCurrent();
    applyFilter();
    emit bpmFilterChanged();
}

int SongModel::tuningIdFilter() const { return m_tuningIdFilter; }

void SongModel::setTuningIdFilter(int tuningId) {
    const int normalized = tuningId > 0 ? tuningId : 0;
    if (m_tuningIdFilter == normalized) {
        return;
    }

    m_tuningIdFilter = normalized;
    ensureFilterCurrent();
    applyFilter();
    emit tuningIdFilterChanged();
}

bool SongModel::favoritesOnly() const { return m_favoritesOnly; }

void SongModel::setFavoritesOnly(bool only) {
    if (m_favoritesOnly == only) {
        return;
    }

    m_favoritesOnly = only;
    ensureFilterCurrent();
    applyFilter();
    emit favoritesOnlyChanged();
}

const QVariantList &SongModel::knownBpms() const { return m_knownBpms; }

const QVariantList &SongModel::knownTunings() const { return m_knownTunings; }

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

bool SongModel::setFavorite(qlonglong songId, bool favorite) {
    if (songId <= 0) {
        return false;
    }

    if (!m_songRepo.setFavorite(songId, favorite)) {
        return false;
    }

    updateFavoriteInRows(songId, favorite);
    applyFilter();
    return true;
}

void SongModel::applyPrebuiltRows(QList<SongRow> allRows, QList<SongRow> secondaryRows) {
    const int previousTotal = totalCount();
    m_allRows = std::move(allRows);
    m_secondaryRows = std::move(secondaryRows);

    rebuildKnownFilters();
    ensureFilterCurrent();
    applyFilter();

    if (previousTotal != totalCount()) {
        emit totalCountChanged();
    }
}

void SongModel::rebuildKnownFilters() {
    QSet<int> bpms;
    QHash<qlonglong, QString> tunings;

    const auto collect = [&](const SongRow &row) {
        if (row.baseBpm > 0) {
            bpms.insert(row.baseBpm);
        }
        if (row.tuningId > 0 && !row.tuningName.isEmpty()) {
            tunings.insert(row.tuningId, row.tuningName);
        }
    };

    for (const SongRow &row : m_allRows) {
        collect(row);
    }
    for (const SongRow &row : m_secondaryRows) {
        collect(row);
    }

    QList<int> sortedBpms = bpms.values();
    std::sort(sortedBpms.begin(), sortedBpms.end());

    QVariantList bpmList;
    bpmList.reserve(sortedBpms.size());
    for (int bpm : sortedBpms) {
        bpmList.append(bpm);
    }

    QList<QPair<QString, qlonglong>> sortedTunings;
    sortedTunings.reserve(tunings.size());
    for (auto it = tunings.cbegin(); it != tunings.cend(); ++it) {
        sortedTunings.append({it.value(), it.key()});
    }
    std::sort(sortedTunings.begin(), sortedTunings.end(),
              [](const QPair<QString, qlonglong> &a, const QPair<QString, qlonglong> &b) {
                  return a.first.localeAwareCompare(b.first) < 0;
              });

    QVariantList tuningList;
    tuningList.reserve(sortedTunings.size());
    for (const auto &entry : sortedTunings) {
        tuningList.append(QVariantMap{
            {QStringLiteral("id"), static_cast<int>(entry.second)},
            {QStringLiteral("name"), entry.first},
        });
    }

    if (m_knownBpms == bpmList && m_knownTunings == tuningList) {
        return;
    }

    m_knownBpms = std::move(bpmList);
    m_knownTunings = std::move(tuningList);
    emit knownFiltersChanged();
}

void SongModel::applyFilter() {
    beginResetModel();
    m_rows.clear();

    const auto appendIfMatches = [this](const SongRow &row) {
        if (matchesFilters(row)) {
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
            if (m_expandAllGroups) {
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

bool SongModel::rowHasMediaKind(const SongRow &row, const QString &kind) const {
    for (const QVariant &entry : row.assetSummary) {
        const QVariantMap map = entry.toMap();
        if (map.value(QStringLiteral("kind")).toString() == kind) {
            return true;
        }
    }
    return false;
}

bool SongModel::matchesFilters(const SongRow &row) const {
    if (m_favoritesOnly && !row.isFavorite) {
        return false;
    }

    if (m_bpmFilter > 0 && row.baseBpm != m_bpmFilter) {
        return false;
    }

    if (m_tuningIdFilter > 0 && row.tuningId != m_tuningIdFilter) {
        return false;
    }

    if (!m_mediaKindFilter.isEmpty() && !rowHasMediaKind(row, m_mediaKindFilter)) {
        return false;
    }

    const QString needle = m_searchText.trimmed().toLower();
    if (!needle.isEmpty() && !row.searchHaystack.contains(needle)) {
        return false;
    }

    return true;
}

void SongModel::updateFavoriteInRows(qlonglong songId, bool favorite) {
    const auto updateList = [songId, favorite](QList<SongRow> &rows) {
        for (SongRow &row : rows) {
            if (row.id == songId) {
                row.isFavorite = favorite;
            }
        }
    };

    updateList(m_allRows);
    updateList(m_secondaryRows);
}

QString SongModel::buildSearchHaystack(const SongRow &row) {
    return QStringLiteral("%1 %2 %3 %4 %5")
        .arg(row.title, row.displayTitle, row.linkGroupTitle, row.artistName, row.tuningName)
        .toLower();
}
