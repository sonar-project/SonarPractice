#include "DatabaseTestFixture.h"

#include "Artist.h"
#include "DatabaseSchema.h"
#include "PracticeAsset.h"
#include "Song.h"
#include "Tuning.h"
#include "User.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTest>

DatabaseTestFixture::DatabaseTestFixture(QString connectionName)
    : m_connector(std::move(connectionName)) {}

void DatabaseTestFixture::setUp() {
    QVERIFY(m_connector.open(QStringLiteral(":memory:")));

    DatabaseSchema schema(m_connector);
    QVERIFY(schema.createAllTables());
}

void DatabaseTestFixture::tearDown() {
    const QString name = m_connector.connectionName();
    m_connector.close();
    QSqlDatabase::removeDatabase(name);
}

void DatabaseTestFixture::createTestUsers(qlonglong &adminId, qlonglong &noAdminId) {
    User admUser;
    admUser.name = QStringLiteral("TestAdmin");
    admUser.role = QStringLiteral("admin");

    User otherUser;
    otherUser.name = QStringLiteral("StudentUser");
    otherUser.role = QStringLiteral("student");

    const std::optional<qlonglong> createdAdminId = m_userRepo.createUser(admUser);
    const std::optional<qlonglong> createdNoAdminId = m_userRepo.createUser(otherUser);

    QVERIFY(createdAdminId.has_value());
    QVERIFY(createdNoAdminId.has_value());

    adminId = *createdAdminId;
    noAdminId = *createdNoAdminId;
}

std::optional<qlonglong> DatabaseTestFixture::createTestArtist(const QString &name) {
    Artist artist;
    artist.name = name;
    return m_artistRepo.createArtist(artist);
}

std::optional<qlonglong> DatabaseTestFixture::createTestTuning(const QString &name) {
    Tuning tuning;
    tuning.name = name;
    return m_tuningRepo.createTuning(tuning);
}

std::optional<qlonglong> DatabaseTestFixture::createTestSong(qlonglong artistId, qlonglong tuningId,
                                                             const QString &title, int baseBpm) {
    Song song;
    song.title = title;
    song.baseBpm = baseBpm;
    song.artistId = artistId;
    song.tuningId = tuningId;
    return m_songRepo.createSong(song);
}

std::optional<qlonglong> DatabaseTestFixture::createTestPracticeAsset(qlonglong songId) {
    PracticeAsset asset;
    asset.songId = songId;
    return m_practiceAssetRepo.upsert(asset);
}
