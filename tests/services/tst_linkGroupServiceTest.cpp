#include "tst_linkGroupServiceTest.h"

#include "DatabaseSchema.h"
#include "MediaFile.h"
#include "MediaFileMapping.h"
#include "Song.h"

#include <QSqlDatabase>
#include <QTest>

void TestLinkGroupService::init() {
    QVERIFY(m_connector.open(QStringLiteral(":memory:")));
    DatabaseSchema schema(m_connector);
    QVERIFY(schema.createAllTables());
}

void TestLinkGroupService::cleanup() {
    m_connector.close();
    QSqlDatabase::removeDatabase(m_connector.connectionName());
}

std::optional<qlonglong> TestLinkGroupService::createSong(const QString &title) {
    Song song;
    song.title = title;
    return m_songRepo.createSong(song);
}

std::optional<qlonglong> TestLinkGroupService::createMedia(qlonglong songId, const QString &path,
                                                           MediaKind kind,
                                                           const QString &importRoot,
                                                           const QString &relativePath) {
    MediaFile media;
    media.songId = songId;
    media.filePath = path;
    media.fileType = QStringLiteral("gp5");
    media.mediaKind = kind;
    media.sourceType = MediaSourceType::Local;
    media.importRoot = importRoot;
    media.sourceRelativePath = relativePath;
    media.canBePracticed = kind == MediaKind::GuitarPro;
    return m_mediaRepo.createMediaFile(media);
}

void TestLinkGroupService::testCreateGroupLinksSecondaryMedia() {
    const std::optional<qlonglong> primarySongId = createSong(QStringLiteral("Primary"));
    const std::optional<qlonglong> secondarySongId = createSong(QStringLiteral("Secondary"));
    QVERIFY(primarySongId.has_value());
    QVERIFY(secondarySongId.has_value());

    const std::optional<qlonglong> primaryMediaId =
        createMedia(*primarySongId, QStringLiteral("/tmp/primary.gp5"), MediaKind::GuitarPro);
    const std::optional<qlonglong> secondaryMediaId =
        createMedia(*secondarySongId, QStringLiteral("/tmp/secondary.mp4"), MediaKind::Video);
    QVERIFY(primaryMediaId.has_value());
    QVERIFY(secondaryMediaId.has_value());

    const std::optional<qlonglong> groupId = m_service.createGroup(
        QStringLiteral("Test Group"), *primarySongId, *primaryMediaId, {*secondaryMediaId});
    QVERIFY(groupId.has_value());

    const QList<MediaFile> linked = m_fileRelationRepo.getLinkedMedia(*primaryMediaId);
    QCOMPARE(linked.size(), 1);
    QCOMPARE(linked.first().id, *secondaryMediaId);
    QVERIFY(m_fileRelationRepo.isSecondaryMedia(*secondaryMediaId));
}

void TestLinkGroupService::testDissolveGroupRemovesRelations() {
    const std::optional<qlonglong> primarySongId = createSong(QStringLiteral("Primary"));
    const std::optional<qlonglong> secondarySongId = createSong(QStringLiteral("Secondary"));
    QVERIFY(primarySongId.has_value());
    QVERIFY(secondarySongId.has_value());

    const std::optional<qlonglong> primaryMediaId =
        createMedia(*primarySongId, QStringLiteral("/tmp/primary.gp5"), MediaKind::GuitarPro);
    const std::optional<qlonglong> secondaryMediaId =
        createMedia(*secondarySongId, QStringLiteral("/tmp/secondary.mp4"), MediaKind::Video);
    QVERIFY(primaryMediaId.has_value());
    QVERIFY(secondaryMediaId.has_value());

    const std::optional<qlonglong> groupId = m_service.createGroup(
        QStringLiteral("Temp Group"), *primarySongId, *primaryMediaId, {*secondaryMediaId});
    QVERIFY(groupId.has_value());

    QVERIFY(m_service.dissolveGroup(*groupId));
    QCOMPARE(m_fileRelationRepo.getLinkedMedia(*primaryMediaId).size(), 0);
    QVERIFY(!m_linkGroupRepo.getGroup(*groupId).has_value());
}

void TestLinkGroupService::testDissolveGroupForSongReturnsMemberIds() {
    const std::optional<qlonglong> primarySongId = createSong(QStringLiteral("Primary"));
    const std::optional<qlonglong> secondarySongId = createSong(QStringLiteral("Secondary"));
    QVERIFY(primarySongId.has_value());
    QVERIFY(secondarySongId.has_value());

    const std::optional<qlonglong> primaryMediaId =
        createMedia(*primarySongId, QStringLiteral("/tmp/primary.gp5"), MediaKind::GuitarPro);
    const std::optional<qlonglong> secondaryMediaId =
        createMedia(*secondarySongId, QStringLiteral("/tmp/secondary.mp4"), MediaKind::Video);
    QVERIFY(primaryMediaId.has_value());
    QVERIFY(secondaryMediaId.has_value());

    const std::optional<qlonglong> groupId = m_service.createGroup(
        QStringLiteral("Dissolve Members"), *primarySongId, *primaryMediaId, {*secondaryMediaId});
    QVERIFY(groupId.has_value());

    const QVariantList dissolvedIds = m_service.dissolveGroupForSong(*primarySongId);
    QCOMPARE(dissolvedIds.size(), 2);
    QCOMPARE(dissolvedIds.at(0).toLongLong(), *primarySongId);
    QCOMPARE(dissolvedIds.at(1).toLongLong(), *secondarySongId);
    QVERIFY(m_service.dissolveGroupForSong(*primarySongId).isEmpty());
}

void TestLinkGroupService::testAddSongsToExistingGroup() {
    const std::optional<qlonglong> primarySongId = createSong(QStringLiteral("Primary"));
    const std::optional<qlonglong> existingSecondaryId = createSong(QStringLiteral("Existing"));
    const std::optional<qlonglong> newSecondaryId = createSong(QStringLiteral("New File"));
    QVERIFY(primarySongId.has_value());
    QVERIFY(existingSecondaryId.has_value());
    QVERIFY(newSecondaryId.has_value());

    const std::optional<qlonglong> primaryMediaId =
        createMedia(*primarySongId, QStringLiteral("/tmp/primary.gp5"), MediaKind::GuitarPro);
    const std::optional<qlonglong> existingMediaId =
        createMedia(*existingSecondaryId, QStringLiteral("/tmp/existing.mp4"), MediaKind::Video);
    const std::optional<qlonglong> newMediaId =
        createMedia(*newSecondaryId, QStringLiteral("/tmp/new.pdf"), MediaKind::Document);
    QVERIFY(primaryMediaId.has_value());
    QVERIFY(existingMediaId.has_value());
    QVERIFY(newMediaId.has_value());

    const std::optional<qlonglong> groupId = m_service.createGroup(
        QStringLiteral("Existing Group"), *primarySongId, *primaryMediaId, {*existingMediaId});
    QVERIFY(groupId.has_value());

    QVERIFY(m_service.addSongsToGroup(*groupId, {*newSecondaryId}));

    const QList<MediaFile> linked = m_fileRelationRepo.getLinkedMedia(*primaryMediaId);
    QCOMPARE(linked.size(), 2);
    QVERIFY(m_fileRelationRepo.isSecondaryMedia(*newMediaId));
}

QTEST_MAIN(TestLinkGroupService)
