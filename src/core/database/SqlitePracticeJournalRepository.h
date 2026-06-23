#ifndef SQLITEPRACTICEJOURNALREPOSITORY_H
#define SQLITEPRACTICEJOURNALREPOSITORY_H

#include "interfaces/IDatabaseConnection.h"
#include "interfaces/IPracticeJournalRepository.h"

#include <QDate>
#include <QList>
#include <QSqlQuery>
#include <optional>

/**
 * @brief Repository for the practice_journal table.
 *
 * Rows reference practice assets via asset_id (not song_id directly).
 * All methods validate inputs and ensure the database connection is open
 * (via RepositoryUtils).
 */
class SqlitePracticeJournalRepository final : public IPracticeJournalRepository {
  public:
    explicit SqlitePracticeJournalRepository(IDatabaseConnection &connection);

    QList<JournalEntry> listForAssetAndDate(qlonglong assetId, const QDate &date) override;
    QList<JournalEntry> listForSongAndDate(qlonglong songId, const QDate &date) override;
    QList<JournalDayEntry> listDayEntriesWithSong(const QDate &date) override;
    QList<QDate> distinctPracticeDatesInMonth(int year, int month) override;
    std::optional<JournalEntry> lastEntryForAsset(qlonglong assetId) override;

    // CRUD operations
    std::optional<qlonglong> createEntry(const JournalEntry &entry) override;
    std::optional<JournalEntry> getEntry(qlonglong id) override;

    bool updateEntry(const JournalEntry &entry) override;
    bool deleteEntry(qlonglong id) override;

  private:
    static JournalEntry entryFromQuery(QSqlQuery &query);
    static JournalDayEntry dayEntryFromQuery(QSqlQuery &query);
    IDatabaseConnection &m_connection;
};

#endif // SQLITEPRACTICEJOURNALREPOSITORY_H
