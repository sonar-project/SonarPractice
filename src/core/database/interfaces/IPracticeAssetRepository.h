#ifndef IPRACTICEASSETREPOSITORY_H
#define IPRACTICEASSETREPOSITORY_H

#include "PracticeAsset.h"
#include "PracticeSyncMarker.h"

#include <optional>
#include <vector>

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

    /**
     * Returns the practice_assets.id from the most recent journal entry for a song.
     */
    [[nodiscard]] virtual qlonglong lastPracticeAssetIdForSong(qlonglong songId) = 0;

    /** Markers for one GP+audio pair, ordered along the score. */
    [[nodiscard]] virtual std::vector<PracticeSyncMarker> listSyncMarkers(qlonglong assetId) = 0;
    /** Inserts or replaces the marker at the same bar and beat. */
    [[nodiscard]] virtual bool saveSyncMarker(const PracticeSyncMarker &marker) = 0;
    [[nodiscard]] virtual bool deleteSyncMarker(qlonglong assetId, int bar, int beat) = 0;
    /** True when playback should hear the audio before the first marker. */
    [[nodiscard]] virtual bool audioLeadIn(qlonglong assetId) = 0;
    [[nodiscard]] virtual bool setAudioLeadIn(qlonglong assetId, bool enabled) = 0;

  protected:
    IPracticeAssetRepository() = default;
};

#endif // IPRACTICEASSETREPOSITORY_H
