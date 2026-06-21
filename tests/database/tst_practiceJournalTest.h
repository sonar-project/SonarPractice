#ifndef TST_PRACTICEJOURNALTEST_H
#define TST_PRACTICEJOURNALTEST_H

#include "DatabaseTestFixture.h"

#include <QObject>

class TestPracticeJournal : public QObject, protected DatabaseTestFixture {
    Q_OBJECT

  public:
    TestPracticeJournal() : DatabaseTestFixture(QStringLiteral("PracticeJournalTest")) {}

  private slots:
    void init();
    void cleanup();

    void testAddJournalEntry();
    void testListJournalForAssetAndDate();
    void testUpdateJournalEntry();
    void testDeleteJournalEntry();
    void testLastJournalEntryForAsset();
};

#endif // TST_PRACTICEJOURNALTEST_H
