#include "SqlitePracticeAssetRepository.h"

#include "RepositoryUtils.h"
#include "SqlQueryColumns.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

    QVariant nullOrVal(qlonglong value) { return value > 0 ? QVariant(value) : QVariant(); }

} // namespace

SqlitePracticeAssetRepository::SqlitePracticeAssetRepository(IDatabaseConnection &connection)
    : m_connection(connection) {}

PracticeAsset SqlitePracticeAssetRepository::assetFromQuery(QSqlQuery &query) {
    return PracticeAsset{
        .id = query.value(SqlQueryColumns::PracticeAsset::Id).toLongLong(),
        .songId = query.value(SqlQueryColumns::PracticeAsset::SongId).toLongLong(),
        .guitarProId = query.value(SqlQueryColumns::PracticeAsset::GuitarProId).toLongLong(),
        .audioId = query.value(SqlQueryColumns::PracticeAsset::AudioId).toLongLong(),
        .videoId = query.value(SqlQueryColumns::PracticeAsset::VideoId).toLongLong(),
        .imageId = query.value(SqlQueryColumns::PracticeAsset::ImageId).toLongLong(),
        .documentId = query.value(SqlQueryColumns::PracticeAsset::DocumentId).toLongLong(),
    };
}

std::optional<PracticeAsset> SqlitePracticeAssetRepository::getById(qlonglong id) {
    if (id <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(QStringLiteral("SELECT id, song_id, guitar_pro_id, audio_id, video_id, image_id, "
                                 "document_id "
                                 "FROM practice_assets WHERE id = :id LIMIT 1"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    return assetFromQuery(query);
}

std::optional<qlonglong> SqlitePracticeAssetRepository::upsert(const PracticeAsset &asset) {
    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlDatabase db = RepositoryUtils::database(m_connection);

    QSqlQuery sel(db);
    sel.prepare(QStringLiteral("SELECT id FROM practice_assets "
                               "WHERE song_id = :sid AND "
                               "guitar_pro_id IS :gp AND "
                               "audio_id IS :au AND "
                               "video_id IS :vi AND "
                               "image_id IS :im AND "
                               "document_id IS :doc LIMIT 1"));
    sel.bindValue(QStringLiteral(":sid"), nullOrVal(asset.songId));
    sel.bindValue(QStringLiteral(":gp"), nullOrVal(asset.guitarProId));
    sel.bindValue(QStringLiteral(":au"), nullOrVal(asset.audioId));
    sel.bindValue(QStringLiteral(":vi"), nullOrVal(asset.videoId));
    sel.bindValue(QStringLiteral(":im"), nullOrVal(asset.imageId));
    sel.bindValue(QStringLiteral(":doc"), nullOrVal(asset.documentId));

    if (sel.exec() && sel.next()) {
        const qlonglong existingId = sel.value(0).toLongLong();
        return existingId > 0 ? std::optional<qlonglong>(existingId) : std::nullopt;
    }

    QSqlQuery ins(db);
    ins.prepare(
        QStringLiteral("INSERT INTO practice_assets "
                       "(song_id, guitar_pro_id, audio_id, video_id, image_id, document_id) "
                       "VALUES (:sid, :gp, :au, :vi, :im, :doc)"));
    ins.bindValue(QStringLiteral(":sid"), nullOrVal(asset.songId));
    ins.bindValue(QStringLiteral(":gp"), nullOrVal(asset.guitarProId));
    ins.bindValue(QStringLiteral(":au"), nullOrVal(asset.audioId));
    ins.bindValue(QStringLiteral(":vi"), nullOrVal(asset.videoId));
    ins.bindValue(QStringLiteral(":im"), nullOrVal(asset.imageId));
    ins.bindValue(QStringLiteral(":doc"), nullOrVal(asset.documentId));

    if (!ins.exec()) {
        qCritical() << "[SqlitePracticeAssetRepository] upsert failed:" << ins.lastError().text();
        return std::nullopt;
    }

    const qlonglong newId = ins.lastInsertId().toLongLong();
    return newId > 0 ? std::optional<qlonglong>(newId) : std::nullopt;
}

qlonglong SqlitePracticeAssetRepository::lastPrimaryMediaIdForSong(qlonglong songId) {
    if (songId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return 0;
    }

    QSqlQuery sel(RepositoryUtils::database(m_connection));
    sel.prepare(QStringLiteral(
        "SELECT COALESCE(pa.guitar_pro_id, pa.audio_id, pa.video_id, pa.image_id, pa.document_id) "
        "FROM practice_journal pj "
        "JOIN practice_assets pa ON pa.id = pj.asset_id "
        "WHERE pa.song_id = :song_id "
        "AND (pa.guitar_pro_id IS NOT NULL OR pa.audio_id IS NOT NULL OR "
        "     pa.video_id IS NOT NULL OR pa.image_id IS NOT NULL OR pa.document_id IS NOT NULL) "
        "ORDER BY pj.practice_date DESC, pj.id DESC "
        "LIMIT 1"));
    sel.bindValue(QStringLiteral(":song_id"), songId);

    if (!sel.exec()) {
        qCritical() << "[SqlitePracticeAssetRepository] lastPrimaryMediaIdForSong failed:"
                    << sel.lastError().text();
        return 0;
    }

    if (sel.next()) {
        return sel.value(0).toLongLong();
    }
    return 0;
}

qlonglong SqlitePracticeAssetRepository::lastPracticeAssetIdForSong(qlonglong songId) {
    if (songId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return 0;
    }

    QSqlQuery sel(RepositoryUtils::database(m_connection));
    sel.prepare(QStringLiteral(
        "SELECT pa.id "
        "FROM practice_journal pj "
        "JOIN practice_assets pa ON pa.id = pj.asset_id "
        "WHERE pa.song_id = :song_id "
        "AND (pa.guitar_pro_id IS NOT NULL OR pa.audio_id IS NOT NULL OR "
        "     pa.video_id IS NOT NULL OR pa.image_id IS NOT NULL OR pa.document_id IS NOT NULL) "
        "ORDER BY pj.practice_date DESC, pj.id DESC "
        "LIMIT 1"));
    sel.bindValue(QStringLiteral(":song_id"), songId);

    if (!sel.exec()) {
        qCritical() << "[SqlitePracticeAssetRepository] lastPracticeAssetIdForSong failed:"
                    << sel.lastError().text();
        return 0;
    }

    if (sel.next()) {
        return sel.value(0).toLongLong();
    }
    return 0;
}

std::vector<PracticeSyncMarker> SqlitePracticeAssetRepository::listSyncMarkers(qlonglong assetId) {
    std::vector<PracticeSyncMarker> markers;
    if (assetId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return markers;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(QStringLiteral(
        "SELECT id, asset_id, bar, beat, tick, audio_ms "
        "FROM practice_sync_markers WHERE asset_id = :asset "
        "ORDER BY tick ASC, bar ASC, beat ASC, audio_ms ASC"));
    query.bindValue(QStringLiteral(":asset"), assetId);
    if (!query.exec()) {
        qCritical() << "[SqlitePracticeAssetRepository] listSyncMarkers failed:"
                    << query.lastError().text();
        return markers;
    }

    while (query.next()) {
        PracticeSyncMarker marker;
        marker.id = query.value(0).toLongLong();
        marker.assetId = query.value(1).toLongLong();
        marker.bar = query.value(2).toInt();
        marker.beat = query.value(3).toInt();
        marker.tick = query.value(4).toInt();
        marker.audioMs = query.value(5).toLongLong();
        markers.push_back(marker);
    }
    return markers;
}

bool SqlitePracticeAssetRepository::saveSyncMarker(const PracticeSyncMarker &marker) {
    if (marker.assetId <= 0 || marker.bar < 1 || marker.beat < 1 || marker.audioMs < 0 ||
        !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(QStringLiteral(
        "INSERT INTO practice_sync_markers (asset_id, bar, beat, tick, audio_ms) "
        "VALUES (:asset, :bar, :beat, :tick, :ms) "
        "ON CONFLICT(asset_id, bar, beat) DO UPDATE SET "
        "tick = excluded.tick, audio_ms = excluded.audio_ms"));
    query.bindValue(QStringLiteral(":asset"), marker.assetId);
    query.bindValue(QStringLiteral(":bar"), marker.bar);
    query.bindValue(QStringLiteral(":beat"), marker.beat);
    query.bindValue(QStringLiteral(":tick"), qMax(0, marker.tick));
    query.bindValue(QStringLiteral(":ms"), marker.audioMs);
    if (!query.exec()) {
        qCritical() << "[SqlitePracticeAssetRepository] saveSyncMarker failed:"
                    << query.lastError().text();
        return false;
    }
    return true;
}

bool SqlitePracticeAssetRepository::deleteSyncMarker(qlonglong assetId, int bar, int beat) {
    if (assetId <= 0 || bar < 1 || beat < 1 || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(QStringLiteral("DELETE FROM practice_sync_markers "
                                 "WHERE asset_id = :asset AND bar = :bar AND beat = :beat"));
    query.bindValue(QStringLiteral(":asset"), assetId);
    query.bindValue(QStringLiteral(":bar"), bar);
    query.bindValue(QStringLiteral(":beat"), beat);
    if (!query.exec()) {
        qCritical() << "[SqlitePracticeAssetRepository] deleteSyncMarker failed:"
                    << query.lastError().text();
        return false;
    }
    return true;
}

bool SqlitePracticeAssetRepository::audioLeadIn(qlonglong assetId) {
    if (assetId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return true;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(QStringLiteral("SELECT audio_lead_in FROM practice_assets WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), assetId);
    if (!query.exec() || !query.next()) {
        return true;
    }
    return query.value(0).toInt() != 0;
}

bool SqlitePracticeAssetRepository::setAudioLeadIn(qlonglong assetId, bool enabled) {
    if (assetId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(
        QStringLiteral("UPDATE practice_assets SET audio_lead_in = :lead WHERE id = :id"));
    query.bindValue(QStringLiteral(":lead"), enabled ? 1 : 0);
    query.bindValue(QStringLiteral(":id"), assetId);
    if (!query.exec()) {
        qCritical() << "[SqlitePracticeAssetRepository] setAudioLeadIn failed:"
                    << query.lastError().text();
        return false;
    }
    return true;
}
