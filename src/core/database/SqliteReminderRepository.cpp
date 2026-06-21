#include "SqliteReminderRepository.h"

#include "ReminderConstants.h"
#include "RepositoryUtils.h"
#include "SqlQueryColumns.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

    constexpr auto kReminderSelectColumns =
        "id, user_id, song_id, title, reminder_date, interval_days, weekday, "
        "is_daily, is_monthly, is_weekly, is_active, practice_asset_id";

    constexpr auto kReminderSongSelectColumns =
        "r.id, r.user_id, r.song_id, r.title, r.reminder_date, r.interval_days, r.weekday, "
        "r.is_daily, r.is_monthly, r.is_weekly, r.is_active, r.practice_asset_id, "
        "s.title AS song_title, s.base_bpm AS song_base_bpm";

    int schemaWeekdayForDate(const QDate &date) {
        const int qtWeekday = date.dayOfWeek();
        return qtWeekday == ReminderConstants::kQtSundayDayOfWeek
                   ? ReminderConstants::kReminderSundayIndex
                   : qtWeekday;
    }

} // namespace

/**
 * @brief Constructs the repository with a database connection.
 * @param connection The database connection object to use.
 */
SqliteReminderRepository::SqliteReminderRepository(IDatabaseConnection &connection)
    : m_connection(connection) {}

/**
 * @brief Converts the current row of a database query into a Reminder object.
 * @param query The active query object positioned at the desired record.
 * @return A populated Reminder object.
 */
Reminder SqliteReminderRepository::reminderFromQuery(const QSqlQuery &query) {
    Reminder reminder;
    reminder.id = query.value(SqlQueryColumns::Reminder::Id).toLongLong();
    reminder.userId = query.value(SqlQueryColumns::Reminder::UserId).toLongLong();
    reminder.songId = query.value(SqlQueryColumns::Reminder::SongId).toLongLong();
    reminder.title = query.value(SqlQueryColumns::Reminder::Title).toString();
    const QString dateStr = query.value(SqlQueryColumns::Reminder::ReminderDate).toString();
    if (!dateStr.isEmpty()) {
        reminder.reminderDate = QDate::fromString(dateStr, Qt::ISODate);
    }
    reminder.intervalDays = query.value(SqlQueryColumns::Reminder::IntervalDays).toInt();
    reminder.weekday = query.value(SqlQueryColumns::Reminder::Weekday).isNull()
                           ? -1
                           : query.value(SqlQueryColumns::Reminder::Weekday).toInt();
    reminder.isDaily = query.value(SqlQueryColumns::Reminder::IsDaily).toInt() != 0;
    reminder.isMonthly = query.value(SqlQueryColumns::Reminder::IsMonthly).toInt() != 0;
    reminder.isWeekly = query.value(SqlQueryColumns::Reminder::IsWeekly).toInt() != 0;
    reminder.isActive = query.value(SqlQueryColumns::Reminder::IsActive).toInt() != 0;
    reminder.practiceAssetId = query.value(SqlQueryColumns::Reminder::PracticeAssetId).toLongLong();
    return reminder;
}

/**
 * @brief Converts the current row of a database query into a ReminderDayEntry object.
 * @param query The active query object positioned at the desired record.
 * @return A populated ReminderDayEntry object.
 */
ReminderDayEntry SqliteReminderRepository::entryFromQuery(const QSqlQuery &query) {
    ReminderDayEntry entry;
    entry.reminder = reminderFromQuery(query);
    entry.songTitle = query.value(QStringLiteral("song_title")).toString();
    entry.baseBpm = query.value(QStringLiteral("song_base_bpm")).toInt();
    return entry;
}

/**
 * @brief Saves a new reminder to the database.
 * @param reminder The Reminder object to store.
 * @return The ID of the newly created reminder, or nullopt if it failed.
 */
std::optional<qlonglong> SqliteReminderRepository::createReminder(const Reminder &reminder) {
    if (reminder.songId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("INSERT INTO reminders "
                  "(user_id, song_id, title, reminder_date, interval_days, weekday, "
                  "is_daily, is_monthly, is_weekly, is_active, practice_asset_id) "
                  "VALUES (:user_id, :song_id, :title, :reminder_date, :interval_days, :weekday, "
                  ":is_daily, :is_monthly, :is_weekly, :is_active, :practice_asset_id)");
    query.bindValue(QStringLiteral(":user_id"), reminder.userId > 0 ? reminder.userId : 1);
    query.bindValue(QStringLiteral(":song_id"), reminder.songId);
    query.bindValue(QStringLiteral(":title"), reminder.title);
    query.bindValue(QStringLiteral(":reminder_date"),
                    reminder.reminderDate.isValid() ? reminder.reminderDate.toString(Qt::ISODate)
                                                    : QVariant());
    query.bindValue(QStringLiteral(":interval_days"), reminder.intervalDays);
    query.bindValue(QStringLiteral(":weekday"),
                    reminder.weekday >= 0 ? reminder.weekday : QVariant());
    query.bindValue(QStringLiteral(":is_daily"), reminder.isDaily ? 1 : 0);
    query.bindValue(QStringLiteral(":is_monthly"), reminder.isMonthly ? 1 : 0);
    query.bindValue(QStringLiteral(":is_weekly"), reminder.isWeekly ? 1 : 0);
    query.bindValue(QStringLiteral(":is_active"), reminder.isActive ? 1 : 0);
    query.bindValue(QStringLiteral(":practice_asset_id"),
                    reminder.practiceAssetId > 0 ? QVariant(reminder.practiceAssetId) : QVariant());

    if (!query.exec()) {
        qCritical() << "[SqliteReminderRepository] createReminder failed:"
                    << query.lastError().text();
        return std::nullopt;
    }

    const qlonglong newId = query.lastInsertId().toLongLong();
    return newId > 0 ? std::optional<qlonglong>(newId) : std::nullopt;
}

/**
 * @brief Retrieves a specific reminder from the database by its ID.
 * @param id The unique ID of the reminder.
 * @return The Reminder object, or nullopt if not found.
 */
std::optional<Reminder> SqliteReminderRepository::getReminder(qlonglong id) {
    if (id <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(QStringLiteral("SELECT %1 FROM reminders WHERE id = :id")
                      .arg(QString::fromLatin1(kReminderSelectColumns)));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    return reminderFromQuery(query);
}

/**
 * @brief Updates an existing reminder with new settings.
 * @param reminder The reminder object containing updated values.
 * @return True if the update was successful, false otherwise.
 */
bool SqliteReminderRepository::updateReminder(const Reminder &reminder) {
    if (reminder.id <= 0 || reminder.songId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(
        "UPDATE reminders SET user_id = :user_id, song_id = :song_id, title = :title, "
        "reminder_date = :reminder_date, interval_days = :interval_days, weekday = :weekday, "
        "is_daily = :is_daily, is_monthly = :is_monthly, is_weekly = :is_weekly, "
        "is_active = :is_active, practice_asset_id = :practice_asset_id WHERE id = :id");
    query.bindValue(QStringLiteral(":id"), reminder.id);
    query.bindValue(QStringLiteral(":user_id"), reminder.userId > 0 ? reminder.userId : 1);
    query.bindValue(QStringLiteral(":song_id"), reminder.songId);
    query.bindValue(QStringLiteral(":title"), reminder.title);
    query.bindValue(QStringLiteral(":reminder_date"),
                    reminder.reminderDate.isValid() ? reminder.reminderDate.toString(Qt::ISODate)
                                                    : QVariant());
    query.bindValue(QStringLiteral(":interval_days"), reminder.intervalDays);
    query.bindValue(QStringLiteral(":weekday"),
                    reminder.weekday >= 0 ? reminder.weekday : QVariant());
    query.bindValue(QStringLiteral(":is_daily"), reminder.isDaily ? 1 : 0);
    query.bindValue(QStringLiteral(":is_monthly"), reminder.isMonthly ? 1 : 0);
    query.bindValue(QStringLiteral(":is_weekly"), reminder.isWeekly ? 1 : 0);
    query.bindValue(QStringLiteral(":is_active"), reminder.isActive ? 1 : 0);
    query.bindValue(QStringLiteral(":practice_asset_id"),
                    reminder.practiceAssetId > 0 ? QVariant(reminder.practiceAssetId) : QVariant());

    if (!query.exec()) {
        qCritical() << "[SqliteReminderRepository] updateReminder failed:"
                    << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

/**
 * @brief Removes a reminder from the database by its ID.
 * @param id The unique ID of the reminder to delete.
 * @return True if the deletion was successful, false otherwise.
 */
bool SqliteReminderRepository::deleteReminder(qlonglong id) {
    if (id <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(QStringLiteral("DELETE FROM reminders WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qCritical() << "[SqliteReminderRepository] deleteReminder failed:"
                    << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

/**
 * @brief Lists all reminders that are currently marked as active.
 * @return A list of all active Reminder objects.
 */
QList<Reminder> SqliteReminderRepository::listAllActive() {
    QList<Reminder> reminders;

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return reminders;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    const QString sql =
        QStringLiteral("SELECT %1 FROM reminders WHERE is_active = 1 ORDER BY song_id, id")
            .arg(QString::fromLatin1(kReminderSelectColumns));

    if (!query.exec(sql)) {
        qCritical() << "[SqliteReminderRepository] listAllActive failed:"
                    << query.lastError().text();
        return reminders;
    }

    while (query.next()) {
        reminders.append(reminderFromQuery(query));
    }

    return reminders;
}

/**
 * @brief Lists all reminders linked to a specific song.
 * @param songId The ID of the song.
 * @return A list of reminders linked to the song.
 */
QList<Reminder> SqliteReminderRepository::listForSong(qlonglong songId) {
    QList<Reminder> reminders;

    if (songId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return reminders;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(QStringLiteral("SELECT %1 FROM reminders "
                                 "WHERE song_id = :song_id "
                                 "AND (practice_asset_id IS NULL OR practice_asset_id = 0) "
                                 "ORDER BY id")
                      .arg(QString::fromLatin1(kReminderSelectColumns)));
    query.bindValue(QStringLiteral(":song_id"), songId);

    if (!query.exec()) {
        qCritical() << "[SqliteReminderRepository] listForSong failed:" << query.lastError().text();
        return reminders;
    }

    while (query.next()) {
        reminders.append(reminderFromQuery(query));
    }

    return reminders;
}

/**
 * @brief Lists all reminders associated with a specific practice asset.
 * @param practiceAssetId The ID of the practice asset.
 * @return A list of reminders linked to the practice asset.
 */
QList<Reminder> SqliteReminderRepository::listForPracticeAsset(qlonglong practiceAssetId) {
    QList<Reminder> reminders;

    if (practiceAssetId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return reminders;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(QStringLiteral("SELECT %1 FROM reminders "
                                 "WHERE practice_asset_id = :practice_asset_id "
                                 "ORDER BY id")
                      .arg(QString::fromLatin1(kReminderSelectColumns)));
    query.bindValue(QStringLiteral(":practice_asset_id"), practiceAssetId);

    if (!query.exec()) {
        qCritical() << "[SqliteReminderRepository] listForPracticeAsset failed:"
                    << query.lastError().text();
        return reminders;
    }

    while (query.next()) {
        reminders.append(reminderFromQuery(query));
    }

    return reminders;
}

/**
 * @brief Lists all reminders due on a specific date.
 * @param date The target date.
 * @return A list of reminders due on that date.
 */
QList<Reminder> SqliteReminderRepository::listForDate(const QDate &date) {
    QList<Reminder> due;
    const QList<ReminderDayEntry> entries = listForDateWithSong(date);
    due.reserve(entries.size());
    for (const ReminderDayEntry &entry : entries) {
        due.append(entry.reminder);
    }
    return due;
}

/**
 * @brief Finds potential candidates for a due reminder on a specific date.
 * @param date The target date.
 * @return A list of ReminderDayEntry objects that might be due.
 */
QList<ReminderDayEntry> SqliteReminderRepository::queryCandidatesForDate(const QDate &date) {
    QList<ReminderDayEntry> candidates;

    if (!date.isValid() || !RepositoryUtils::ensureOpen(m_connection)) {
        return candidates;
    }

    const int weekday = schemaWeekdayForDate(date);
    const int dayOfMonth = date.day();
    const QString dateString = date.toString(Qt::ISODate);

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(
        QStringLiteral(
            "SELECT %1 FROM reminders r "
            "LEFT JOIN songs s ON r.song_id = s.id "
            "WHERE r.is_active = 1 AND ("
            "  r.is_daily = 1 "
            "  OR (r.is_weekly = 1 AND r.weekday = :weekday) "
            "  OR (r.is_monthly = 1 AND r.reminder_date IS NOT NULL "
            "      AND CAST(strftime('%%d', r.reminder_date) AS INTEGER) = :day_of_month) "
            "  OR (r.interval_days > 0 AND r.reminder_date IS NOT NULL "
            "      AND r.reminder_date <= :target_date) "
            "  OR (r.is_daily = 0 AND r.is_weekly = 0 AND r.is_monthly = 0 "
            "      AND r.interval_days = 0 AND r.reminder_date = :target_date)"
            ") ORDER BY r.song_id, r.id")
            .arg(QString::fromLatin1(kReminderSongSelectColumns)));
    query.bindValue(QStringLiteral(":weekday"), weekday);
    query.bindValue(QStringLiteral(":day_of_month"), dayOfMonth);
    query.bindValue(QStringLiteral(":target_date"), dateString);

    if (!query.exec()) {
        qCritical() << "[SqliteReminderRepository] queryCandidatesForDate failed:"
                    << query.lastError().text();
        return candidates;
    }

    while (query.next()) {
        candidates.append(entryFromQuery(query));
    }

    return candidates;
}

/**
 * @brief Lists all reminders due on a date, including song information.
 * @param date The target date.
 * @return A list of ReminderDayEntry objects.
 */
QList<ReminderDayEntry> SqliteReminderRepository::listForDateWithSong(const QDate &date) {
    if (!date.isValid()) {
        return {};
    }

    const QList<ReminderDayEntry> candidates = queryCandidatesForDate(date);
    QList<ReminderDayEntry> due;
    due.reserve(candidates.size());

    for (const ReminderDayEntry &entry : candidates) {
        if (entry.reminder.isDueOn(date)) {
            due.append(entry);
        }
    }

    return due;
}

/**
 * @brief Lists all active reminders including their associated song details.
 * @return A list of all active ReminderDayEntry objects.
 */
QList<ReminderDayEntry> SqliteReminderRepository::listAllActiveWithSong() {
    QList<ReminderDayEntry> entries;

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return entries;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    const QString sql = QStringLiteral("SELECT %1 FROM reminders r "
                                       "LEFT JOIN songs s ON r.song_id = s.id "
                                       "WHERE r.is_active = 1 ORDER BY r.song_id, r.id")
                            .arg(QString::fromLatin1(kReminderSongSelectColumns));

    if (!query.exec(sql)) {
        qCritical() << "[SqliteReminderRepository] listAllActiveWithSong failed:"
                    << query.lastError().text();
        return entries;
    }

    while (query.next()) {
        entries.append(entryFromQuery(query));
    }

    return entries;
}

/**
 * @brief Finds all active reminders that will be due within a date range.
 * @param from The start date of the range.
 * @param to The end date of the range.
 * @return A list of reminders due within the range.
 */
QList<Reminder> SqliteReminderRepository::listActiveInRange(const QDate &from, const QDate &to) {
    QList<Reminder> result;

    if (!from.isValid() || !to.isValid() || from > to) {
        return result;
    }

    const QList<Reminder> active = listAllActive();
    for (const Reminder &reminder : active) {
        for (QDate day = from; day <= to; day = day.addDays(1)) {
            if (reminder.isDueOn(day)) {
                result.append(reminder);
                break;
            }
        }
    }

    return result;
}
