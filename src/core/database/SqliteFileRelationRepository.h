#ifndef SQLITEFILERELATIONREPOSITORY_H
#define SQLITEFILERELATIONREPOSITORY_H

#include "interfaces/IDatabaseConnection.h"
#include "interfaces/IFileRelationRepository.h"

class SqliteFileRelationRepository : public IFileRelationRepository {
  public:
    explicit SqliteFileRelationRepository(IDatabaseConnection &connection);

    [[nodiscard]] bool linkToPrimary(qlonglong primaryMediaId, qlonglong secondaryMediaId) override;
    [[nodiscard]] bool unlink(qlonglong secondaryMediaId) override;
    [[nodiscard]] bool deleteRelationsForPrimary(qlonglong primaryMediaId) override;
    [[nodiscard]] QList<MediaFile> getLinkedMedia(qlonglong primaryMediaId) override;
    [[nodiscard]] bool isSecondaryMedia(qlonglong mediaId) override;
    [[nodiscard]] std::optional<qlonglong> getPrimaryMediaId(qlonglong mediaId) override;
    [[nodiscard]] QList<std::pair<qlonglong, qlonglong>> getAllRelationPairs() override;
    [[nodiscard]] bool isMediaInAnyGroup(qlonglong mediaId) override;

  private:
    static MediaFile mapMediaRow(class QSqlQuery &query);
    IDatabaseConnection &m_connection;
};

#endif // SQLITEFILERELATIONREPOSITORY_H
