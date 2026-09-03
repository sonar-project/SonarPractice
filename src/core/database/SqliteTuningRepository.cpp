#include "SqliteTuningRepository.h"

#include "RepositoryUtils.h"
#include "SqlQueryColumns.h"

#include <QSqlError>
#include <QSqlQuery>

/**
 * @brief Constructs the repository with a database connection.
 * @param connection The database connection object to use.
 */
SqliteTuningRepository::SqliteTuningRepository(IDatabaseConnection &connection)
    : m_connection(connection) {}

/**
 * @brief Adds a new guitar tuning to the database.
 * @param tuning The Tuning object to create.
 * @return The ID of the new tuning if successful, or nullopt if it failed.
 */
std::optional<qlonglong> SqliteTuningRepository::createTuning(const Tuning &tuning) {
    if (tuning.name.trimmed().isEmpty()) {
        return std::nullopt;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("INSERT OR IGNORE INTO tunings (name) VALUES (:name)");
    query.bindValue(":name", tuning.name);

    if (!query.exec()) {
        qCritical() << "[SqliteTuningRepository] createTuning failed:" << query.lastError().text();
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
 * @brief Searches for a tuning in the database by its name.
 * @param name The name of the tuning to search for.
 * @return The Tuning object if found, or nullopt if not found.
 */
std::optional<Tuning> SqliteTuningRepository::findTuningByName(const QString &name) {
    if (name.trimmed().isEmpty()) {
        return std::nullopt;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, name FROM tunings WHERE name = :name");
    query.bindValue(":name", name);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Tuning loadedTuning;
    loadedTuning.id = query.value(SqlQueryColumns::Tuning::Id).toLongLong();
    loadedTuning.name = query.value(SqlQueryColumns::Tuning::Name).toString();
    return loadedTuning;
}
