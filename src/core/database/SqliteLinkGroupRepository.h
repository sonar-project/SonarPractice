#ifndef SQLITELINKGROUPREPOSITORY_H
#define SQLITELINKGROUPREPOSITORY_H

#include "interfaces/IDatabaseConnection.h"
#include "interfaces/ILinkGroupRepository.h"

class SqliteLinkGroupRepository : public ILinkGroupRepository {
  public:
    explicit SqliteLinkGroupRepository(IDatabaseConnection &connection);

    [[nodiscard]] std::optional<qlonglong> createGroup(const LinkGroup &group) override;
    [[nodiscard]] std::optional<LinkGroup> getGroup(qlonglong groupId) override;
    [[nodiscard]] std::optional<LinkGroup> getGroupByPrimarySong(qlonglong primarySongId) override;
    [[nodiscard]] std::optional<LinkGroup> getGroupByPrimaryMedia(qlonglong primaryMediaId) override;
    [[nodiscard]] std::optional<LinkGroup> getGroupForSecondaryMedia(qlonglong secondaryMediaId) override;
    [[nodiscard]] bool updateTitle(qlonglong groupId, const QString &title) override;
    [[nodiscard]] bool deleteGroup(qlonglong groupId) override;
    [[nodiscard]] QList<LinkGroup> getAllGroups() override;

  private:
    static LinkGroup mapRow(class QSqlQuery &query);
    IDatabaseConnection &m_connection;
};

#endif // SQLITELINKGROUPREPOSITORY_H
