#include "SqlitePracticeJournalRepository.h"

#include "RepositoryUtils.h"
#include "SqlQueryColumns.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

/**
 * @brief Constructs the repository with a database connection.
 * @param connection The database connection object to use.
 */
SqlitePracticeJournalRepository::SqlitePracticeJournalRepository(IDatabaseConnection &connection)
    : m_connection(connection) {}

/**
 * @brief Adds a new practice session entry to the journal.
 * @param entry The JournalEntry object to save.
 * @return The ID of the new entry, or nullopt if it failed.
 */
std::optional<qlonglong> SqlitePracticeJournalRepository::createEntry(const JournalEntry &entry) {
    if (entry.assetId <= 0) {
        return std::nullopt;
    }

    if (!RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(
        "INSERT INTO practice_journal "
        "(user_id, asset_id, practice_date, start_bar, end_bar, practiced_bpm, "
        "total_reps, successful_streaks, duration_seconds, notice_id) "
        "VALUES (:user_id, :asset_id, :practice_date, :start_bar, :end_bar, :practiced_bpm, "
        ":total_reps, :successful_streaks, :duration_seconds, :notice_id)");
    query.bindValue(":user_id", entry.userId > 0 ? entry.userId : 1);
    query.bindValue(":asset_id", entry.assetId);
    query.bindValue(":practice_date", entry.practiceDate.isValid() ? entry.practiceDate
                                                                   : QDateTime::currentDateTime());
    query.bindValue(":start_bar", entry.startBar);
    query.bindValue(":end_bar", entry.endBar);
    query.bindValue(":practiced_bpm", entry.practicedBpm);
    query.bindValue(":total_reps", entry.totalReps);
    query.bindValue(":successful_streaks", entry.successfulStreaks);
    query.bindValue(":duration_seconds", entry.durationSeconds);
    query.bindValue(":notice_id", entry.noticeId > 0 ? QVariant(entry.noticeId) : QVariant());

    if (!query.exec()) {
        qCritical() << "[SqlitePracticeJournalRepository] createEntry failed:"
                    << query.lastError().text();
        return std::nullopt;
    }

    const qlonglong newId = query.lastInsertId().toLongLong();
    return newId > 0 ? std::optional<qlonglong>(newId) : std::nullopt;
}

/**
 * @brief Retrieves a specific journal entry by its unique ID.
 * @param id The ID of the entry.
 * @return The JournalEntry object, or nullopt if not found.
 */
std::optional<JournalEntry> SqlitePracticeJournalRepository::getEntry(qlonglong id) {
    if (id <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, user_id, asset_id, practice_date, start_bar, end_bar, practiced_bpm, "
                  "total_reps, successful_streaks, duration_seconds, notice_id "
                  "FROM practice_journal WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    return entryFromQuery(query);
}

/**
 * @brief Updates an existing journal entry with new details.
 * @param entry The JournalEntry object containing updated information.
 * @return True if the update was successful, false otherwise.
 */
bool SqlitePracticeJournalRepository::updateEntry(const JournalEntry &entry) {
    if (entry.id <= 0 || entry.assetId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("UPDATE practice_journal SET "
                  "user_id = :user_id, asset_id = :asset_id, practice_date = :practice_date, "
                  "start_bar = :start_bar, end_bar = :end_bar, practiced_bpm = :practiced_bpm, "
                  "total_reps = :total_reps, successful_streaks = :successful_streaks, "
                  "duration_seconds = :duration_seconds, notice_id = :notice_id "
                  "WHERE id = :id");
    query.bindValue(":id", entry.id);
    query.bindValue(":user_id", entry.userId > 0 ? entry.userId : 1);
    query.bindValue(":asset_id", entry.assetId);
    query.bindValue(":practice_date", entry.practiceDate);
    query.bindValue(":start_bar", entry.startBar);
    query.bindValue(":end_bar", entry.endBar);
    query.bindValue(":practiced_bpm", entry.practicedBpm);
    query.bindValue(":total_reps", entry.totalReps);
    query.bindValue(":successful_streaks", entry.successfulStreaks);
    query.bindValue(":duration_seconds", entry.durationSeconds);
    query.bindValue(":notice_id", entry.noticeId > 0 ? QVariant(entry.noticeId) : QVariant());

    if (!query.exec()) {
        qCritical() << "[SqlitePracticeJournalRepository] updateEntry failed:"
                    << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

/**
 * @brief Removes a journal entry from the database.
 * @param id The ID of the entry to delete.
 * @return True if the entry was deleted, false otherwise.
 */
bool SqlitePracticeJournalRepository::deleteEntry(qlonglong id) {
    if (id <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("DELETE FROM practice_journal WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qCritical() << "[SqlitePracticeJournalRepository] deleteEntry failed:"
                    << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

/**
 * @brief Gets all journal entries for a specific asset on a given date.
 * @param assetId The ID of the practice asset.
 * @param date The date to filter by.
 * @return A list of journal entries for that asset and date.
 */
QList<JournalEntry> SqlitePracticeJournalRepository::listForAssetAndDate(qlonglong assetId,
                                                                         const QDate &date) {
    QList<JournalEntry> entries;

    if (assetId <= 0 || !date.isValid() || !RepositoryUtils::ensureOpen(m_connection)) {
        return entries;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, user_id, asset_id, practice_date, start_bar, end_bar, practiced_bpm, "
                  "total_reps, successful_streaks, duration_seconds, notice_id "
                  "FROM practice_journal "
                  "WHERE asset_id = :asset_id AND date(practice_date) = date(:practice_date) "
                  "ORDER BY practice_date");
    query.bindValue(":asset_id", assetId);
    query.bindValue(":practice_date", date.startOfDay());

    if (!query.exec()) {
        qCritical() << "[SqlitePracticeJournalRepository] listForAssetAndDate failed:"
                    << query.lastError().text();
        return entries;
    }

    while (query.next()) {
        entries.append(entryFromQuery(query));
    }

    return entries;
}

/**
 * @brief Retrieves the most recent practice entry for a specific asset.
 * @param assetId The ID of the practice asset.
 * @return The latest JournalEntry, or nullopt if no entry exists.
 */
std::optional<JournalEntry> SqlitePracticeJournalRepository::lastEntryForAsset(qlonglong assetId) {
    if (assetId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, user_id, asset_id, practice_date, start_bar, end_bar, practiced_bpm, "
                  "total_reps, successful_streaks, duration_seconds, notice_id "
                  "FROM practice_journal "
                  "WHERE asset_id = :asset_id "
                  "ORDER BY practice_date DESC, id DESC "
                  "LIMIT 1");
    query.bindValue(":asset_id", assetId);

    if (!query.exec()) {
        qCritical() << "[SqlitePracticeJournalRepository] lastEntryForAsset failed:"
                    << query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return entryFromQuery(query);
}

/**
 * @brief Maps a single database query row to a JournalEntry object.
 * @param query The active query object at the current record.
 * @return A populated JournalEntry object.
 */
JournalEntry SqlitePracticeJournalRepository::entryFromQuery(QSqlQuery &query) {
    JournalEntry entry;
    entry.id = query.value(SqlQueryColumns::PracticeJournal::Id).toLongLong();
    entry.userId = query.value(SqlQueryColumns::PracticeJournal::UserId).toLongLong();
    entry.assetId = query.value(SqlQueryColumns::PracticeJournal::AssetId).toLongLong();
    entry.practiceDate = query.value(SqlQueryColumns::PracticeJournal::PracticeDate).toDateTime();
    entry.startBar = query.value(SqlQueryColumns::PracticeJournal::StartBar).toInt();
    entry.endBar = query.value(SqlQueryColumns::PracticeJournal::EndBar).toInt();
    entry.practicedBpm = query.value(SqlQueryColumns::PracticeJournal::PracticedBpm).toInt();
    entry.totalReps = query.value(SqlQueryColumns::PracticeJournal::TotalReps).toInt();
    entry.successfulStreaks =
        query.value(SqlQueryColumns::PracticeJournal::SuccessfulStreaks).toInt();
    entry.durationSeconds = query.value(SqlQueryColumns::PracticeJournal::DurationSeconds).toInt();
    entry.noticeId = query.value(SqlQueryColumns::PracticeJournal::NoticeId).toLongLong();
    return entry;
}
