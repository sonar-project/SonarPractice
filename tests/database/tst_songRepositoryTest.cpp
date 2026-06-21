#include "tst_songRepositoryTest.h"

#include "Song.h"
#include "SqliteConnection.h"
#include "SqliteSongRepository.h"
#include "TestConstants.h"

#include <QTest>

void TestSongRepository::init() { setUp(); }

void TestSongRepository::cleanup() { tearDown(); }

void TestSongRepository::testCreateSong() {
    const std::optional<qlonglong> artistId = createTestArtist(QStringLiteral("Metallica"));
    const std::optional<qlonglong> tuningId = createTestTuning(QStringLiteral("E Standard"));
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    Song song;
    song.title = QStringLiteral("Nothing Else Matters");
    song.baseBpm = TestBpm::kDefaultSong;
    song.artistId = *artistId;
    song.tuningId = *tuningId;

    const std::optional<qlonglong> songId = m_songRepo.createSong(song);

    QVERIFY(songId.has_value());
    QVERIFY(*songId > 0);
}

void TestSongRepository::testGetSong() {
    const std::optional<qlonglong> artistId = createTestArtist();
    const std::optional<qlonglong> tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    const std::optional<qlonglong> songId = createTestSong(*artistId, *tuningId, QStringLiteral("Test Song"), 140);
    QVERIFY(songId.has_value());

    const std::optional<Song> loadedSong = m_songRepo.getSong(*songId);
    QVERIFY(loadedSong.has_value());

    QCOMPARE(loadedSong->id, *songId);
    QCOMPARE(loadedSong->title, QStringLiteral("Test Song"));
    QCOMPARE(loadedSong->baseBpm, 140);
}

void TestSongRepository::testUpdateSong() {
    const std::optional<qlonglong> artistId = createTestArtist();
    const std::optional<qlonglong> tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    const std::optional<qlonglong> songId =
        createTestSong(*artistId, *tuningId, QStringLiteral("Old Title"), TestBpm::kSecondary);
    QVERIFY(songId.has_value());

    const std::optional<Song> existingSong = m_songRepo.getSong(*songId);
    QVERIFY(existingSong.has_value());

    Song song = *existingSong;
    song.title = QStringLiteral("New Title");
    song.baseBpm = 160;

    QVERIFY(m_songRepo.updateSong(song));

    const std::optional<Song> loadedSong = m_songRepo.getSong(*songId);
    QVERIFY(loadedSong.has_value());
    QCOMPARE(loadedSong->title, QStringLiteral("New Title"));
    QCOMPARE(loadedSong->baseBpm, 160);
}

void TestSongRepository::testDeleteSong() {
    const std::optional<qlonglong> artistId = createTestArtist();
    const std::optional<qlonglong> tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    const std::optional<qlonglong> songId = createTestSong(*artistId, *tuningId);
    QVERIFY(songId.has_value());

    QVERIFY(m_songRepo.deleteSong(*songId));

    const std::optional<Song> loadedSong = m_songRepo.getSong(*songId);
    QVERIFY(!loadedSong.has_value());
}

void TestSongRepository::testGetAllSongsEmpty() {
    const QList<Song> songs = m_songRepo.getAllSongs();
    QVERIFY(songs.isEmpty());
}

void TestSongRepository::testGetAllSongs() {
    const std::optional<qlonglong> artistId = createTestArtist();
    const std::optional<qlonglong> tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    QVERIFY(createTestSong(*artistId, *tuningId, QStringLiteral("Song A"), TestBpm::kDefaultSong).has_value());
    QVERIFY(createTestSong(*artistId, *tuningId, QStringLiteral("Song B"), 140).has_value());

    const QList<Song> songs = m_songRepo.getAllSongs();
    QCOMPARE(songs.size(), 2);
}

void TestSongRepository::testGetSongNotFound() {
    const std::optional<Song> loadedSong = m_songRepo.getSong(99999);
    QVERIFY(!loadedSong.has_value());
}

void TestSongRepository::testCreateSongWithoutConnection() {
    SqliteConnection closedConnector(QStringLiteral("ClosedSongDb"));
    SqliteSongRepository repo(closedConnector);

    Song song;
    song.title = QStringLiteral("Ghost Song");
    song.baseBpm = TestBpm::kDefaultSong;
    song.artistId = 1;
    song.tuningId = 1;

    const std::optional<qlonglong> songId = repo.createSong(song);
    QVERIFY(!songId.has_value());
}

QTEST_MAIN(TestSongRepository)
