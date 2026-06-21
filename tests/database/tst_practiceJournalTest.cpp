#include "tst_practiceJournalTest.h"

#include "JournalEntry.h"
#include "TestConstants.h"

#include <QDate>
#include <QDateTime>
#include <QTest>
#include <QTime>

void TestPracticeJournal::init() { setUp(); }

void TestPracticeJournal::cleanup() { tearDown(); }

void TestPracticeJournal::testAddJournalEntry() {
    qlonglong userId = 0;
    qlonglong unusedUserId = 0;
    createTestUsers(userId, unusedUserId);

    const std::optional<qlonglong> artistId = createTestArtist();
    const std::optional<qlonglong> tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    const std::optional<qlonglong> songId = createTestSong(*artistId, *tuningId);
    QVERIFY(songId.has_value());

    const std::optional<qlonglong> assetId = createTestPracticeAsset(*songId);
    QVERIFY(assetId.has_value());

    JournalEntry entry;
    entry.userId = userId;
    entry.assetId = *assetId;
    entry.practiceDate = QDateTime(QDate(TestDates::kYear, TestDates::kFebruary, TestDates::kDay28),
                                   QTime(TestDates::kHour10, TestDates::kMinute30));
    entry.startBar = TestJournal::kDefaultStartBar;
    entry.endBar = TestBars::kEnd;
    entry.practicedBpm = TestJournal::kDefaultTargetBpm;
    entry.totalReps = TestReps::kTotal;
    entry.successfulStreaks = TestReps::kPartialSuccess;
    entry.durationSeconds = TestDurations::kNineMinutes;

    const std::optional<qlonglong> entryId = m_journalRepo.createEntry(entry);
    QVERIFY(entryId.has_value());

    const std::optional<JournalEntry> loaded = m_journalRepo.getEntry(*entryId);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->assetId, *assetId);
    QCOMPARE(loaded->startBar, TestJournal::kDefaultStartBar);
    QCOMPARE(loaded->endBar, TestBars::kEnd);
    QCOMPARE(loaded->practicedBpm, TestJournal::kDefaultTargetBpm);
    QCOMPARE(loaded->successfulStreaks, TestReps::kPartialSuccess);
    QCOMPARE(loaded->durationSeconds, TestDurations::kNineMinutes);
}

void TestPracticeJournal::testListJournalForAssetAndDate() {
    qlonglong userId = 0;
    qlonglong unusedUserId = 0;
    createTestUsers(userId, unusedUserId);

    const std::optional<qlonglong> artistId = createTestArtist();
    const std::optional<qlonglong> tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    const std::optional<qlonglong> songId = createTestSong(*artistId, *tuningId);
    QVERIFY(songId.has_value());

    const std::optional<qlonglong> assetId = createTestPracticeAsset(*songId);
    QVERIFY(assetId.has_value());

    const QDate practiceDay(TestDates::kYear, TestDates::kFebruary, TestDates::kDay28);

    JournalEntry dayEntry;
    dayEntry.userId = userId;
    dayEntry.assetId = *assetId;
    dayEntry.practiceDate = QDateTime(practiceDay, QTime(TestDates::kHour9, TestDates::kMinute0));
    dayEntry.startBar = TestJournal::kDefaultStartBar;
    dayEntry.endBar = TestJournal::kDefaultEndBar;
    dayEntry.practicedBpm = TestBpm::kSlow;
    dayEntry.durationSeconds = TestDurations::kLongSeconds;
    QVERIFY(m_journalRepo.createEntry(dayEntry).has_value());

    JournalEntry otherDayEntry = dayEntry;
    otherDayEntry.practiceDate =
        QDateTime(QDate(TestDates::kYear, TestDates::kMarch, TestDates::kDay1),
                  QTime(TestDates::kHour9, TestDates::kMinute0));
    QVERIFY(m_journalRepo.createEntry(otherDayEntry).has_value());

    const QList<JournalEntry> entries = m_journalRepo.listForAssetAndDate(*assetId, practiceDay);
    QCOMPARE(entries.size(), TestCounts::kOne);
    QCOMPARE(entries.first().startBar, TestJournal::kDefaultStartBar);
    QCOMPARE(entries.first().endBar, TestJournal::kDefaultEndBar);
}

void TestPracticeJournal::testUpdateJournalEntry() {
    qlonglong userId = 0;
    qlonglong unusedUserId = 0;
    createTestUsers(userId, unusedUserId);

    const std::optional<qlonglong> artistId = createTestArtist();
    const std::optional<qlonglong> tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    const std::optional<qlonglong> songId = createTestSong(*artistId, *tuningId);
    QVERIFY(songId.has_value());

    const std::optional<qlonglong> assetId = createTestPracticeAsset(*songId);
    QVERIFY(assetId.has_value());

    JournalEntry entry;
    entry.userId = userId;
    entry.assetId = *assetId;
    entry.practiceDate = QDateTime::currentDateTime();
    entry.startBar = TestJournal::kDefaultStartBar;
    entry.endBar = TestJournal::kDefaultEndBar;
    entry.practicedBpm = TestJournal::kDefaultTargetBpm;

    const std::optional<qlonglong> entryId = m_journalRepo.createEntry(entry);
    QVERIFY(entryId.has_value());

    JournalEntry updated = *m_journalRepo.getEntry(*entryId);
    updated.endBar = TestBars::kEndSection;
    updated.practicedBpm = TestBpm::kPractice;
    updated.durationSeconds = TestDurations::kFiveMinutes;
    QVERIFY(m_journalRepo.updateEntry(updated));

    const std::optional<JournalEntry> loaded = m_journalRepo.getEntry(*entryId);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->endBar, TestBars::kEndSection);
    QCOMPARE(loaded->practicedBpm, TestBpm::kPractice);
    QCOMPARE(loaded->durationSeconds, TestDurations::kFiveMinutes);
}

void TestPracticeJournal::testLastJournalEntryForAsset() {
    qlonglong userId = 0;
    qlonglong unusedUserId = 0;
    createTestUsers(userId, unusedUserId);

    const std::optional<qlonglong> artistId = createTestArtist();
    const std::optional<qlonglong> tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    const std::optional<qlonglong> songId = createTestSong(*artistId, *tuningId);
    QVERIFY(songId.has_value());

    const std::optional<qlonglong> assetId = createTestPracticeAsset(*songId);
    QVERIFY(assetId.has_value());

    JournalEntry olderEntry;
    olderEntry.userId = userId;
    olderEntry.assetId = *assetId;
    olderEntry.practiceDate =
        QDateTime(QDate(TestDates::kYear, TestDates::kJanuary, TestDates::kDay10),
                  QTime(TestDates::kHour9, TestDates::kMinute0));
    olderEntry.startBar = TestJournal::kDefaultStartBar;
    olderEntry.endBar = TestJournal::kDefaultEndBar;
    olderEntry.practicedBpm = TestBpm::kOlderEntry;
    QVERIFY(m_journalRepo.createEntry(olderEntry).has_value());

    JournalEntry latestEntry;
    latestEntry.userId = userId;
    latestEntry.assetId = *assetId;
    latestEntry.practiceDate =
        QDateTime(QDate(TestDates::kYear, TestDates::kFebruary, TestDates::kDay20),
                  QTime(TestDates::kHour9, TestDates::kMinute0));
    latestEntry.startBar = TestBars::kStartLater;
    latestEntry.endBar = TestBars::kEndLater;
    latestEntry.practicedBpm = TestBpm::kLastEntry;
    QVERIFY(m_journalRepo.createEntry(latestEntry).has_value());

    const std::optional<JournalEntry> loaded = m_journalRepo.lastEntryForAsset(*assetId);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->startBar, TestBars::kStartLater);
    QCOMPARE(loaded->endBar, TestBars::kEndLater);
    QCOMPARE(loaded->practicedBpm, TestBpm::kLastEntry);
}

void TestPracticeJournal::testDeleteJournalEntry() {
    qlonglong userId = 0;
    qlonglong unusedUserId = 0;
    createTestUsers(userId, unusedUserId);

    const std::optional<qlonglong> artistId = createTestArtist();
    const std::optional<qlonglong> tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    const std::optional<qlonglong> songId = createTestSong(*artistId, *tuningId);
    QVERIFY(songId.has_value());

    const std::optional<qlonglong> assetId = createTestPracticeAsset(*songId);
    QVERIFY(assetId.has_value());

    JournalEntry entry;
    entry.userId = userId;
    entry.assetId = *assetId;
    entry.practiceDate = QDateTime::currentDateTime();

    const std::optional<qlonglong> entryId = m_journalRepo.createEntry(entry);
    QVERIFY(entryId.has_value());
    QVERIFY(m_journalRepo.deleteEntry(*entryId));
    QVERIFY(!m_journalRepo.getEntry(*entryId).has_value());
}

QTEST_MAIN(TestPracticeJournal)
