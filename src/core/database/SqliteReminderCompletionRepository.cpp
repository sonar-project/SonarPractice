#include "SqliteReminderCompletionRepository.h"

#include "RepositoryUtils.h"

#include <QSqlError>
#include <QSqlQuery>

SqliteReminderCompletionRepository::SqliteReminderCompletionRepository(
    IDatabaseConnection &connection)
    : m_connection(connection) {}

bool SqliteReminderCompletionRepository::isAccepted(qlonglong reminderId,
                                                    const QDate &date) const {
    if (reminderId <= 0 || !date.isValid() || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(QStringLiteral(
        "SELECT accepted FROM reminder_completion_overrides "
        "WHERE reminder_id = :reminder_id AND completion_date = :completion_date"));
    query.bindValue(QStringLiteral(":reminder_id"), reminderId);
    query.bindValue(QStringLiteral(":completion_date"), date.toString(Qt::ISODate));

    if (!query.exec() || !query.next()) {
        return false;
    }

    return query.value(0).toInt() != 0;
}

bool SqliteReminderCompletionRepository::setAccepted(qlonglong reminderId, const QDate &date,
                                                     bool accepted) {
    if (reminderId <= 0 || !date.isValid() || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(QStringLiteral(
        "INSERT INTO reminder_completion_overrides (reminder_id, completion_date, accepted) "
        "VALUES (:reminder_id, :completion_date, :accepted) "
        "ON CONFLICT(reminder_id, completion_date) DO UPDATE SET accepted = :accepted"));
    query.bindValue(QStringLiteral(":reminder_id"), reminderId);
    query.bindValue(QStringLiteral(":completion_date"), date.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":accepted"), accepted ? 1 : 0);

    if (!query.exec()) {
        qCritical() << "[SqliteReminderCompletionRepository] setAccepted failed:"
                    << query.lastError().text();
        return false;
    }

    return true;
}

bool SqliteReminderCompletionRepository::clearOverride(qlonglong reminderId, const QDate &date) {
    if (reminderId <= 0 || !date.isValid() || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(QStringLiteral(
        "DELETE FROM reminder_completion_overrides "
        "WHERE reminder_id = :reminder_id AND completion_date = :completion_date"));
    query.bindValue(QStringLiteral(":reminder_id"), reminderId);
    query.bindValue(QStringLiteral(":completion_date"), date.toString(Qt::ISODate));

    if (!query.exec()) {
        qCritical() << "[SqliteReminderCompletionRepository] clearOverride failed:"
                    << query.lastError().text();
        return false;
    }

    return true;
}
