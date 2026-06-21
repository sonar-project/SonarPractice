#include "SqliteArtistRepository.h"

#include "RepositoryUtils.h"
#include "SqlQueryColumns.h"

#include <QSqlError>
#include <QSqlQuery>

/**
 * @brief Constructs the repository with a database connection.
 * @param connection The database connection object to use.
 */
SqliteArtistRepository::SqliteArtistRepository(IDatabaseConnection &connection)
    : m_connection(connection) {}

/**
 * @brief Adds a new artist to the database.
 * @param artist The artist object to create.
 * @return The ID of the new artist if successful, or nullopt if it failed.
 */
std::optional<qlonglong> SqliteArtistRepository::createArtist(const Artist &artist) {
    if (artist.name.trimmed().isEmpty()) {
        return std::nullopt;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("INSERT OR IGNORE INTO artists (name) VALUES (:name)");
    query.bindValue(":name", artist.name);

    if (!query.exec()) {
        qCritical() << "[SqliteArtistRepository] createArtist failed:" << query.lastError().text();
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
 * @brief Retrieves an artist from the database by their ID.
 * @param id The unique ID of the artist.
 * @return The artist object if found, or nullopt if not found.
 */
std::optional<Artist> SqliteArtistRepository::getArtist(qlonglong id) {
    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, name FROM artists WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Artist loadedArtist;
    loadedArtist.id = query.value(SqlQueryColumns::Artist::Id).toLongLong();
    loadedArtist.name = query.value(SqlQueryColumns::Artist::Name).toString();
    return loadedArtist;
}

/**
 * @brief Retrieves a list of all artists sorted by name.
 * @return A list containing all artist objects from the database.
 */
QList<Artist> SqliteArtistRepository::getAllArtists() {
    QList<Artist> artists;

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return artists;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    if (!query.exec(QStringLiteral("SELECT id, name FROM artists ORDER BY name"))) {
        qCritical() << "[SqliteArtistRepository] getAllArtists failed:" << query.lastError().text();
        return artists;
    }

    while (query.next()) {
        Artist artist;
        artist.id = query.value(SqlQueryColumns::Artist::Id).toLongLong();
        artist.name = query.value(SqlQueryColumns::Artist::Name).toString();
        artists.append(artist);
    }

    return artists;
}

/**
 * @brief Searches for an artist in the database by their name.
 * @param name The name of the artist to search for.
 * @return The artist object if found, or nullopt if not found.
 */
std::optional<Artist> SqliteArtistRepository::findArtistByName(const QString &name) {
    if (name.trimmed().isEmpty()) {
        return std::nullopt;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, name FROM artists WHERE name = :name");
    query.bindValue(":name", name);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Artist loadedArtist;
    loadedArtist.id = query.value(SqlQueryColumns::Artist::Id).toLongLong();
    loadedArtist.name = query.value(SqlQueryColumns::Artist::Name).toString();
    return loadedArtist;
}

/**
 * @brief Updates the name of an existing artist in the database.
 * @param artist The artist object containing the updated information.
 * @return True if the update was successful, false otherwise.
 */
bool SqliteArtistRepository::updateArtist(const Artist &artist) {
    if (artist.id <= 0 || artist.name.trimmed().isEmpty()) {
        return false;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("UPDATE artists SET name = :name WHERE id = :id");
    query.bindValue(":name", artist.name);
    query.bindValue(":id", artist.id);

    if (!query.exec()) {
        qCritical() << "[SqliteArtistRepository] updateArtist failed:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

/**
 * @brief Removes an artist from the database using their ID.
 * @param id The unique ID of the artist to delete.
 * @return True if the artist was deleted successfully, false otherwise.
 */
bool SqliteArtistRepository::deleteArtist(qlonglong id) {
    if (id <= 0) {
        return false;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("DELETE FROM artists WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qCritical() << "[SqliteArtistRepository] deleteArtist failed:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}
