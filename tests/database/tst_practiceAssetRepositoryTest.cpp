#include "tst_practiceAssetRepositoryTest.h"

#include "JournalEntry.h"
#include "MediaFile.h"
#include "PracticeAsset.h"
#include "PracticeSyncMarker.h"
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

void TestPracticeAssetRepository::testSyncMarkersRoundTrip() {
    const auto artistId = createTestArtist();
    const auto tuningId = createTestTuning();
    const auto songId = createTestSong(*artistId, *tuningId);
    QVERIFY(songId.has_value());

    PracticeAsset asset;
    asset.songId = *songId;
    const auto assetId = m_practiceAssetRepo.upsert(asset);
    QVERIFY(assetId.has_value());
    QVERIFY(m_practiceAssetRepo.audioLeadIn(*assetId));
    QVERIFY(m_practiceAssetRepo.setAudioLeadIn(*assetId, false));
    QVERIFY(!m_practiceAssetRepo.audioLeadIn(*assetId));

    PracticeSyncMarker first;
    first.assetId = *assetId;
    first.bar = 1;
    first.beat = 1;
    first.tick = 0;
    first.audioMs = 1800;
    QVERIFY(m_practiceAssetRepo.saveSyncMarker(first));

    PracticeSyncMarker drift;
    drift.assetId = *assetId;
    drift.bar = 17;
    drift.beat = 1;
    drift.tick = 15360;
    drift.audioMs = 34200;
    QVERIFY(m_practiceAssetRepo.saveSyncMarker(drift));

    first.audioMs = 1760;
    QVERIFY(m_practiceAssetRepo.saveSyncMarker(first));

    const auto markers = m_practiceAssetRepo.listSyncMarkers(*assetId);
    QCOMPARE(static_cast<int>(markers.size()), 2);
    QCOMPARE(markers[0].bar, 1);
    QCOMPARE(markers[0].audioMs, static_cast<qint64>(1760));
    QCOMPARE(markers[1].bar, 17);
    QCOMPARE(markers[1].tick, 15360);

    QVERIFY(m_practiceAssetRepo.deleteSyncMarker(*assetId, 17, 1));
    QCOMPARE(static_cast<int>(m_practiceAssetRepo.listSyncMarkers(*assetId).size()), 1);
}

QTEST_MAIN(TestPracticeAssetRepository)
