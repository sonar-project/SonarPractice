#ifndef IPRACTICENOTICEREPOSITORY_H
#define IPRACTICENOTICEREPOSITORY_H

#include <optional>

#include <QDate>
#include <QString>

#include "PracticeNotice.h"

class IPracticeNoticeRepository {
  public:
    virtual ~IPracticeNoticeRepository() = default;

    IPracticeNoticeRepository(const IPracticeNoticeRepository &) = delete;
    IPracticeNoticeRepository &operator=(const IPracticeNoticeRepository &) = delete;
    IPracticeNoticeRepository(IPracticeNoticeRepository &&) = delete;
    IPracticeNoticeRepository &operator=(IPracticeNoticeRepository &&) = delete;

    virtual std::optional<PracticeNotice> findForSongAndDate(qlonglong songId,
                                                             const QDate &date) = 0;
    virtual bool upsertForSongAndDate(qlonglong songId, const QDate &date,
                                      const QString &content) = 0;

  protected:
    IPracticeNoticeRepository() = default;
};

#endif // IPRACTICENOTICEREPOSITORY_H
