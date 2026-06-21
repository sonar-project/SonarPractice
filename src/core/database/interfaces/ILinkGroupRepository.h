#ifndef ILINKGROUPREPOSITORY_H
#define ILINKGROUPREPOSITORY_H

#include <optional>

#include <QList>

#include "LinkGroup.h"

class ILinkGroupRepository {
  public:
    virtual ~ILinkGroupRepository() = default;

    ILinkGroupRepository(const ILinkGroupRepository &) = delete;
    ILinkGroupRepository &operator=(const ILinkGroupRepository &) = delete;
    ILinkGroupRepository(ILinkGroupRepository &&) = delete;
    ILinkGroupRepository &operator=(ILinkGroupRepository &&) = delete;

    virtual std::optional<qlonglong> createGroup(const LinkGroup &group) = 0;
    virtual std::optional<LinkGroup> getGroup(qlonglong groupId) = 0;
    virtual std::optional<LinkGroup> getGroupByPrimarySong(qlonglong primarySongId) = 0;
    virtual std::optional<LinkGroup> getGroupByPrimaryMedia(qlonglong primaryMediaId) = 0;
    virtual std::optional<LinkGroup> getGroupForSecondaryMedia(qlonglong secondaryMediaId) = 0;
    virtual bool updateTitle(qlonglong groupId, const QString &title) = 0;
    virtual bool deleteGroup(qlonglong groupId) = 0;
    virtual QList<LinkGroup> getAllGroups() = 0;

  protected:
    ILinkGroupRepository() = default;
};

#endif // ILINKGROUPREPOSITORY_H
