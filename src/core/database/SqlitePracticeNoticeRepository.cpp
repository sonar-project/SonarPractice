#include "SqlitePracticeNoticeRepository.h"

#include "RepositoryUtils.h"
#include "SqlQueryColumns.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

/**
 * @brief Constructs the repository with a database connection.
 * @param connection The database connection object to use.
 */
SqlitePracticeNoticeRepository::SqlitePracticeNoticeRepository(IDatabaseConnection &connection)
    : m_connection(connection) {}

/**
 * @brief Finds a practice notice for a specific song and date.
 * @param songId The ID of the song.
 * @param date The date of the notice.
 * @return The PracticeNotice object if found, or nullopt if not found.
 */
std::optional<PracticeNotice>
SqlitePracticeNoticeRepository::findForSongAndDate(qlonglong songId, const QDate &date) {
    if (songId <= 0 || !date.isValid() || !RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare("SELECT id, song_id, note_date, content "
                  "FROM practice_notices "
                  "WHERE song_id = :song_id AND note_date = :note_date");
    query.bindValue(QStringLiteral(":song_id"), songId);
    query.bindValue(QStringLiteral(":note_date"), date.toString(Qt::ISODate));

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    PracticeNotice notice;
    notice.id = query.value(SqlQueryColumns::PracticeNotice::Id).toLongLong();
    notice.songId = query.value(SqlQueryColumns::PracticeNotice::SongId).toLongLong();
    notice.noteDate = QDate::fromString(
        query.value(SqlQueryColumns::PracticeNotice::NoteDate).toString(), Qt::ISODate);
    notice.content = query.value(SqlQueryColumns::PracticeNotice::Content).toString();
    return notice;
}

/**
 * @brief Adds a new notice or updates an existing one for a song and date.
 * @param songId The ID of the song.
 * @param date The date of the notice.
 * @param content The text content of the notice.
 * @return True if the operation was successful, false otherwise.
 */
bool SqlitePracticeNoticeRepository::upsertForSongAndDate(qlonglong songId, const QDate &date,
                                                          const QString &content) {
    if (songId <= 0 || !date.isValid() || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    const std::optional<PracticeNotice> existing = findForSongAndDate(songId, date);
    QSqlQuery query(RepositoryUtils::database(m_connection));

    if (existing.has_value()) {
        query.prepare("UPDATE practice_notices SET content = :content WHERE id = :id");
        query.bindValue(QStringLiteral(":content"), content);
        query.bindValue(QStringLiteral(":id"), existing->id);
    } else {
        query.prepare("INSERT INTO practice_notices (song_id, note_date, content) "
                      "VALUES (:song_id, :note_date, :content)");
        query.bindValue(QStringLiteral(":song_id"), songId);
        query.bindValue(QStringLiteral(":note_date"), date.toString(Qt::ISODate));
        query.bindValue(QStringLiteral(":content"), content);
    }

    if (!query.exec()) {
        qCritical() << "[SqlitePracticeNoticeRepository] upsertForSongAndDate failed:"
                    << query.lastError().text();
        return false;
    }

    return true;
}
