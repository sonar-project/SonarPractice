#include "SqliteLinkGroupRepository.h"

#include "RepositoryUtils.h"
#include "SqlQueryColumns.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

/**
 * @brief Constructs the repository with a database connection.
 * @param connection The database connection object to use.
 */
SqliteLinkGroupRepository::SqliteLinkGroupRepository(IDatabaseConnection &connection)
    : m_connection(connection) {}

/**
 * @brief Creates a new link group in the database.
 * @param group The LinkGroup object containing group details.
 * @return The ID of the new link group, or nullopt if it failed.
 */
std::optional<qlonglong> SqliteLinkGroupRepository::createGroup(const LinkGroup &group) {
    if (group.title.trimmed().isEmpty() || group.primarySongId <= 0 || group.primaryMediaId <= 0) {
        return std::nullopt;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("INSERT INTO link_groups (title, primary_song_id, primary_media_id) "
                  "VALUES (:title, :primary_song_id, :primary_media_id)");
    query.bindValue(":title", group.title.trimmed());
    query.bindValue(":primary_song_id", group.primarySongId);
    query.bindValue(":primary_media_id", group.primaryMediaId);

    if (!query.exec() || query.numRowsAffected() != 1) {
        qCritical() << "[SqliteLinkGroupRepository] createGroup failed:"
                    << query.lastError().text();
        return std::nullopt;
    }

    const qlonglong newId = query.lastInsertId().toLongLong();
    return newId > 0 ? std::optional<qlonglong>(newId) : std::nullopt;
}

/**
 * @brief Retrieves a link group by its unique ID.
 * @param groupId The ID of the group.
 * @return The LinkGroup object, or nullopt if not found.
 */
std::optional<LinkGroup> SqliteLinkGroupRepository::getGroup(qlonglong groupId) {
    if (groupId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, title, primary_song_id, primary_media_id, created_at "
                  "FROM link_groups WHERE id = :id");
    query.bindValue(":id", groupId);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    return mapRow(query);
}

/**
 * @brief Finds a link group associated with a specific primary song.
 * @param primarySongId The ID of the primary song.
 * @return The LinkGroup object, or nullopt if not found.
 */
std::optional<LinkGroup> SqliteLinkGroupRepository::getGroupByPrimarySong(qlonglong primarySongId) {
    if (primarySongId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, title, primary_song_id, primary_media_id, created_at "
                  "FROM link_groups WHERE primary_song_id = :primary_song_id");
    query.bindValue(":primary_song_id", primarySongId);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    return mapRow(query);
}

/**
 * @brief Finds a link group associated with a specific primary media file.
 * @param primaryMediaId The ID of the primary media file.
 * @return The LinkGroup object, or nullopt if not found.
 */
std::optional<LinkGroup>
SqliteLinkGroupRepository::getGroupByPrimaryMedia(qlonglong primaryMediaId) {
    if (primaryMediaId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, title, primary_song_id, primary_media_id, created_at "
                  "FROM link_groups WHERE primary_media_id = :primary_media_id");
    query.bindValue(":primary_media_id", primaryMediaId);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    return mapRow(query);
}

/**
 * @brief Finds the link group that contains a given secondary media file.
 * @param secondaryMediaId The ID of the secondary media file.
 * @return The LinkGroup object, or nullopt if not found.
 */
std::optional<LinkGroup>
SqliteLinkGroupRepository::getGroupForSecondaryMedia(qlonglong secondaryMediaId) {
    if (secondaryMediaId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT lg.id, lg.title, lg.primary_song_id, lg.primary_media_id, lg.created_at "
                  "FROM link_groups lg "
                  "INNER JOIN file_relations fr ON fr.file_id_a = lg.primary_media_id "
                  "WHERE fr.file_id_b = :media_id");
    query.bindValue(":media_id", secondaryMediaId);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    return mapRow(query);
}

/**
 * @brief Updates the title of an existing link group.
 * @param groupId The ID of the group to update.
 * @param title The new title for the group.
 * @return True if the update was successful, false otherwise.
 */
bool SqliteLinkGroupRepository::updateTitle(qlonglong groupId, const QString &title) {
    if (groupId <= 0 || title.trimmed().isEmpty() || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("UPDATE link_groups SET title = :title WHERE id = :id");
    query.bindValue(":title", title.trimmed());
    query.bindValue(":id", groupId);

    return query.exec() && query.numRowsAffected() == 1;
}

/**
 * @brief Deletes a link group from the database.
 * @param groupId The ID of the group to delete.
 * @return True if the group was deleted, false otherwise.
 */
bool SqliteLinkGroupRepository::deleteGroup(qlonglong groupId) {
    if (groupId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("DELETE FROM link_groups WHERE id = :id");
    query.bindValue(":id", groupId);

    return query.exec();
}

/**
 * @brief Retrieves a list of all link groups, sorted by title.
 * @return A list of all existing LinkGroup objects.
 */
QList<LinkGroup> SqliteLinkGroupRepository::getAllGroups() {
    QList<LinkGroup> groups;

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return groups;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    if (!query.exec("SELECT id, title, primary_song_id, primary_media_id, created_at "
                    "FROM link_groups ORDER BY title")) {
        return groups;
    }

    while (query.next()) {
        groups.append(mapRow(query));
    }

    return groups;
}

/**
 * @brief Maps a single database query row to a LinkGroup object.
 * @param query The active query object at the current record.
 * @return A populated LinkGroup object.
 */
LinkGroup SqliteLinkGroupRepository::mapRow(QSqlQuery &query) {
    LinkGroup group;
    group.id = query.value(SqlQueryColumns::LinkGroup::Id).toLongLong();
    group.title = query.value(SqlQueryColumns::LinkGroup::Title).toString();
    group.primarySongId = query.value(SqlQueryColumns::LinkGroup::PrimarySongId).toLongLong();
    group.primaryMediaId = query.value(SqlQueryColumns::LinkGroup::PrimaryMediaId).toLongLong();
    group.createdAt = query.value(SqlQueryColumns::LinkGroup::CreatedAt).toString();
    return group;
}
