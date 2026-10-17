#include "SqliteMediaFileRepository.h"

#include "MediaFileMapping.h"
#include "RepositoryUtils.h"
#include "SqlQueryColumns.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

/**
 * @brief Constructs the repository with a database connection.
 * @param connection The database connection object to use.
 */
SqliteMediaFileRepository::SqliteMediaFileRepository(IDatabaseConnection &connection)
    : m_connection(connection) {}

/**
 * @brief Saves a new media file record to the database.
 * @param mediaFile The MediaFile object containing file metadata.
 * @return The ID of the created media file, or nullopt if it failed.
 */
std::optional<qlonglong> SqliteMediaFileRepository::createMediaFile(const MediaFile &mediaFile) {
    if (mediaFile.filePath.trimmed().isEmpty() || mediaFile.songId <= 0) {
        return std::nullopt;
    }

    if (mediaFile.sourceType == MediaSourceType::Local && mediaFile.isManaged &&
        mediaFile.fileHash.trimmed().isEmpty()) {
        return std::nullopt;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(
        "INSERT INTO media_files "
        "(song_id, file_path, file_type, media_kind, file_size, file_hash, "
        "source_type, is_managed, can_be_practiced, import_root, source_relative_path, has_video, "
        "has_audio) "
        "VALUES (:song_id, :file_path, :file_type, :media_kind, :file_size, :file_hash, "
        ":source_type, :is_managed, :can_be_practiced, :import_root, :source_relative_path, "
        ":has_video, :has_audio)");
    query.bindValue(":song_id", mediaFile.songId);
    query.bindValue(":file_path", mediaFile.filePath);
    query.bindValue(":file_type", mediaFile.fileType);
    query.bindValue(":media_kind", MediaFileMapping::mediaKindToString(mediaFile.mediaKind));
    query.bindValue(":file_size", mediaFile.fileSize);
    query.bindValue(":file_hash", mediaFile.fileHash.isEmpty() ? QVariant() : mediaFile.fileHash);
    query.bindValue(":source_type", MediaFileMapping::sourceTypeToString(mediaFile.sourceType));
    query.bindValue(":is_managed", mediaFile.isManaged ? 1 : 0);
    query.bindValue(":can_be_practiced", mediaFile.canBePracticed ? 1 : 0);
    query.bindValue(":import_root",
                    mediaFile.importRoot.isEmpty() ? QVariant() : mediaFile.importRoot);
    query.bindValue(":source_relative_path", mediaFile.sourceRelativePath.isEmpty()
                                                 ? QVariant()
                                                 : mediaFile.sourceRelativePath);
    query.bindValue(":has_video", mediaFile.hasVideo ? 1 : 0);
    query.bindValue(":has_audio", mediaFile.hasAudio ? 1 : 0);

    if (!query.exec()) {
        qCritical() << "[SqliteMediaFileRepository] createMediaFile failed:"
                    << query.lastError().text();
        return std::nullopt;
    }

    if (query.numRowsAffected() != 1) {
        return std::nullopt;
    }

    const qlonglong newId = query.lastInsertId().toLongLong();
    if (newId <= 0) {
        return std::nullopt;
    }

    return newId;
}

/**
 * @brief Retrieves a media file record by its unique database ID.
 * @param id The unique ID of the media file.
 * @return The MediaFile object, or nullopt if not found.
 */
std::optional<MediaFile> SqliteMediaFileRepository::getMediaFile(qlonglong id) {
    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, song_id, file_path, file_type, media_kind, file_size, file_hash, "
                  "source_type, is_managed, can_be_practiced, import_root, source_relative_path, "
                  "has_video, has_audio "
                  "FROM media_files WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    return mapRow(query);
}

/**
 * @brief Finds a media file record using its unique file hash.
 * @param fileHash The string hash identifying the file.
 * @return The MediaFile object, or nullopt if not found.
 */
std::optional<MediaFile> SqliteMediaFileRepository::findByHash(const QString &fileHash) {
    if (fileHash.trimmed().isEmpty()) {
        return std::nullopt;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, song_id, file_path, file_type, media_kind, file_size, file_hash, "
                  "source_type, is_managed, can_be_practiced, import_root, source_relative_path, "
                  "has_video, has_audio "
                  "FROM media_files WHERE file_hash = :file_hash");
    query.bindValue(":file_hash", fileHash);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    return mapRow(query);
}

/**
 * @brief Finds a media file record using its file path.
 * @param filePath The path string to search for.
 * @return The MediaFile object, or nullopt if not found.
 */
std::optional<MediaFile> SqliteMediaFileRepository::findByPath(const QString &filePath) {
    if (filePath.trimmed().isEmpty()) {
        return std::nullopt;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, song_id, file_path, file_type, media_kind, file_size, file_hash, "
                  "source_type, is_managed, can_be_practiced, import_root, source_relative_path, "
                  "has_video, has_audio "
                  "FROM media_files WHERE file_path = :file_path");
    query.bindValue(":file_path", filePath);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    return mapRow(query);
}

/**
 * @brief Retrieves all media files associated with a specific song.
 * @param songId The ID of the song.
 * @return A list of MediaFile objects linked to the song.
 */
QList<MediaFile> SqliteMediaFileRepository::getMediaFilesBySongId(qlonglong songId) {
    QList<MediaFile> files;

    if (songId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return files;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, song_id, file_path, file_type, media_kind, file_size, file_hash, "
                  "source_type, is_managed, can_be_practiced, import_root, source_relative_path, "
                  "has_video, has_audio "
                  "FROM media_files WHERE song_id = :song_id ORDER BY id");
    query.bindValue(":song_id", songId);

    if (!query.exec()) {
        qCritical() << "[SqliteMediaFileRepository] getMediaFilesBySongId failed:"
                    << query.lastError().text();
        return files;
    }

    while (query.next()) {
        files.append(mapRow(query));
    }

    return files;
}

/**
 * @brief Retrieves every media file record stored in the database.
 * @return A list containing all MediaFile objects.
 */
QList<MediaFile> SqliteMediaFileRepository::getAllMediaFiles() {
    QList<MediaFile> files;

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return files;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    const QString sql = QStringLiteral(
        "SELECT id, song_id, file_path, file_type, media_kind, file_size, file_hash, "
        "source_type, is_managed, can_be_practiced, import_root, source_relative_path, has_video, "
        "has_audio "
        "FROM media_files ORDER BY song_id, id");

    if (!query.exec(sql)) {
        qCritical() << "[SqliteMediaFileRepository] getAllMediaFiles failed:"
                    << query.lastError().text();
        return files;
    }

    while (query.next()) {
        files.append(mapRow(query));
    }

    return files;
}

/**
 * @brief Fetches the first media ID for each song in a provided list.
 * @param songIds A list of song IDs to look up.
 * @return A hash mapping each song ID to its first associated media ID.
 */
QHash<qlonglong, qlonglong>
SqliteMediaFileRepository::firstMediaIdBySongIds(const QList<qlonglong> &songIds) {
    QHash<qlonglong, qlonglong> mediaBySong;

    if (songIds.isEmpty() || !RepositoryUtils::ensureOpen(m_connection)) {
        return mediaBySong;
    }

    QStringList placeholders;
    QVariantList bindValues;
    placeholders.reserve(songIds.size());
    bindValues.reserve(songIds.size());

    for (const qlonglong songId : songIds) {
        if (songId <= 0 || mediaBySong.contains(songId)) {
            continue;
        }
        placeholders.append(QStringLiteral("?"));
        bindValues.append(songId);
    }

    if (bindValues.isEmpty()) {
        return mediaBySong;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(QStringLiteral("SELECT id, song_id FROM media_files "
                                 "WHERE song_id IN (%1) ORDER BY song_id, id")
                      .arg(placeholders.join(QLatin1Char(','))));

    for (const QVariant &value : bindValues) {
        query.addBindValue(value);
    }

    if (!query.exec()) {
        qCritical() << "[SqliteMediaFileRepository] firstMediaIdBySongIds failed:"
                    << query.lastError().text();
        return mediaBySong;
    }

    while (query.next()) {
        const qlonglong songId = query.value(1).toLongLong();
        if (!mediaBySong.contains(songId)) {
            mediaBySong.insert(songId, query.value(0).toLongLong());
        }
    }

    return mediaBySong;
}

/**
 * @brief Maps a single database query row to a MediaFile object.
 * @param query The active query object at the current record.
 * @return A populated MediaFile object.
 */
MediaFile SqliteMediaFileRepository::mapRow(QSqlQuery &query) {
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
