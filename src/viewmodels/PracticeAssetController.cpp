/**
 * @file PracticeAssetController.cpp
 * @brief QML-facing facade for practice asset lookup and upsert.
 */

#include "PracticeAssetController.h"

#include "MediaFile.h"
#include "MediaFileListModel.h"
#include "interfaces/IMediaFileRepository.h"
#include "interfaces/IPracticeAssetRepository.h"

PracticeAssetController::PracticeAssetController(IPracticeAssetRepository &assetRepo,
                                                 IMediaFileRepository &mediaFileRepo,
                                                 QObject *parent)
    : QObject(parent), m_assetRepo(assetRepo), m_mediaFileRepository(mediaFileRepo) {}

qlonglong PracticeAssetController::lastMediaFileIdForSong(qlonglong songId) {
    return m_assetRepo.lastPrimaryMediaIdForSong(songId);
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

QVariantList PracticeAssetController::filteredAudioFiles(const QString &filter) const {
    QVariantList result;

    const auto allFiles = m_mediaFileRepository.getAllMediaFiles();

    for (const auto &file : allFiles) {
        const QString name = MediaFileListModel::displayNameForFile(file);

        if (file.mediaKind == MediaKind::Audio && name.contains(filter, Qt::CaseInsensitive)) {
            QVariantMap map;
            map[QStringLiteral("mediaId")] = file.id;
            map[QStringLiteral("displayName")] = name;
            result.append(map);
        }
    }
    return result;
}
