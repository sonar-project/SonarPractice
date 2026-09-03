// src/viewmodels/PracticeAssetController.h
#ifndef PRACTICEASSETCONTROLLER_H
#define PRACTICEASSETCONTROLLER_H

#include "PracticeAsset.h"
#include "interfaces/IPracticeAssetRepository.h"
#include <QObject>
#include <QtQml/qqmlregistration.h>

class PracticeAssetController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use context property 'practiceAssetController'.")

  public:
    explicit PracticeAssetController(IPracticeAssetRepository &assetRepo,
                                     QObject *parent = nullptr);

    Q_INVOKABLE qlonglong lastMediaFileIdForSong(qlonglong songId);
    /** Most recent practice_assets.id used for this song (from journal), or 0. */
    Q_INVOKABLE qlonglong lastPracticeAssetIdForSong(qlonglong songId);
    Q_INVOKABLE qlonglong mediaFileIdForAsset(qlonglong assetId);

    Q_INVOKABLE QVariantMap assetById(qlonglong assetId) const;

    [[nodiscard]] std::optional<PracticeAsset> loadAsset(qlonglong assetId) const;

    qlonglong upsertCompositeAsset(const PracticeAsset &asset);

    Q_INVOKABLE qlonglong upsertCompositeAsset(qlonglong songId, qlonglong guitarProId,
                                               qlonglong audioId, qlonglong videoId,
                                               qlonglong imageId, qlonglong documentId);

  private:
    IPracticeAssetRepository &m_assetRepo;
};

#endif // PRACTICEASSETCONTROLLER_H
