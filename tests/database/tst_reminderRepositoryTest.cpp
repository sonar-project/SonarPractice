#include "tst_reminderRepositoryTest.h"

#include "ReminderDayEntry.h"
#include "MediaFile.h"
#include "PracticeAsset.h"
#include "Reminder.h"
#include "ReminderCondition.h"
#include "ReminderDayEntry.h"
#include "TestConstants.h"
#include "User.h"

#include <QDate>
#include <QTest>

void TestReminderRepository::init() {
    setUp();

    User admin;
    admin.name = QStringLiteral("admin");
    admin.role = QStringLiteral("admin");
    QVERIFY(m_userRepo.createUser(admin).has_value());
}

void TestReminderRepository::cleanup() { tearDown(); }

std::optional<qlonglong> TestReminderRepository::createSong() {
    const auto artistId = createTestArtist(QStringLiteral("Test Artist"));
    if (!artistId.has_value()) {
        return std::nullopt;
    }

    const auto tuningId = createTestTuning(QStringLiteral("E Standard"));
    if (!tuningId.has_value()) {
        return std::nullopt;
    }

    return createTestSong(*artistId, *tuningId, QStringLiteral("Scale Practice"), TestBpm::kSecondary);
}

std::optional<qlonglong> TestReminderRepository::createGuitarProFile(qlonglong songId,
                                                                     const QString &path) {
    MediaFile file;
    file.songId = songId;
    file.filePath = path;
    file.fileType = path.section(QLatin1Char('.'), -1);
    file.mediaKind = MediaKind::GuitarPro;
    file.sourceType = MediaSourceType::Local;
    file.canBePracticed = true;
    return m_mediaFileRepo.createMediaFile(file);
}

std::optional<qlonglong> TestReminderRepository::insertPracticeAssetWithGuitarPro(qlonglong songId,
                                                                                  qlonglong guitarProId) {
    PracticeAsset asset;
    asset.songId = songId;
    asset.guitarProId = guitarProId;
    return m_practiceAssetRepo.upsert(asset);
}

void TestReminderRepository::testCreateAndGetReminder() {
    const auto songId = createSong();
    QVERIFY(songId.has_value());

    Reminder reminder;
    reminder.songId = *songId;
    reminder.title = QStringLiteral("Warmup");
    reminder.reminderDate = QDate(TestDates::kYear, TestDates::kMay, TestDates::kDay28);
    reminder.isActive = true;

    const auto id = m_reminderRepo.createReminder(reminder);
    QVERIFY(id.has_value());

    const auto loaded = m_reminderRepo.getReminder(*id);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->songId, *songId);
    QCOMPARE(loaded->title, QStringLiteral("Warmup"));
    QCOMPARE(loaded->reminderDate, QDate(TestDates::kYear, TestDates::kMay, TestDates::kDay28));
    QVERIFY(loaded->isActive);
}

void TestReminderRepository::testListForSong() {
    const auto songId = createSong();
    QVERIFY(songId.has_value());

    Reminder first;
    first.songId = *songId;
    first.isDaily = true;
    QVERIFY(m_reminderRepo.createReminder(first).has_value());

    Reminder second;
    second.songId = *songId;
    second.reminderDate = QDate::currentDate();
    QVERIFY(m_reminderRepo.createReminder(second).has_value());

    const QList<Reminder> list = m_reminderRepo.listForSong(*songId);
    QCOMPARE(list.size(), 2);
}

void TestReminderRepository::testListForPracticeAsset() {
    const auto songId = createSong();
    QVERIFY(songId.has_value());

    const auto gpxId = createGuitarProFile(*songId, QStringLiteral("/tmp/Song1.gpx"));
    const auto gpId = createGuitarProFile(*songId, QStringLiteral("/tmp/Song2.gp"));
    QVERIFY(gpxId.has_value());
    QVERIFY(gpId.has_value());

    const auto assetGpx = insertPracticeAssetWithGuitarPro(*songId, *gpxId);
    const auto assetGp = insertPracticeAssetWithGuitarPro(*songId, *gpId);
    QVERIFY(assetGpx.has_value());
    QVERIFY(assetGp.has_value());

    Reminder gpxReminder;
    gpxReminder.songId = *songId;
    gpxReminder.practiceAssetId = *assetGpx;
    gpxReminder.title = QStringLiteral("GPX warmup");
    QVERIFY(m_reminderRepo.createReminder(gpxReminder).has_value());

    Reminder gpReminder;
    gpReminder.songId = *songId;
    gpReminder.practiceAssetId = *assetGp;
    gpReminder.title = QStringLiteral("GP drill");
    QVERIFY(m_reminderRepo.createReminder(gpReminder).has_value());

    const QList<Reminder> gpxList = m_reminderRepo.listForPracticeAsset(*assetGpx);
    QCOMPARE(gpxList.size(), 1);
    QCOMPARE(gpxList.first().title, QStringLiteral("GPX warmup"));

    const QList<Reminder> gpList = m_reminderRepo.listForPracticeAsset(*assetGp);
    QCOMPARE(gpList.size(), 1);
    QCOMPARE(gpList.first().title, QStringLiteral("GP drill"));
}

void TestReminderRepository::testListForSongExcludesPracticeAssetScopedReminders() {
    const auto songId = createSong();
    QVERIFY(songId.has_value());

    const auto gpxId = createGuitarProFile(*songId, QStringLiteral("/tmp/Song1.gpx"));
    QVERIFY(gpxId.has_value());

    const auto assetId = insertPracticeAssetWithGuitarPro(*songId, *gpxId);
    QVERIFY(assetId.has_value());

    Reminder legacy;
    legacy.songId = *songId;
    legacy.title = QStringLiteral("Legacy");
    QVERIFY(m_reminderRepo.createReminder(legacy).has_value());

    Reminder scoped;
    scoped.songId = *songId;
    scoped.practiceAssetId = *assetId;
    scoped.title = QStringLiteral("Scoped");
    QVERIFY(m_reminderRepo.createReminder(scoped).has_value());

    const QList<Reminder> songList = m_reminderRepo.listForSong(*songId);
    QCOMPARE(songList.size(), 1);
    QCOMPARE(songList.first().title, QStringLiteral("Legacy"));
}

void TestReminderRepository::testListForDateDailyAndOnce() {
    const auto songId = createSong();
    QVERIFY(songId.has_value());

    Reminder daily;
    daily.songId = *songId;
    daily.isDaily = true;
    QVERIFY(m_reminderRepo.createReminder(daily).has_value());

    Reminder once;
    once.songId = *songId;
    once.reminderDate = QDate(2026, 6, 1);
    QVERIFY(m_reminderRepo.createReminder(once).has_value());

    const QList<Reminder> onJune1 = m_reminderRepo.listForDate(QDate(2026, 6, 1));
    QCOMPARE(onJune1.size(), 2);

    const QList<Reminder> onJune2 = m_reminderRepo.listForDate(QDate(2026, 6, 2));
    QCOMPARE(onJune2.size(), 1);
    QVERIFY(onJune2.first().isDaily);
}

void TestReminderRepository::testListForDateWithSongJoin() {
    const auto songId = createSong();
    QVERIFY(songId.has_value());

    Reminder daily;
    daily.songId = *songId;
    daily.isDaily = true;
    daily.title = QStringLiteral("Daily drill");
    QVERIFY(m_reminderRepo.createReminder(daily).has_value());

    const QList<ReminderDayEntry> entries = m_reminderRepo.listForDateWithSong(QDate(2026, 6, 2));
    QCOMPARE(entries.size(), 1);
    QCOMPARE(entries.first().reminder.title, QStringLiteral("Daily drill"));
    QVERIFY(!entries.first().songTitle.isEmpty());
}

void TestReminderRepository::testUpdateAndDeleteReminder() {
    const auto songId = createSong();
    QVERIFY(songId.has_value());

    Reminder reminder;
    reminder.songId = *songId;
    reminder.title = QStringLiteral("Old");
    const auto id = m_reminderRepo.createReminder(reminder);
    QVERIFY(id.has_value());

    Reminder updated = *m_reminderRepo.getReminder(*id);
    updated.title = QStringLiteral("New");
    updated.isActive = false;
    QVERIFY(m_reminderRepo.updateReminder(updated));

    const auto loaded = m_reminderRepo.getReminder(*id);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->title, QStringLiteral("New"));
    QVERIFY(!loaded->isActive);

    QVERIFY(m_reminderRepo.deleteReminder(*id));
    QVERIFY(!m_reminderRepo.getReminder(*id).has_value());
}

void TestReminderRepository::testCreateConditionForReminder() {
    const auto songId = createSong();
    QVERIFY(songId.has_value());

    Reminder reminder;
    reminder.songId = *songId;
    const auto reminderId = m_reminderRepo.createReminder(reminder);
    QVERIFY(reminderId.has_value());

    ReminderCondition condition;
    condition.reminderId = *reminderId;
    condition.startBar = 1;
    condition.endBar = 8;
    condition.minBpm = 80;
    condition.minMinutes = 15;

    const auto conditionId = m_conditionRepo.createCondition(condition);
    QVERIFY(conditionId.has_value());

    const QList<ReminderCondition> list = m_conditionRepo.listForReminder(*reminderId);
    QCOMPARE(list.size(), 1);
    QCOMPARE(list.first().startBar, 1);
    QCOMPARE(list.first().endBar, 8);
    QCOMPARE(list.first().minBpm, 80);
    QCOMPARE(list.first().minMinutes, 15);
}

QTEST_MAIN(TestReminderRepository)
