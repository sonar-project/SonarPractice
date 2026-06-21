#include "tst_practiceAssetRepositoryTest.h"

#include "JournalEntry.h"
#include "MediaFile.h"
#include "PracticeAsset.h"
#include "TestConstants.h"

#include <QDateTime>
#include <QTest>
#include <QTime>

void TestPracticeAssetRepository::init() { setUp(); }

void TestPracticeAssetRepository::cleanup() { tearDown(); }

void TestPracticeAssetRepository::testUpsertSongOnlyAsset() {
    const auto artistId = createTestArtist();
    const auto tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    const auto songId = createTestSong(*artistId, *tuningId);
    QVERIFY(songId.has_value());

    PracticeAsset asset;
    asset.songId = *songId;
    const auto assetId = m_practiceAssetRepo.upsert(asset);
    QVERIFY(assetId.has_value());
    QVERIFY(*assetId > 0);

    const auto again = m_practiceAssetRepo.upsert(asset);
    QVERIFY(again.has_value());
    QCOMPARE(*again, *assetId);
}

void TestPracticeAssetRepository::testUpsertCompositeNaturalKey() {
    const auto artistId = createTestArtist();
    const auto tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    const auto songId = createTestSong(*artistId, *tuningId);
    QVERIFY(songId.has_value());

    MediaFile gpx;
    gpx.songId = *songId;
    gpx.filePath = QStringLiteral("/tmp/composite.gpx");
    gpx.mediaKind = MediaKind::GuitarPro;
    gpx.sourceType = MediaSourceType::Local;
    const auto gpxId = m_mediaFileRepo.createMediaFile(gpx);
    QVERIFY(gpxId.has_value());

    PracticeAsset asset;
    asset.songId = *songId;
    asset.guitarProId = *gpxId;

    const auto firstId = m_practiceAssetRepo.upsert(asset);
    QVERIFY(firstId.has_value());

    const auto secondId = m_practiceAssetRepo.upsert(asset);
    QVERIFY(secondId.has_value());
    QCOMPARE(*secondId, *firstId);
}

void TestPracticeAssetRepository::testGetById() {
    const auto artistId = createTestArtist();
    const auto tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    const auto songId = createTestSong(*artistId, *tuningId);
    QVERIFY(songId.has_value());

    MediaFile audio;
    audio.songId = *songId;
    audio.filePath = QStringLiteral("/tmp/track.mp3");
    audio.mediaKind = MediaKind::Audio;
    audio.sourceType = MediaSourceType::Local;
    const auto audioId = m_mediaFileRepo.createMediaFile(audio);
    QVERIFY(audioId.has_value());

    PracticeAsset asset;
    asset.songId = *songId;
    asset.audioId = *audioId;
    const auto createdId = m_practiceAssetRepo.upsert(asset);
    QVERIFY(createdId.has_value());

    const auto loaded = m_practiceAssetRepo.getById(*createdId);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->songId, *songId);
    QCOMPARE(loaded->audioId, *audioId);
    QCOMPARE(loaded->primaryMediaId(), *audioId);
}

void TestPracticeAssetRepository::testLastPrimaryMediaIdForSong() {
    qlonglong userId = 0;
    qlonglong unusedUserId = 0;
    createTestUsers(userId, unusedUserId);

    const auto artistId = createTestArtist();
    const auto tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    const auto songId = createTestSong(*artistId, *tuningId);
    QVERIFY(songId.has_value());

    MediaFile gpx;
    gpx.songId = *songId;
    gpx.filePath = QStringLiteral("/tmp/journal.gpx");
    gpx.mediaKind = MediaKind::GuitarPro;
    gpx.sourceType = MediaSourceType::Local;
    const auto gpxId = m_mediaFileRepo.createMediaFile(gpx);
    QVERIFY(gpxId.has_value());

    PracticeAsset asset;
    asset.songId = *songId;
    asset.guitarProId = *gpxId;
    const auto assetId = m_practiceAssetRepo.upsert(asset);
    QVERIFY(assetId.has_value());

    JournalEntry entry;
    entry.userId = userId;
    entry.assetId = *assetId;
    entry.practiceDate = QDateTime(QDate(TestDates::kYear, TestDates::kJune, TestDates::kDay1),
                                   QTime(TestDates::kHour10, TestDates::kMinute0));
    QVERIFY(m_journalRepo.createEntry(entry).has_value());

    QCOMPARE(m_practiceAssetRepo.lastPrimaryMediaIdForSong(*songId), *gpxId);
}

QTEST_MAIN(TestPracticeAssetRepository)
