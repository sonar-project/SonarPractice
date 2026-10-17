#ifndef TST_PRACTICETRACKERTEST_H
#define TST_PRACTICETRACKERTEST_H

#include <QObject>
#include <optional>

#include "SqliteConnection.h"
#include "SqliteMediaFileRepository.h"
#include "SqlitePracticeAssetRepository.h"
#include "SqlitePracticeJournalRepository.h"
#include "SqliteSongRepository.h"

class TestPracticeTracker : public QObject {
    Q_OBJECT

  private slots:
    void init();
    void cleanup();

    void testComputeStreak();
    void testStartStopDoesNotPersistUntilStop();
    void testStopWithDurationCreatesJournalEntry();
    void testStopRejectsInvalidBarRange();
    void testDurationMinutesFromSeconds();

  private:
    /// Legt Song + MediaFile + practice_asset an und gibt die asset_id zurück.
    std::optional<qlonglong> createAsset();
    std::optional<qlonglong> insertPracticeAsset(qlonglong songId, qlonglong mediaFileId);

    SqliteConnection m_connector{QStringLiteral("PracticeTrackerTestDb")};
    SqliteSongRepository m_songRepo{m_connector};
    SqliteMediaFileRepository m_mediaRepo{m_connector};
    SqlitePracticeJournalRepository m_journalRepo{m_connector};
    SqlitePracticeAssetRepository m_practiceAssetRepo{m_connector};
};

#endif // TST_PRACTICETRACKERTEST_H
