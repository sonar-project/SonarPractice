#include "SqliteFileRelationRepository.h"

#include "MediaFileMapping.h"
#include "RepositoryUtils.h"
#include "SqlQueryColumns.h"

#include <QSqlError>
#include <QSqlQuery>

/**
 * @brief Constructs the repository with a database connection.
 * @param connection The database connection object to use.
 */
SqliteFileRelationRepository::SqliteFileRelationRepository(IDatabaseConnection &connection)
    : m_connection(connection) {}

/**
 * @brief Creates a link between a primary media file and a secondary media file.
 * @param primaryMediaId The ID of the main media file.
 * @param secondaryMediaId The ID of the media file to link as secondary.
 * @return True if the link was created successfully, false otherwise.
 */
bool SqliteFileRelationRepository::linkToPrimary(qlonglong primaryMediaId,
                                                 qlonglong secondaryMediaId) {
    if (primaryMediaId <= 0 || secondaryMediaId <= 0 || primaryMediaId == secondaryMediaId) {
        return false;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    if (isMediaInAnyGroup(secondaryMediaId)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("INSERT OR IGNORE INTO file_relations (file_id_a, file_id_b) "
                  "VALUES (:primary_id, :secondary_id)");
    query.bindValue(":primary_id", primaryMediaId);
    query.bindValue(":secondary_id", secondaryMediaId);

    if (!query.exec()) {
        qCritical() << "[SqliteFileRelationRepository] linkToPrimary failed:"
                    << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() == 1;
}

/**
 * @brief Removes the link for a specific secondary media file.
 * @param secondaryMediaId The ID of the secondary media file to unlink.
 * @return True if the link was removed successfully, false otherwise.
 */
bool SqliteFileRelationRepository::unlink(qlonglong secondaryMediaId) {
    if (secondaryMediaId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("DELETE FROM file_relations WHERE file_id_b = :secondary_id");
    query.bindValue(":secondary_id", secondaryMediaId);

    return query.exec();
}

/**
 * @brief Removes all secondary links associated with a primary media file.
 * @param primaryMediaId The ID of the primary media file.
 * @return True if the relations were deleted successfully, false otherwise.
 */
bool SqliteFileRelationRepository::deleteRelationsForPrimary(qlonglong primaryMediaId) {
    if (primaryMediaId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("DELETE FROM file_relations WHERE file_id_a = :primary_id");
    query.bindValue(":primary_id", primaryMediaId);

    return query.exec();
}

/**
 * @brief Retrieves all secondary media files linked to a primary media file.
 * @param primaryMediaId The ID of the primary media file.
 * @return A list of all linked MediaFile objects.
 */
QList<MediaFile> SqliteFileRelationRepository::getLinkedMedia(qlonglong primaryMediaId) {
    QList<MediaFile> files;

    if (primaryMediaId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return files;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT mf.id, mf.song_id, mf.file_path, mf.file_type, mf.media_kind, "
                  "mf.file_size, mf.file_hash, mf.source_type, mf.is_managed, mf.can_be_practiced, "
                  "mf.import_root, mf.source_relative_path, mf.has_video "
                  "FROM file_relations fr "
                  "INNER JOIN media_files mf ON mf.id = fr.file_id_b "
                  "WHERE fr.file_id_a = :primary_id "
                  "ORDER BY mf.source_relative_path, mf.file_path");
    query.bindValue(":primary_id", primaryMediaId);

    if (!query.exec()) {
        qCritical() << "[SqliteFileRelationRepository] getLinkedMedia failed:"
                    << query.lastError().text();
        return files;
    }

    while (query.next()) {
        files.append(mapMediaRow(query));
    }

    return files;
}

/**
 * @brief Checks if a specific media file is a secondary file in any relation.
 * @param mediaId The ID of the media file to check.
 * @return True if it is a secondary file, false otherwise.
 */
bool SqliteFileRelationRepository::isSecondaryMedia(qlonglong mediaId) {
    if (mediaId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT 1 FROM file_relations WHERE file_id_b = :media_id LIMIT 1");
    query.bindValue(":media_id", mediaId);

    return query.exec() && query.next();
}

/**
 * @brief Finds the primary media ID for a given secondary media ID.
 * @param mediaId The ID of the secondary media file.
 * @return The ID of the primary media, or nullopt if no relation exists.
 */
std::optional<qlonglong> SqliteFileRelationRepository::getPrimaryMediaId(qlonglong mediaId) {
    if (mediaId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT file_id_a FROM file_relations WHERE file_id_b = :media_id LIMIT 1");
    query.bindValue(":media_id", mediaId);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    const qlonglong primaryId =
        query.value(SqlQueryColumns::FileRelation::PrimaryMediaId).toLongLong();
    return primaryId > 0 ? std::optional<qlonglong>(primaryId) : std::nullopt;
}

/**
 * @brief Gets a list of all current primary-secondary relation pairs.
 * @return A list of pairs containing primary and secondary IDs.
 */
QList<std::pair<qlonglong, qlonglong>> SqliteFileRelationRepository::getAllRelationPairs() {
    QList<std::pair<qlonglong, qlonglong>> pairs;

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return pairs;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    if (!query.exec(QStringLiteral("SELECT file_id_a, file_id_b FROM file_relations"))) {
        qCritical() << "[SqliteFileRelationRepository] getAllRelationPairs failed:"
                    << query.lastError().text();
        return pairs;
    }

    while (query.next()) {
        const qlonglong primaryId = query.value(0).toLongLong();
        const qlonglong secondaryId = query.value(1).toLongLong();
        if (primaryId > 0 && secondaryId > 0) {
            pairs.emplace_back(primaryId, secondaryId);
        }
    }

    return pairs;
}

/**
 * @brief Checks if a media file is already part of any link group or relation.
 * @param mediaId The ID of the media file to check.
 * @return True if the media file is currently in use, false otherwise.
 */
bool SqliteFileRelationRepository::isMediaInAnyGroup(qlonglong mediaId) {
    if (mediaId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    if (isSecondaryMedia(mediaId)) {
        return true;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT 1 FROM link_groups WHERE primary_media_id = :media_id LIMIT 1");
    query.bindValue(":media_id", mediaId);

    if (query.exec() && query.next()) {
        return true;
    }

    query.prepare("SELECT 1 FROM file_relations WHERE file_id_a = :media_id LIMIT 1");
    query.bindValue(":media_id", mediaId);

    return query.exec() && query.next();
}

/**
 * @brief Maps a raw database query row into a MediaFile object.
 * @param query The active query object at the current record.
 * @return The populated MediaFile object.
 */
MediaFile SqliteFileRelationRepository::mapMediaRow(QSqlQuery &query) {
    MediaFile mediaFile;
    mediaFile.id = query.value(SqlQueryColumns::MediaFile::Id).toLongLong();
    mediaFile.songId = query.value(SqlQueryColumns::MediaFile::SongId).toLongLong();
    mediaFile.filePath = query.value(SqlQueryColumns::MediaFile::FilePath).toString();
    mediaFile.fileType = query.value(SqlQueryColumns::MediaFile::FileType).toString();
    mediaFile.mediaKind = MediaFileMapping::mediaKindFromString(
        query.value(SqlQueryColumns::MediaFile::MediaKind).toString());
    mediaFile.fileSize = query.value(SqlQueryColumns::MediaFile::FileSize).toLongLong();
    mediaFile.fileHash = query.value(SqlQueryColumns::MediaFile::FileHash).toString();
    mediaFile.sourceType = MediaFileMapping::sourceTypeFromString(
        query.value(SqlQueryColumns::MediaFile::SourceType).toString());
    mediaFile.isManaged = query.value(SqlQueryColumns::MediaFile::IsManaged).toInt() != 0;
    mediaFile.canBePracticed = query.value(SqlQueryColumns::MediaFile::CanBePracticed).toInt() != 0;
    mediaFile.importRoot = query.value(SqlQueryColumns::MediaFile::ImportRoot).toString();
    mediaFile.sourceRelativePath =
        query.value(SqlQueryColumns::MediaFile::SourceRelativePath).toString();
    mediaFile.hasVideo = query.value(SqlQueryColumns::MediaFile::HasVideo).toInt() != 0;
    mediaFile.hasAudio = query.value(SqlQueryColumns::MediaFile::HasAudio).toInt() != 0;
    return mediaFile;
}
