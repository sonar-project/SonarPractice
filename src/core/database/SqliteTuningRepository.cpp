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
 * @brief Retrieves a tuning from the database by its ID.
 * @param id The unique ID of the tuning.
 * @return The Tuning object if found, or nullopt if not found.
 */
std::optional<Tuning> SqliteTuningRepository::getTuning(qlonglong id) {
    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, name FROM tunings WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    Tuning loadedTuning;
    loadedTuning.id = query.value(SqlQueryColumns::Tuning::Id).toLongLong();
    loadedTuning.name = query.value(SqlQueryColumns::Tuning::Name).toString();
    return loadedTuning;
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

QList<Tuning> SqliteTuningRepository::listAllTunings() {
    QList<Tuning> tunings;
    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return tunings;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    if (!query.exec(QStringLiteral("SELECT id, name FROM tunings ORDER BY name COLLATE NOCASE"))) {
        qCritical() << "[SqliteTuningRepository] listAllTunings failed:" << query.lastError().text();
        return tunings;
    }

    while (query.next()) {
        Tuning tuning;
        tuning.id = query.value(SqlQueryColumns::Tuning::Id).toLongLong();
        tuning.name = query.value(SqlQueryColumns::Tuning::Name).toString();
        tunings.append(tuning);
    }
    return tunings;
}

/**
 * @brief Updates the name of an existing tuning in the database.
 * @param tuning The Tuning object containing the updated information.
 * @return True if the update was successful, false otherwise.
 */
bool SqliteTuningRepository::updateTuning(const Tuning &tuning) {
    if (tuning.id <= 0 || tuning.name.trimmed().isEmpty()) {
        return false;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("UPDATE tunings SET name = :name WHERE id = :id");
    query.bindValue(":name", tuning.name);
    query.bindValue(":id", tuning.id);

    if (!query.exec()) {
        qCritical() << "[SqliteTuningRepository] updateTuning failed:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

/**
 * @brief Removes a tuning from the database using its ID.
 * @param id The unique ID of the tuning to delete.
 * @return True if the tuning was deleted successfully, false otherwise.
 */
bool SqliteTuningRepository::deleteTuning(qlonglong id) {
    if (id <= 0) {
        return false;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("DELETE FROM tunings WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qCritical() << "[SqliteTuningRepository] deleteTuning failed:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}
