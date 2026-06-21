#ifndef SQLITEPRACTICEASSETREPOSITORY_H
#define SQLITEPRACTICEASSETREPOSITORY_H

#include "interfaces/IDatabaseConnection.h"
#include "interfaces/IPracticeAssetRepository.h"

class SqlitePracticeAssetRepository final : public IPracticeAssetRepository {
  public:
    explicit SqlitePracticeAssetRepository(IDatabaseConnection &connection);

    [[nodiscard]] std::optional<PracticeAsset> getById(qlonglong id) override;
    [[nodiscard]] std::optional<qlonglong> upsert(const PracticeAsset &asset) override;
    [[nodiscard]] qlonglong lastPrimaryMediaIdForSong(qlonglong songId) override;

  private:
    static PracticeAsset assetFromQuery(class QSqlQuery &query);

    IDatabaseConnection &m_connection;
};

#endif // SQLITEPRACTICEASSETREPOSITORY_H
