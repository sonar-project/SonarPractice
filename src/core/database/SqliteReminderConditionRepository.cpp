#include "SqliteReminderConditionRepository.h"

#include "RepositoryUtils.h"
#include "SqlQueryColumns.h"

#include <QSqlError>
#include <QSqlQuery>

/**
 * @brief Constructs the repository with a database connection.
 * @param connection The database connection object to use.
 */
SqliteReminderConditionRepository::SqliteReminderConditionRepository(
    IDatabaseConnection &connection)
    : m_connection(connection) {}

/**
 * @brief Converts the current row of a database query into a ReminderCondition object.
 * @param query The active query object positioned at the desired record.
 * @return A populated ReminderCondition object.
 */
ReminderCondition SqliteReminderConditionRepository::conditionFromQuery(const QSqlQuery &query) {
    ReminderCondition condition;
    condition.id = query.value(SqlQueryColumns::ReminderCondition::Id).toLongLong();
    condition.reminderId = query.value(SqlQueryColumns::ReminderCondition::ReminderId).toLongLong();
    condition.startBar = query.value(SqlQueryColumns::ReminderCondition::StartBar).toInt();
    condition.endBar = query.value(SqlQueryColumns::ReminderCondition::EndBar).toInt();
    condition.minBpm = query.value(SqlQueryColumns::ReminderCondition::MinBpm).toInt();
    condition.minMinutes = query.value(SqlQueryColumns::ReminderCondition::MinMinutes).toInt();
    return condition;
}

/**
 * @brief Saves a new reminder condition to the database.
 * @param condition The ReminderCondition object to store.
 * @return The ID of the newly created condition, or nullopt if it failed.
 */
std::optional<qlonglong>
SqliteReminderConditionRepository::createCondition(const ReminderCondition &condition) {
    if (condition.reminderId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("INSERT INTO reminder_conditions "
                  "(reminder_id, start_bar, end_bar, min_bpm, min_minutes) "
                  "VALUES (:reminder_id, :start_bar, :end_bar, :min_bpm, :min_minutes)");
    query.bindValue(QStringLiteral(":reminder_id"), condition.reminderId);
    query.bindValue(QStringLiteral(":start_bar"), condition.startBar);
    query.bindValue(QStringLiteral(":end_bar"), condition.endBar);
    query.bindValue(QStringLiteral(":min_bpm"), condition.minBpm);
    query.bindValue(QStringLiteral(":min_minutes"), condition.minMinutes);

    if (!query.exec()) {
        qCritical() << "[SqliteReminderConditionRepository] createCondition failed:"
                    << query.lastError().text();
        return std::nullopt;
    }

    const qlonglong newId = query.lastInsertId().toLongLong();
    return newId > 0 ? std::optional<qlonglong>(newId) : std::nullopt;
}

/**
 * @brief Retrieves a specific reminder condition from the database by its ID.
 * @param id The unique ID of the condition.
 * @return The ReminderCondition object, or nullopt if not found.
 */
std::optional<ReminderCondition> SqliteReminderConditionRepository::getCondition(qlonglong id) {
    if (id <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, reminder_id, start_bar, end_bar, min_bpm, min_minutes "
                  "FROM reminder_conditions WHERE id = :id");
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    return conditionFromQuery(query);
}

/**
 * @brief Updates an existing reminder condition with new values.
 * @param condition The condition object containing the updated information.
 * @return True if the update was successful, false otherwise.
 */
bool SqliteReminderConditionRepository::updateCondition(const ReminderCondition &condition) {
    if (condition.id <= 0 || condition.reminderId <= 0 ||
        !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(
        "UPDATE reminder_conditions SET reminder_id = :reminder_id, start_bar = :start_bar, "
        "end_bar = :end_bar, min_bpm = :min_bpm, min_minutes = :min_minutes WHERE id = :id");
    query.bindValue(QStringLiteral(":id"), condition.id);
    query.bindValue(QStringLiteral(":reminder_id"), condition.reminderId);
    query.bindValue(QStringLiteral(":start_bar"), condition.startBar);
    query.bindValue(QStringLiteral(":end_bar"), condition.endBar);
    query.bindValue(QStringLiteral(":min_bpm"), condition.minBpm);
    query.bindValue(QStringLiteral(":min_minutes"), condition.minMinutes);

    if (!query.exec()) {
        qCritical() << "[SqliteReminderConditionRepository] updateCondition failed:"
                    << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

/**
 * @brief Removes a reminder condition from the database by its ID.
 * @param id The unique ID of the condition to delete.
 * @return True if the deletion was successful, false otherwise.
 */
bool SqliteReminderConditionRepository::deleteCondition(qlonglong id) {
    if (id <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(QStringLiteral("DELETE FROM reminder_conditions WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qCritical() << "[SqliteReminderConditionRepository] deleteCondition failed:"
                    << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

/**
 * @brief Removes all conditions associated with a specific reminder.
 * @param reminderId The ID of the reminder to clear.
 * @return True if the conditions were deleted successfully, false otherwise.
 */
bool SqliteReminderConditionRepository::deleteConditionsForReminder(qlonglong reminderId) {
    if (reminderId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(
        QStringLiteral("DELETE FROM reminder_conditions WHERE reminder_id = :reminder_id"));
    query.bindValue(QStringLiteral(":reminder_id"), reminderId);

    if (!query.exec()) {
        qCritical() << "[SqliteReminderConditionRepository] deleteConditionsForReminder failed:"
                    << query.lastError().text();
        return false;
    }

    return true;
}

/**
 * @brief Lists all conditions linked to a specific reminder.
 * @param reminderId The ID of the parent reminder.
 * @return A list of ReminderCondition objects.
 */
QList<ReminderCondition> SqliteReminderConditionRepository::listForReminder(qlonglong reminderId) {
    QList<ReminderCondition> conditions;

    if (reminderId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return conditions;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, reminder_id, start_bar, end_bar, min_bpm, min_minutes "
                  "FROM reminder_conditions WHERE reminder_id = :reminder_id ORDER BY id");
    query.bindValue(QStringLiteral(":reminder_id"), reminderId);

    if (!query.exec()) {
        qCritical() << "[SqliteReminderConditionRepository] listForReminder failed:"
                    << query.lastError().text();
        return conditions;
    }

    while (query.next()) {
        conditions.append(conditionFromQuery(query));
    }

    return conditions;
}
