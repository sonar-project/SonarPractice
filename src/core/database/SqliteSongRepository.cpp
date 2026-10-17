#include "SqliteSongRepository.h"

#include "RepositoryUtils.h"
#include "SqlQueryColumns.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

SqliteSongRepository::SqliteSongRepository(IDatabaseConnection &connection)
    : m_connection(connection) {}

std::optional<qlonglong> SqliteSongRepository::createSong(const Song &song) {
    if (song.title.trimmed().isEmpty()) {
        return std::nullopt;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("INSERT INTO songs (title, artist_id, tuning_id, base_bpm) "
                  "VALUES (:title, :artist_id, :tuning_id, :base_bpm)");
    query.bindValue(":title", song.title);
    query.bindValue(":artist_id", song.artistId > 0 ? QVariant(song.artistId) : QVariant());
    query.bindValue(":tuning_id", song.tuningId > 0 ? QVariant(song.tuningId) : QVariant());
    query.bindValue(":base_bpm", song.baseBpm);

    if (!query.exec()) {
        qCritical() << "[SqliteSongRepository] createSong failed:" << query.lastError().text();
        return std::nullopt;
    }

    const qlonglong newId = query.lastInsertId().toLongLong();
    if (newId <= 0) {
        return std::nullopt;
    }

    return newId;
}

std::optional<Song> SqliteSongRepository::getSong(qlonglong id) {
    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));

    QString sql = "SELECT s.id, s.title, s.base_bpm, s.artist_id, s.tuning_id, t.name "
                  "FROM songs s "
                  "LEFT JOIN tunings t ON s.tuning_id = t.id "
                  "WHERE s.id = :id";
    query.prepare(sql);
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Song loadedSong;
    loadedSong.id = query.value(SqlQueryColumns::Song::Id).toLongLong();
    loadedSong.title = query.value(SqlQueryColumns::Song::Title).toString();
    loadedSong.baseBpm = query.value(SqlQueryColumns::Song::BaseBpm).toInt();
    loadedSong.artistId = query.value(SqlQueryColumns::Song::ArtistId).toLongLong();
    loadedSong.tuningId = query.value(SqlQueryColumns::Song::TuningId).toLongLong();
    loadedSong.tuningName = query.value(SqlQueryColumns::Song::TuningName).toString();
    return loadedSong;
}

QList<Song> SqliteSongRepository::getAllSongs() {
    QList<Song> songs;

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return songs;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    QString sql = "SELECT s.id, s.title, s.base_bpm, s.artist_id, s.tuning_id, t.name "
                  "FROM songs s "
                  "LEFT JOIN tunings t ON s.tuning_id = t.id "
                  "ORDER BY s.id";

    if (!query.exec(sql)) {
        qCritical() << "[SqliteSongRepository] getAllSongs failed:" << query.lastError().text();
        return songs;
    }

    while (query.next()) {
        Song song;
        song.id = query.value(SqlQueryColumns::Song::Id).toLongLong();
        song.title = query.value(SqlQueryColumns::Song::Title).toString();
        song.baseBpm = query.value(SqlQueryColumns::Song::BaseBpm).toInt();
        song.artistId = query.value(SqlQueryColumns::Song::ArtistId).toLongLong();
        song.tuningId = query.value(SqlQueryColumns::Song::TuningId).toLongLong();
        song.tuningName = query.value(SqlQueryColumns::Song::TuningName).toString();
        songs.append(song);
    }

    return songs;
}

bool SqliteSongRepository::updateSong(const Song &song) {
    if (song.id <= 0 || song.title.trimmed().isEmpty()) {
        return false;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("UPDATE songs SET title = :title, artist_id = :artist_id, "
                  "tuning_id = :tuning_id, base_bpm = :base_bpm WHERE id = :id");
    query.bindValue(":title", song.title);
    query.bindValue(":artist_id", song.artistId > 0 ? QVariant(song.artistId) : QVariant());
    query.bindValue(":tuning_id", song.tuningId > 0 ? QVariant(song.tuningId) : QVariant());
    query.bindValue(":base_bpm", song.baseBpm);
    query.bindValue(":id", song.id);

    if (!query.exec()) {
        qCritical() << "[SqliteSongRepository] updateSong failed:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool SqliteSongRepository::deleteSong(qlonglong id) {
    if (id <= 0) {
        return false;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("DELETE FROM songs WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qCritical() << "[SqliteSongRepository] deleteSong failed:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}
