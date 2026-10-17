/**
 * @file PracticeAssetController.cpp
 * @brief QML-facing facade for practice asset lookup and upsert.
 */

#include "PracticeAssetController.h"

#include "interfaces/IPracticeAssetRepository.h"

#include <QVariantList>
#include <QVariantMap>

PracticeAssetController::PracticeAssetController(IPracticeAssetRepository &assetRepo,
                                                 QObject *parent)
    : QObject(parent), m_assetRepo(assetRepo) {}

qlonglong PracticeAssetController::lastMediaFileIdForSong(qlonglong songId) {
    return m_assetRepo.lastPrimaryMediaIdForSong(songId);
}

qlonglong PracticeAssetController::lastPracticeAssetIdForSong(qlonglong songId) {
    return m_assetRepo.lastPracticeAssetIdForSong(songId);
}

qlonglong PracticeAssetController::mediaFileIdForAsset(qlonglong assetId) {
    const auto asset = m_assetRepo.getById(assetId);
    if (!asset.has_value()) {
        return 0;
    }
    return asset->primaryMediaId();
}

QVariantMap PracticeAssetController::assetById(qlonglong assetId) const {
    const auto asset = loadAsset(assetId);
    if (!asset.has_value()) {
        return {};
    }
    return {
        {"guitarProId", asset->guitarProId}, {"audioId", asset->audioId},
        {"videoId", asset->videoId},         {"imageId", asset->imageId},
        {"documentId", asset->documentId},
    };
}

std::optional<PracticeAsset> PracticeAssetController::loadAsset(qlonglong assetId) const {
    return m_assetRepo.getById(assetId);
}

qlonglong PracticeAssetController::upsertCompositeAsset(const PracticeAsset &asset) {
    const std::optional<qlonglong> id = m_assetRepo.upsert(asset);
    return id.value_or(0);
}

qlonglong PracticeAssetController::upsertCompositeAsset(qlonglong songId, qlonglong guitarProId,
                                                        qlonglong audioId, qlonglong videoId,
                                                        qlonglong imageId, qlonglong documentId) {
    PracticeAsset asset;
    asset.songId = songId;
    asset.guitarProId = guitarProId;
    asset.audioId = audioId;
    asset.videoId = videoId;
    asset.imageId = imageId;
    asset.documentId = documentId;
    return upsertCompositeAsset(asset);
}

QVariantList PracticeAssetController::syncMarkers(qlonglong assetId) const {
    QVariantList list;
    const std::vector<PracticeSyncMarker> markers = m_assetRepo.listSyncMarkers(assetId);
    list.reserve(static_cast<int>(markers.size()));
    for (const PracticeSyncMarker &marker : markers) {
        list.push_back(QVariantMap{
            {QStringLiteral("bar"), marker.bar},
            {QStringLiteral("beat"), marker.beat},
            {QStringLiteral("tick"), marker.tick},
            {QStringLiteral("audioMs"), marker.audioMs},
        });
    }
    return list;
}

bool PracticeAssetController::saveSyncMarker(qlonglong assetId, int bar, int beat, int tick,
                                             int audioMs) {
    PracticeSyncMarker marker;
    marker.assetId = assetId;
    marker.bar = bar;
    marker.beat = beat;
    marker.tick = tick;
    marker.audioMs = audioMs;
    return m_assetRepo.saveSyncMarker(marker);
}

bool PracticeAssetController::deleteSyncMarker(qlonglong assetId, int bar, int beat) {
    return m_assetRepo.deleteSyncMarker(assetId, bar, beat);
}

bool PracticeAssetController::audioLeadIn(qlonglong assetId) const {
    return m_assetRepo.audioLeadIn(assetId);
}

bool PracticeAssetController::setAudioLeadIn(qlonglong assetId, bool enabled) {
    return m_assetRepo.setAudioLeadIn(assetId, enabled);
}
