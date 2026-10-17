#ifndef SQLITEPRACTICEASSETREPOSITORY_H
#define SQLITEPRACTICEASSETREPOSITORY_H

#include "interfaces/IDatabaseConnection.h"
#include "interfaces/IPracticeAssetRepository.h"

#include <vector>

class SqlitePracticeAssetRepository final : public IPracticeAssetRepository {
  public:
    explicit SqlitePracticeAssetRepository(IDatabaseConnection &connection);

    [[nodiscard]] std::optional<PracticeAsset> getById(qlonglong id) override;
    [[nodiscard]] std::optional<qlonglong> upsert(const PracticeAsset &asset) override;
    [[nodiscard]] qlonglong lastPrimaryMediaIdForSong(qlonglong songId) override;
    [[nodiscard]] qlonglong lastPracticeAssetIdForSong(qlonglong songId) override;
    [[nodiscard]] std::vector<PracticeSyncMarker> listSyncMarkers(qlonglong assetId) override;
    [[nodiscard]] bool saveSyncMarker(const PracticeSyncMarker &marker) override;
    [[nodiscard]] bool deleteSyncMarker(qlonglong assetId, int bar, int beat) override;
    [[nodiscard]] bool audioLeadIn(qlonglong assetId) override;
    [[nodiscard]] bool setAudioLeadIn(qlonglong assetId, bool enabled) override;

  private:
    static PracticeAsset assetFromQuery(class QSqlQuery &query);

    IDatabaseConnection &m_connection;
};

#endif // SQLITEPRACTICEASSETREPOSITORY_H
