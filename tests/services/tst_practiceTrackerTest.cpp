#include "tst_practiceTrackerTest.h"

#include "DatabaseSchema.h"
#include "JournalEntry.h"
#include "MediaFile.h"
#include "PracticeAsset.h"
#include "PracticeConstants.h"
#include "PracticeTrackerService.h"
#include "Song.h"
#include "SqliteUserRepository.h"
#include "TestConstants.h"
#include "User.h"

#include <QDate>
#include <QTest>

void TestPracticeTracker::init() {
    QVERIFY(m_connector.open(QStringLiteral(":memory:")));

    DatabaseSchema schema(m_connector);
    QVERIFY(schema.createAllTables());

    User user;
    user.name = QStringLiteral("TrackerUser");
    user.role = QStringLiteral("student");
    SqliteUserRepository userRepo(m_connector);
    QVERIFY(userRepo.createUser(user).has_value());
}

void TestPracticeTracker::cleanup() {
    m_connector.close();
    QSqlDatabase::removeDatabase(QStringLiteral("PracticeTrackerTestDb"));
}

void TestPracticeTracker::testComputeStreak() {
    QCOMPARE(PracticeTrackerService::computeStreak(0, 0), 0);
    QCOMPARE(PracticeTrackerService::computeStreak(TestReps::kTotal, TestReps::kPartialSuccess),
             TestReps::kPartialSuccess);
    QCOMPARE(PracticeTrackerService::computeStreak(TestReps::kTotal, TestReps::kFullSuccess),
             PracticeConstants::kMaxSuccessfulStreak);
    QCOMPARE(PracticeTrackerService::computeStreak(TestReps::kNearTotal, TestReps::kNearTotal - 1),
             PracticeConstants::kMaxSuccessfulStreak);
}

void TestPracticeTracker::testStartStopDoesNotPersistUntilStop() {
    const std::optional<qlonglong> assetId = createAsset();
    QVERIFY(assetId.has_value());

    PracticeTrackerService service(m_journalRepo);
    PracticeTrackerParams params;
    params.assetId = *assetId;
    params.startBar = PracticeConstants::kDefaultStartBar;
    params.endBar = PracticeConstants::kDefaultEndBar;
    params.targetBpm = PracticeConstants::kDefaultTargetBpm;

    QVERIFY(service.startTimer());
    QVERIFY(service.isTimerRunning());

    const QList<JournalEntry> beforeStop =
        m_journalRepo.listForAssetAndDate(*assetId, QDate::currentDate());
    QCOMPARE(beforeStop.size(), 0);

    service.cancelTimer();
    QVERIFY(!service.isTimerRunning());
}

void TestPracticeTracker::testStopWithDurationCreatesJournalEntry() {
    const std::optional<qlonglong> assetId = createAsset();
    QVERIFY(assetId.has_value());

    PracticeTrackerService service(m_journalRepo);
    PracticeTrackerParams params;
    params.assetId = *assetId;
    params.startBar = TestCounts::kTwo;
    params.endBar = TestBars::kEnd;
    params.targetBpm = TestBpm::kPractice;
    params.totalReps = TestReps::kTotal;
    params.successfulReps = TestReps::kSuccessful;

    QVERIFY(service.startTimer());
    const std::optional<qlonglong> entryId =
        service.stopAndSave(params, TestDurations::kPracticeSeconds);
    QVERIFY(entryId.has_value());

    const std::optional<JournalEntry> loaded = m_journalRepo.getEntry(*entryId);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->assetId, *assetId);
    QCOMPARE(loaded->startBar, TestCounts::kTwo);
    QCOMPARE(loaded->endBar, TestBars::kEnd);
    QCOMPARE(loaded->practicedBpm, TestBpm::kPractice);
    QCOMPARE(loaded->durationSeconds, TestDurations::kPracticeSeconds);
    QCOMPARE(loaded->successfulStreaks, TestReps::kSuccessful);
    QCOMPARE(PracticeTrackerService::durationMinutesFromSeconds(TestDurations::kPracticeSeconds),
             TestDurations::kTwoMinutes);
}

void TestPracticeTracker::testStopRejectsInvalidBarRange() {
    const std::optional<qlonglong> assetId = createAsset();
    QVERIFY(assetId.has_value());

    PracticeTrackerService service(m_journalRepo);
    PracticeTrackerParams params;
    params.assetId = *assetId;
    params.startBar = TestBars::kEnd; // startBar > endBar: ungültiger Bereich
    params.endBar = TestCounts::kTwo;

    QVERIFY(service.startTimer());
    QVERIFY(!service.stopAndSave(params, TestDurations::kShortSeconds).has_value());
}

void TestPracticeTracker::testDurationMinutesFromSeconds() {
    QCOMPARE(PracticeTrackerService::durationMinutesFromSeconds(0), 0);
    QCOMPARE(PracticeTrackerService::durationMinutesFromSeconds(TestDurations::kShortSeconds),
             TestDurations::kOneMinuteRounded);
    QCOMPARE(PracticeTrackerService::durationMinutesFromSeconds(TestDurations::kPracticeSeconds),
             TestDurations::kTwoMinutes);
}

std::optional<qlonglong> TestPracticeTracker::createAsset() {
    // 1. Song anlegen
    Song song;
    song.title = QStringLiteral("Tracker Song");
    song.baseBpm = TestBpm::kSecondary;
    const std::optional<qlonglong> songId = m_songRepo.createSong(song);
    if (!songId.has_value())
        return std::nullopt;

    // 2. MediaFile anlegen (GP-Datei, can_be_practiced = true)
    MediaFile mediaFile;
    mediaFile.songId = *songId;
    mediaFile.filePath = QStringLiteral("/test/tracker_song.gp5");
    mediaFile.fileType = QStringLiteral("gp5");
    mediaFile.mediaKind = MediaKind::GuitarPro;
    mediaFile.canBePracticed = true;
    mediaFile.sourceType = MediaSourceType::Local;
    const std::optional<qlonglong> mediaFileId = m_mediaRepo.createMediaFile(mediaFile);
    if (!mediaFileId.has_value())
        return std::nullopt;

    // 3. practice_assets-Eintrag anlegen und asset_id zurückgeben
    return insertPracticeAsset(*songId, *mediaFileId);
}

std::optional<qlonglong> TestPracticeTracker::insertPracticeAsset(qlonglong songId,
                                                                  qlonglong mediaFileId) {
    PracticeAsset asset;
    asset.songId = songId;
    asset.guitarProId = mediaFileId;
    return m_practiceAssetRepo.upsert(asset);
}

QTEST_MAIN(TestPracticeTracker)
