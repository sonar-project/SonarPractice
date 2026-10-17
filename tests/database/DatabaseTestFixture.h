#ifndef DATABASETESTFIXTURE_H
#define DATABASETESTFIXTURE_H

#include "TestConstants.h"

#include "SqliteArtistRepository.h"
#include "SqliteConnection.h"
#include "SqliteMediaFileRepository.h"
#include "SqlitePracticeAssetRepository.h"
#include "SqlitePracticeJournalRepository.h"
#include "SqliteSongRepository.h"
#include "SqliteTuningRepository.h"
#include "SqliteUserRepository.h"

#include <optional>

class QString;

/**
 * Shared in-memory SQLite setup and seed helpers for database integration tests.
 * Test classes inherit protected and call setUp()/tearDown() from init()/cleanup().
 */
class DatabaseTestFixture {
  protected:
    explicit DatabaseTestFixture(QString connectionName);

    void setUp();
    void tearDown();

    void createTestUsers(qlonglong &adminId, qlonglong &noAdminId);
    [[nodiscard]] std::optional<qlonglong>
    createTestArtist(const QString &name = QStringLiteral("Test Artist"));
    [[nodiscard]] std::optional<qlonglong>
    createTestTuning(const QString &name = QStringLiteral("E Standard"));
    [[nodiscard]] std::optional<qlonglong>
    createTestSong(qlonglong artistId, qlonglong tuningId,
                   const QString &title = QStringLiteral("Test Song"),
                   int baseBpm = TestBpm::kDefaultSong);
    [[nodiscard]] std::optional<qlonglong> createTestPracticeAsset(qlonglong songId);

    SqliteConnection m_connector;
    SqliteUserRepository m_userRepo{m_connector};
    SqliteArtistRepository m_artistRepo{m_connector};
    SqliteTuningRepository m_tuningRepo{m_connector};
    SqliteSongRepository m_songRepo{m_connector};
    SqliteMediaFileRepository m_mediaFileRepo{m_connector};
    SqlitePracticeJournalRepository m_journalRepo{m_connector};
    SqlitePracticeAssetRepository m_practiceAssetRepo{m_connector};
};

#endif // DATABASETESTFIXTURE_H
