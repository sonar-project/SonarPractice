#ifndef LINKGROUPSERVICE_H
#define LINKGROUPSERVICE_H

#include "LinkGroup.h"

#include "interfaces/IFileRelationRepository.h"
#include "interfaces/ILinkGroupRepository.h"
#include "interfaces/IMediaFileRepository.h"
#include "interfaces/ISongRepository.h"

#include <QObject>
#include <QVariant>

class LinkGroupService : public QObject {
    Q_OBJECT

  public:
    struct Dependencies {
        ILinkGroupRepository &linkGroupRepo;
        IFileRelationRepository &fileRelationRepo;
        IMediaFileRepository &mediaFileRepo;
        ISongRepository &songRepo;
    };

    explicit LinkGroupService(const Dependencies &dependencies, QObject *parent = nullptr);

    [[nodiscard]] std::optional<qlonglong> createGroup(const QString &title,
                                                       qlonglong primarySongId,
                                                       qlonglong primaryMediaId,
                                                       const QList<qlonglong> &secondaryMediaIds);

    [[nodiscard]] bool dissolveGroup(qlonglong groupId);
    [[nodiscard]] bool dissolveGroupForPrimarySong(qlonglong primarySongId);

    /// Creates a group and returns its id, or 0 when creation fails.
    Q_INVOKABLE qlonglong createGroupFromSongs(const QString &title, qlonglong primarySongId,
                                               const QVariantList &secondarySongIds);
    /// Dissolves the group and returns affected song ids, or an empty list on failure.
    Q_INVOKABLE QVariantList dissolveGroupForSong(qlonglong songId);
    Q_INVOKABLE bool updateGroupTitle(qlonglong primarySongId, const QString &title);
    Q_INVOKABLE QVariantMap groupInfoForSong(qlonglong songId) const;
    Q_INVOKABLE QVariantList allGroups() const;
    Q_INVOKABLE bool addSongsToGroup(qlonglong groupId, const QVariantList &songIds);

  signals:
    void groupsChanged();

  private:
    [[nodiscard]] std::optional<qlonglong> mediaIdForSong(qlonglong songId) const;
    [[nodiscard]] QHash<qlonglong, qlonglong>
    mediaIdsForSongs(const QList<qlonglong> &songIds) const;
    [[nodiscard]] std::optional<LinkGroup> groupForSong(qlonglong songId) const;
    [[nodiscard]] QVariantList memberSongIdsForGroup(const LinkGroup &group) const;

    Dependencies m_dependencies;
};

#endif // LINKGROUPSERVICE_H
