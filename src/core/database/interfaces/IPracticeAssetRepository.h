#ifndef IPRACTICEASSETREPOSITORY_H
#define IPRACTICEASSETREPOSITORY_H

#include "PracticeAsset.h"

#include <optional>

class IPracticeAssetRepository {
  public:
    virtual ~IPracticeAssetRepository() = default;

    IPracticeAssetRepository(const IPracticeAssetRepository &) = delete;
    IPracticeAssetRepository &operator=(const IPracticeAssetRepository &) = delete;
    IPracticeAssetRepository(IPracticeAssetRepository &&) = delete;
    IPracticeAssetRepository &operator=(IPracticeAssetRepository &&) = delete;

    [[nodiscard]] virtual std::optional<PracticeAsset> getById(qlonglong id) = 0;

    /**
     * Natural key: (song_id, guitar_pro_id, audio_id, video_id, image_id, document_id).
     * Returns existing or newly inserted row id, nullopt on error.
     */
    [[nodiscard]] virtual std::optional<qlonglong> upsert(const PracticeAsset &asset) = 0;

    /**
     * Returns the primary media id from the most recent journal entry for a song.
     */
    [[nodiscard]] virtual qlonglong lastPrimaryMediaIdForSong(qlonglong songId) = 0;

  protected:
    IPracticeAssetRepository() = default;
};

#endif // IPRACTICEASSETREPOSITORY_H
