#ifndef SQLITEPRACTICENOTICEREPOSITORY_H
#define SQLITEPRACTICENOTICEREPOSITORY_H

#include "interfaces/IDatabaseConnection.h"
#include "interfaces/IPracticeNoticeRepository.h"

class SqlitePracticeNoticeRepository : public IPracticeNoticeRepository {
  public:
    explicit SqlitePracticeNoticeRepository(IDatabaseConnection &connection);

    [[nodiscard]] std::optional<PracticeNotice> findForSongAndDate(qlonglong songId, const QDate &date) override;
    [[nodiscard]] bool upsertForSongAndDate(qlonglong songId, const QDate &date, const QString &content) override;

  private:
    IDatabaseConnection &m_connection;
};

#endif // SQLITEPRACTICENOTICEREPOSITORY_H
