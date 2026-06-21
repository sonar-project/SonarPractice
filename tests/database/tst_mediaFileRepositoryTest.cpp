#include "tst_mediaFileRepositoryTest.h"

#include "MediaFile.h"
#include "TestConstants.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTest>

void TestMediaFileRepository::init() { setUp(); }

void TestMediaFileRepository::cleanup() { tearDown(); }

void TestMediaFileRepository::testMediaFilesTable() {
    const std::optional<qlonglong> artistId = createTestArtist();
    const std::optional<qlonglong> tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    const std::optional<qlonglong> songId = createTestSong(*artistId, *tuningId);
    QVERIFY(songId.has_value());

    QSqlDatabase db = QSqlDatabase::database(m_connector.connectionName());
    QSqlQuery query(db);

    query.prepare(
        QStringLiteral("INSERT INTO media_files (song_id, file_path, file_type, media_kind, file_size, file_hash, "
                       "source_type, is_managed, can_be_practiced) "
                       "VALUES (:song_id, :file_path, :file_type, :media_kind, :file_size, :file_hash, "
                       ":source_type, :is_managed, :can_be_practiced)"));
    query.bindValue(QStringLiteral(":song_id"), *songId);
    query.bindValue(QStringLiteral(":file_path"), QStringLiteral("/tmp/test_song.gp"));
    query.bindValue(QStringLiteral(":file_type"), QStringLiteral("gp"));
    query.bindValue(QStringLiteral(":media_kind"), QStringLiteral("guitarpro"));
    query.bindValue(QStringLiteral(":file_size"), 1024);
    query.bindValue(QStringLiteral(":file_hash"), QStringLiteral("abc123hash"));
    query.bindValue(QStringLiteral(":source_type"), QStringLiteral("LOCAL"));
    query.bindValue(QStringLiteral(":is_managed"), 0);
    query.bindValue(QStringLiteral(":can_be_practiced"), 1);

    QVERIFY2(query.exec(), qPrintable(query.lastError().text()));

    QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM media_files")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
}

void TestMediaFileRepository::testMediaFilesCascadeDelete() {
    const std::optional<qlonglong> artistId = createTestArtist();
    const std::optional<qlonglong> tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    const std::optional<qlonglong> songId = createTestSong(*artistId, *tuningId);
    QVERIFY(songId.has_value());

    QSqlDatabase db = QSqlDatabase::database(m_connector.connectionName());
    QSqlQuery query(db);

    query.prepare(
        QStringLiteral("INSERT INTO media_files (song_id, file_path, file_type, media_kind, file_size, file_hash, "
                       "source_type, is_managed, can_be_practiced) "
                       "VALUES (:song_id, :file_path, :file_type, :media_kind, :file_size, :file_hash, "
                       ":source_type, :is_managed, :can_be_practiced)"));
    query.bindValue(QStringLiteral(":song_id"), *songId);
    query.bindValue(QStringLiteral(":file_path"), QStringLiteral("/tmp/cascade_test.gp"));
    query.bindValue(QStringLiteral(":file_type"), QStringLiteral("gp"));
    query.bindValue(QStringLiteral(":media_kind"), QStringLiteral("guitarpro"));
    query.bindValue(QStringLiteral(":file_size"), 512);
    query.bindValue(QStringLiteral(":file_hash"), QStringLiteral("cascadehash123"));
    query.bindValue(QStringLiteral(":source_type"), QStringLiteral("LOCAL"));
    query.bindValue(QStringLiteral(":is_managed"), 0);
    query.bindValue(QStringLiteral(":can_be_practiced"), 1);
    QVERIFY2(query.exec(), qPrintable(query.lastError().text()));

    QVERIFY(m_songRepo.deleteSong(*songId));

    QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM media_files WHERE song_id = ")
                       + QString::number(*songId)));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 0);
}

void TestMediaFileRepository::testGetMediaFilesBySongId() {
    const std::optional<qlonglong> artistId = createTestArtist();
    const std::optional<qlonglong> tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    const std::optional<qlonglong> songId =
        createTestSong(*artistId, *tuningId, QStringLiteral("Multi Asset Song"));
    QVERIFY(songId.has_value());

    MediaFile gpFile;
    gpFile.songId = *songId;
    gpFile.filePath = QStringLiteral("/tmp/multi_asset.gp5");
    gpFile.fileType = QStringLiteral("gp5");
    gpFile.mediaKind = MediaKind::GuitarPro;
    gpFile.sourceType = MediaSourceType::Local;
    gpFile.canBePracticed = true;

    MediaFile pdfFile;
    pdfFile.songId = *songId;
    pdfFile.filePath = QStringLiteral("/tmp/multi_asset.pdf");
    pdfFile.fileType = QStringLiteral("pdf");
    pdfFile.mediaKind = MediaKind::Document;
    pdfFile.sourceType = MediaSourceType::Local;

    QVERIFY(m_mediaFileRepo.createMediaFile(gpFile).has_value());
    QVERIFY(m_mediaFileRepo.createMediaFile(pdfFile).has_value());

    const QList<MediaFile> files = m_mediaFileRepo.getMediaFilesBySongId(*songId);

    QCOMPARE(files.size(), 2);
    QCOMPARE(files.at(0).mediaKind, MediaKind::GuitarPro);
    QCOMPARE(files.at(1).mediaKind, MediaKind::Document);
}

void TestMediaFileRepository::testMediaFileHasVideoRoundtrip() {
    const std::optional<qlonglong> artistId = createTestArtist();
    const std::optional<qlonglong> tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    const std::optional<qlonglong> songId =
        createTestSong(*artistId, *tuningId, QStringLiteral("Video Song"));
    QVERIFY(songId.has_value());

    MediaFile videoFile;
    videoFile.songId = *songId;
    videoFile.filePath = QStringLiteral("/tmp/lesson.mp4");
    videoFile.fileType = QStringLiteral("mp4");
    videoFile.mediaKind = MediaKind::Video;
    videoFile.hasVideo = true;
    videoFile.sourceType = MediaSourceType::Local;

    const std::optional<qlonglong> mediaId = m_mediaFileRepo.createMediaFile(videoFile);
    QVERIFY(mediaId.has_value());

    const std::optional<MediaFile> loaded = m_mediaFileRepo.getMediaFile(*mediaId);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->hasVideo, true);
    QCOMPARE(loaded->mediaKind, MediaKind::Video);

    MediaFile audioFile;
    audioFile.songId = *songId;
    audioFile.filePath = QStringLiteral("/tmp/backing.mp3");
    audioFile.fileType = QStringLiteral("mp3");
    audioFile.mediaKind = MediaKind::Audio;
    audioFile.hasAudio = true;
    audioFile.sourceType = MediaSourceType::Local;

    const std::optional<qlonglong> audioId = m_mediaFileRepo.createMediaFile(audioFile);
    QVERIFY(audioId.has_value());

    const std::optional<MediaFile> loadedAudio = m_mediaFileRepo.getMediaFile(*audioId);
    QVERIFY(loadedAudio.has_value());
    QCOMPARE(loadedAudio->hasVideo, false);
    QCOMPARE(loadedAudio->mediaKind, MediaKind::Audio);
}

void TestMediaFileRepository::testGetMediaFilesBySongIdEmpty() {
    const QList<MediaFile> files = m_mediaFileRepo.getMediaFilesBySongId(99999);
    QVERIFY(files.isEmpty());
}

void TestMediaFileRepository::testFirstMediaIdBySongIds() {
    qlonglong userId = 0;
    qlonglong unusedUserId = 0;
    createTestUsers(userId, unusedUserId);

    const std::optional<qlonglong> artistId = createTestArtist();
    const std::optional<qlonglong> tuningId = createTestTuning();
    QVERIFY(artistId.has_value());
    QVERIFY(tuningId.has_value());

    const std::optional<qlonglong> firstSongId =
        createTestSong(*artistId, *tuningId, QStringLiteral("Batch A"));
    const std::optional<qlonglong> secondSongId =
        createTestSong(*artistId, *tuningId, QStringLiteral("Batch B"));
    QVERIFY(firstSongId.has_value());
    QVERIFY(secondSongId.has_value());

    MediaFile firstFile;
    firstFile.songId = *firstSongId;
    firstFile.filePath = QStringLiteral("/tmp/batch_a.gp5");
    firstFile.fileType = QStringLiteral("gp5");
    firstFile.mediaKind = MediaKind::GuitarPro;
    firstFile.sourceType = MediaSourceType::Local;
    firstFile.canBePracticed = true;

    MediaFile secondFile;
    secondFile.songId = *secondSongId;
    secondFile.filePath = QStringLiteral("/tmp/batch_b.mp4");
    secondFile.fileType = QStringLiteral("mp4");
    secondFile.mediaKind = MediaKind::Video;
    secondFile.sourceType = MediaSourceType::Local;

    const std::optional<qlonglong> firstMediaId = m_mediaFileRepo.createMediaFile(firstFile);
    const std::optional<qlonglong> secondMediaId = m_mediaFileRepo.createMediaFile(secondFile);
    QVERIFY(firstMediaId.has_value());
    QVERIFY(secondMediaId.has_value());

    const QHash<qlonglong, qlonglong> mediaBySong =
        m_mediaFileRepo.firstMediaIdBySongIds({*firstSongId, *secondSongId, 99999});

    QCOMPARE(mediaBySong.size(), 2);
    QCOMPARE(mediaBySong.value(*firstSongId), *firstMediaId);
    QCOMPARE(mediaBySong.value(*secondSongId), *secondMediaId);
}

QTEST_MAIN(TestMediaFileRepository)
