#include "tst_artistRepositoryTest.h"

#include "Artist.h"

#include <QTest>

void TestArtistRepository::init() { setUp(); }

void TestArtistRepository::cleanup() { tearDown(); }

void TestArtistRepository::testCreateArtist() {
    Artist firstArtist;
    firstArtist.name = QStringLiteral("Metallica");

    Artist secondArtist;
    secondArtist.name = QStringLiteral("Megadeth");

    const std::optional<qlonglong> firstId = m_artistRepo.createArtist(firstArtist);
    const std::optional<qlonglong> secondId = m_artistRepo.createArtist(secondArtist);

    QVERIFY(firstId.has_value());
    QVERIFY(secondId.has_value());
    QVERIFY(*firstId > 0);
    QVERIFY(*secondId > 0);
    QVERIFY(*firstId != *secondId);
}

void TestArtistRepository::testGetArtist() {
    Artist artist;
    artist.name = QStringLiteral("Metallica");

    const std::optional<qlonglong> artistId = m_artistRepo.createArtist(artist);
    QVERIFY(artistId.has_value());
    QVERIFY(*artistId > 0);

    const std::optional<Artist> loadedArtist = m_artistRepo.getArtist(*artistId);
    QVERIFY(loadedArtist.has_value());
    QCOMPARE(loadedArtist->name, QStringLiteral("Metallica"));
}

void TestArtistRepository::testUpdateArtist() {
    Artist artist;
    artist.name = QStringLiteral("Old Name");

    const std::optional<qlonglong> artistId = m_artistRepo.createArtist(artist);
    QVERIFY(artistId.has_value());

    Artist updatedArtist;
    updatedArtist.id = *artistId;
    updatedArtist.name = QStringLiteral("New Name");

    QVERIFY(m_artistRepo.updateArtist(updatedArtist));

    const std::optional<Artist> loadedArtist = m_artistRepo.getArtist(*artistId);
    QVERIFY(loadedArtist.has_value());
    QCOMPARE(loadedArtist->name, QStringLiteral("New Name"));
}

void TestArtistRepository::testDeleteArtist() {
    Artist artist;
    artist.name = QStringLiteral("To Delete");

    const std::optional<qlonglong> artistId = m_artistRepo.createArtist(artist);
    QVERIFY(artistId.has_value());

    QVERIFY(m_artistRepo.deleteArtist(*artistId));

    const std::optional<Artist> loadedArtist = m_artistRepo.getArtist(*artistId);
    QVERIFY(!loadedArtist.has_value());
}

void TestArtistRepository::testGetArtistNotFound() {
    const std::optional<Artist> artist = m_artistRepo.getArtist(99999);
    QVERIFY(!artist.has_value());
}

void TestArtistRepository::testCreateArtistDuplicateName() {
    Artist firstArtist;
    firstArtist.name = QStringLiteral("Duplicate Artist");

    Artist secondArtist;
    secondArtist.name = QStringLiteral("Duplicate Artist");

    const std::optional<qlonglong> firstId = m_artistRepo.createArtist(firstArtist);
    const std::optional<qlonglong> secondId = m_artistRepo.createArtist(secondArtist);

    QVERIFY(firstId.has_value());
    QVERIFY(!secondId.has_value());
}

void TestArtistRepository::testCreateArtistEmptyName() {
    Artist artist;
    artist.name = QStringLiteral("");

    const std::optional<qlonglong> artistId = m_artistRepo.createArtist(artist);
    QVERIFY(!artistId.has_value());
}

QTEST_MAIN(TestArtistRepository)
