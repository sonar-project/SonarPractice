#ifndef IPRACTICEJOURNALREPOSITORY_H
#define IPRACTICEJOURNALREPOSITORY_H

#include <optional>

#include <QList>

#include "JournalEntry.h"

class IPracticeJournalRepository {
  public:
    virtual ~IPracticeJournalRepository() = default;

    IPracticeJournalRepository(const IPracticeJournalRepository &) = delete;
    IPracticeJournalRepository &operator=(const IPracticeJournalRepository &) = delete;
    IPracticeJournalRepository(IPracticeJournalRepository &&) = delete;
    IPracticeJournalRepository &operator=(IPracticeJournalRepository &&) = delete;

    virtual std::optional<qlonglong> createEntry(const JournalEntry &entry) = 0;
    virtual std::optional<JournalEntry> getEntry(qlonglong id) = 0;
    virtual bool updateEntry(const JournalEntry &entry) = 0;
    virtual bool deleteEntry(qlonglong id) = 0;
    virtual QList<JournalEntry> listForAssetAndDate(qlonglong assetId, const QDate &date) = 0;
    virtual std::optional<JournalEntry> lastEntryForAsset(qlonglong assetId) = 0;

  protected:
    IPracticeJournalRepository() = default;
};

#endif // IPRACTICEJOURNALREPOSITORY_H
