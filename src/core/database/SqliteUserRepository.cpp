#include "SqliteUserRepository.h"

#include "RepositoryUtils.h"
#include "SqlQueryColumns.h"

#include <QSqlError>
#include <QSqlQuery>

/**
 * @brief Constructs the repository with a database connection.
 * @param connection The database connection object to use.
 */
SqliteUserRepository::SqliteUserRepository(IDatabaseConnection &connection)
    : m_connection(connection) {}

/**
 * @brief Adds a new user to the database.
 * @param user The User object containing name and role.
 * @return The ID of the new user if successful, or nullopt if it failed.
 */
std::optional<qlonglong> SqliteUserRepository::createUser(const User &user) {
    if (user.name.trimmed().isEmpty()) {
        return std::nullopt;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("INSERT OR IGNORE INTO users (name, role) VALUES (:name, :role)");
    query.bindValue(":name", user.name);
    query.bindValue(":role", user.role);

    if (!query.exec()) {
        qCritical() << "[SqliteUserRepository] createUser failed:" << query.lastError().text();
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
 * @brief Retrieves a user from the database by their ID.
 * @param id The unique ID of the user.
 * @return The User object if found, or nullopt if not found.
 */
std::optional<User> SqliteUserRepository::getUser(qlonglong id) {
    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, name, role FROM users WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    User loadedUser;
    loadedUser.id = query.value(SqlQueryColumns::User::Id).toLongLong();
    loadedUser.name = query.value(SqlQueryColumns::User::Name).toString();
    loadedUser.role = query.value(SqlQueryColumns::User::Role).toString();
    return loadedUser;
}

/**
 * @brief Updates an existing user's information in the database.
 * @param user The User object containing the updated name and role.
 * @return True if the update was successful, false otherwise.
 */
bool SqliteUserRepository::updateUser(const User &user) {
    if (user.id <= 0 || user.name.trimmed().isEmpty()) {
        return false;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("UPDATE users SET name = :name, role = :role WHERE id = :id");
    query.bindValue(":name", user.name);
    query.bindValue(":role", user.role);
    query.bindValue(":id", user.id);

    if (!query.exec()) {
        qCritical() << "[SqliteUserRepository] updateUser failed:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}
