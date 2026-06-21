#include "LinkGroupService.h"

#include "LinkGroup.h"

#include <QHash>
#include <QVariantMap>

namespace {

    constexpr int kPrimaryMemberCount = 1;

} // namespace

LinkGroupService::LinkGroupService(const Dependencies &dependencies, QObject *parent)
    : QObject(parent), m_dependencies(dependencies) {}

std::optional<qlonglong> LinkGroupService::mediaIdForSong(qlonglong songId) const {
    const QHash<qlonglong, qlonglong> mediaBySong =
        m_dependencies.mediaFileRepo.firstMediaIdBySongIds({songId});
    const auto iterator = mediaBySong.constFind(songId);
    if (iterator == mediaBySong.cend()) {
        return std::nullopt;
    }
    return *iterator;
}

QHash<qlonglong, qlonglong>
LinkGroupService::mediaIdsForSongs(const QList<qlonglong> &songIds) const {
    return m_dependencies.mediaFileRepo.firstMediaIdBySongIds(songIds);
}

std::optional<qlonglong> LinkGroupService::createGroup(const QString &title,
                                                       qlonglong primarySongId,
                                                       qlonglong primaryMediaId,
                                                       const QList<qlonglong> &secondaryMediaIds) {
    if (title.trimmed().isEmpty() || primarySongId <= 0 || primaryMediaId <= 0) {
        return std::nullopt;
    }

    if (m_dependencies.fileRelationRepo.isMediaInAnyGroup(primaryMediaId)) {
        return std::nullopt;
    }

    for (qlonglong secondaryId : secondaryMediaIds) {
        if (secondaryId <= 0 || secondaryId == primaryMediaId) {
            return std::nullopt;
        }
        if (m_dependencies.fileRelationRepo.isMediaInAnyGroup(secondaryId)) {
            return std::nullopt;
        }
    }

    LinkGroup group;
    group.title = title.trimmed();
    group.primarySongId = primarySongId;
    group.primaryMediaId = primaryMediaId;

    const std::optional<qlonglong> groupId = m_dependencies.linkGroupRepo.createGroup(group);
    if (!groupId.has_value()) {
        return std::nullopt;
    }

    for (qlonglong secondaryId : secondaryMediaIds) {
        if (!m_dependencies.fileRelationRepo.linkToPrimary(primaryMediaId, secondaryId)) {
            m_dependencies.fileRelationRepo.deleteRelationsForPrimary(primaryMediaId);
            m_dependencies.linkGroupRepo.deleteGroup(*groupId);
            return std::nullopt;
        }
    }

    emit groupsChanged();
    return groupId;
}

bool LinkGroupService::dissolveGroup(qlonglong groupId) {
    const std::optional<LinkGroup> group = m_dependencies.linkGroupRepo.getGroup(groupId);
    if (!group.has_value()) {
        return false;
    }

    m_dependencies.fileRelationRepo.deleteRelationsForPrimary(group->primaryMediaId);
    const bool ok = m_dependencies.linkGroupRepo.deleteGroup(groupId);
    if (ok) {
        emit groupsChanged();
    }
    return ok;
}

std::optional<LinkGroup> LinkGroupService::groupForSong(qlonglong songId) const {
    if (songId <= 0) {
        return std::nullopt;
    }

    if (const std::optional<LinkGroup> primaryGroup =
            m_dependencies.linkGroupRepo.getGroupByPrimarySong(songId);
        primaryGroup.has_value()) {
        return primaryGroup;
    }

    const std::optional<qlonglong> mediaId = mediaIdForSong(songId);
    if (!mediaId.has_value()) {
        return std::nullopt;
    }

    return m_dependencies.linkGroupRepo.getGroupForSecondaryMedia(*mediaId);
}

QVariantList LinkGroupService::memberSongIdsForGroup(const LinkGroup &group) const {
    QVariantList memberSongIds;
    memberSongIds.append(group.primarySongId);

    const QList<MediaFile> linkedMedia =
        m_dependencies.fileRelationRepo.getLinkedMedia(group.primaryMediaId);
    for (const MediaFile &media : linkedMedia) {
        memberSongIds.append(media.songId);
    }

    return memberSongIds;
}

QVariantMap LinkGroupService::groupInfoForSong(qlonglong songId) const {
    QVariantMap info;
    const std::optional<LinkGroup> group = groupForSong(songId);
    if (!group.has_value()) {
        return info;
    }

    info.insert(QStringLiteral("groupId"), group->id);
    info.insert(QStringLiteral("title"), group->title);
    info.insert(QStringLiteral("primarySongId"), group->primarySongId);
    info.insert(QStringLiteral("primaryMediaId"), group->primaryMediaId);
    info.insert(QStringLiteral("memberSongIds"), memberSongIdsForGroup(*group));
    return info;
}

bool LinkGroupService::dissolveGroupForPrimarySong(qlonglong primarySongId) {
    const std::optional<LinkGroup> group =
        m_dependencies.linkGroupRepo.getGroupByPrimarySong(primarySongId);
    if (!group.has_value()) {
        return false;
    }
    return dissolveGroup(group->id);
}

qlonglong LinkGroupService::createGroupFromSongs(const QString &title, qlonglong primarySongId,
                                                 const QVariantList &secondarySongIds) {
    QList<qlonglong> songIds;
    songIds.reserve(secondarySongIds.size() + 1);
    songIds.append(primarySongId);

    for (const QVariant &value : secondarySongIds) {
        const qlonglong songId = value.toLongLong();
        if (songId > 0 && songId != primarySongId && !songIds.contains(songId)) {
            songIds.append(songId);
        }
    }

    const QHash<qlonglong, qlonglong> mediaBySong = mediaIdsForSongs(songIds);
    const auto primaryIterator = mediaBySong.constFind(primarySongId);
    if (primaryIterator == mediaBySong.cend()) {
        return 0;
    }

    QList<qlonglong> secondaryMediaIds;
    secondaryMediaIds.reserve(secondarySongIds.size());

    for (const QVariant &value : secondarySongIds) {
        const qlonglong songId = value.toLongLong();
        if (songId <= 0 || songId == primarySongId) {
            continue;
        }

        const auto iterator = mediaBySong.constFind(songId);
        if (iterator == mediaBySong.cend() || *iterator == *primaryIterator) {
            return 0;
        }

        secondaryMediaIds.append(*iterator);
    }

    const std::optional<qlonglong> groupId =
        createGroup(title, primarySongId, *primaryIterator, secondaryMediaIds);
    return groupId.value_or(0);
}

QVariantList LinkGroupService::dissolveGroupForSong(qlonglong songId) {
    const std::optional<LinkGroup> group = groupForSong(songId);
    if (!group.has_value()) {
        return {};
    }

    QVariantList memberSongIds = memberSongIdsForGroup(*group);
    if (!dissolveGroup(group->id)) {
        return {};
    }

    return memberSongIds;
}

bool LinkGroupService::updateGroupTitle(qlonglong songId, const QString &title) {
    const std::optional<LinkGroup> group = groupForSong(songId);
    if (!group.has_value()) {
        return false;
    }

    const bool ok = m_dependencies.linkGroupRepo.updateTitle(group->id, title);
    if (ok) {
        emit groupsChanged();
    }
    return ok;
}

QVariantList LinkGroupService::allGroups() const {
    QVariantList result;
    const QList<LinkGroup> groups = m_dependencies.linkGroupRepo.getAllGroups();

    for (const LinkGroup &group : groups) {
        const QList<MediaFile> linkedMedia =
            m_dependencies.fileRelationRepo.getLinkedMedia(group.primaryMediaId);

        result.append(QVariantMap{
            {QStringLiteral("groupId"), group.id},
            {QStringLiteral("title"), group.title},
            {QStringLiteral("primarySongId"), group.primarySongId},
            {QStringLiteral("memberCount"), linkedMedia.size() + kPrimaryMemberCount},
        });
    }

    return result;
}

bool LinkGroupService::addSongsToGroup(qlonglong groupId, const QVariantList &songIds) {
    const std::optional<LinkGroup> group = m_dependencies.linkGroupRepo.getGroup(groupId);
    if (!group.has_value()) {
        return false;
    }

    QList<qlonglong> requestedSongIds;
    requestedSongIds.reserve(songIds.size());

    for (const QVariant &value : songIds) {
        const qlonglong songId = value.toLongLong();
        if (songId > 0 && songId != group->primarySongId) {
            requestedSongIds.append(songId);
        }
    }

    const QHash<qlonglong, qlonglong> mediaBySong = mediaIdsForSongs(requestedSongIds);
    bool anyAdded = false;

    for (const qlonglong songId : requestedSongIds) {
        const auto iterator = mediaBySong.constFind(songId);
        if (iterator == mediaBySong.cend() || *iterator == group->primaryMediaId) {
            continue;
        }

        if (m_dependencies.fileRelationRepo.isMediaInAnyGroup(*iterator)) {
            continue;
        }

        if (m_dependencies.fileRelationRepo.linkToPrimary(group->primaryMediaId, *iterator)) {
            anyAdded = true;
        }
    }

    if (anyAdded) {
        emit groupsChanged();
    }

    return anyAdded;
}
