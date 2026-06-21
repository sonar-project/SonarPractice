#ifndef TST_VIEWMODELSTEST_H
#define TST_VIEWMODELSTEST_H

#include <QObject>
#include <QTemporaryDir>
#include <memory>
#include <optional>

#include "ApplicationErrorLog.h"
#include "ILauncher.h"
#include "MediaFile.h"
#include "PracticeAssetController.h"
#include "SqliteArtistRepository.h"
#include "SqliteConnection.h"
#include "SqliteFileRelationRepository.h"
#include "SqliteLinkGroupRepository.h"
#include "SqliteMediaFileRepository.h"
#include "SqlitePracticeAssetRepository.h"
#include "SqlitePracticeJournalRepository.h"
#include "SqlitePracticeNoticeRepository.h"
#include "SqliteReminderConditionRepository.h"
#include "SqliteReminderRepository.h"
#include "SqliteSongRepository.h"
#include "SqliteTuningRepository.h"
#include "TestConstants.h"

class MockLauncher : public ILauncher {
  public:
    bool open(const QString &target, MediaSourceType sourceType) override {
        ++openCallCount;
        lastTarget = target;
        lastSourceType = sourceType;
        return shouldSucceed;
    }

    int openCallCount{0};
    QString lastTarget;
    MediaSourceType lastSourceType{MediaSourceType::Local};
    bool shouldSucceed{true};
};

class TestViewModels : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void testSongModelEmpty();
    void testSongModelCatalogReady();
    void testCatalogSnapshotMatchesSongModel();
    void testSongModelAssetSummary();
    void testSongModelTuningName();
    void testSongModelSearchFilter();
    void testSongModelHidesSecondaryLinkedSongs();
    void testSongModelContainersOnlyShowsHub();
    void testSongModelExpandAllGroupsRevealsMembers();
    void testSongModelHideContainersShowsMembers();
    void testMediaFileListModelFiltersBySong();
    void testMediaFileListModelDisplayNameFromRelativePath();
    void testMediaFileListModelIncludesLinkedMedia();
    void testPracticeSessionStartPracticeLaunches();
    void testPracticeSessionRejectsNonPracticeableMedia();
    void testOpenAssetUsesPathResolver();
    void testPracticeTrackerTimerSignals();
    void testPracticeTrackerStopAndSaveUpdatesJournal();
    void testPracticeTrackerRejectsInvalidBarRange();
    void testPracticeTrackerLoadDefaultsFromLastSession();
    void testPracticeTrackerLoadDefaultsFromReminder();
    void testReminderListModelFiltersByPracticeAsset();
    void testReminderListModelBuildScheduleLabelUsesLocaleDateFormat();
    void testJournalTableModelUsesLocaleDateFormat();
    void testReminderControllerIntervalAndWeekdayRoundTrip();
    void testReminderControllerRequiresPracticeAssetId();
    void testReminderControllerDeleteReminderUpdatesCounts();
    void testPracticeAssetControllerCompositeUpsert();
    void testPracticeAssetControllerFilteredAudioFiles();

    void testLibrarySearchExpressionAndOr();
    void testLibrarySearchExpressionRegex();
    void testLibraryLinkModelBooleanSearch();
    void testLibraryLinkModelContainersOnlyShowsGroupTitle();
    void testLibraryLinkModelFolderPrefixFilter();
    void testCatalogViewCacheFeedsBothModels();
    void testLibraryLinkModelSkipsReloadWhenLoaded();

  private:
    std::optional<qlonglong> createSong(const QString &title, int baseBpm = TestBpm::kDefaultSong);
    std::optional<qlonglong> createMediaFile(qlonglong songId, const QString &path, MediaKind kind,
                                             bool canBePracticed = false,
                                             MediaSourceType sourceType = MediaSourceType::Local,
                                             bool isManaged = false);

    SqliteConnection m_connector{QStringLiteral("ViewModelsTestDb")};
    SqliteArtistRepository m_artistRepo{m_connector};
    SqliteSongRepository m_songRepo{m_connector};
    SqliteTuningRepository m_tuningRepo{m_connector};
    SqliteMediaFileRepository m_mediaFileRepo{m_connector};
    SqliteLinkGroupRepository m_linkGroupRepo{m_connector};
    SqliteFileRelationRepository m_fileRelationRepo{m_connector};
    SqlitePracticeJournalRepository m_journalRepo{m_connector};
    SqlitePracticeAssetRepository m_practiceAssetRepo{m_connector};
    SqlitePracticeNoticeRepository m_noticeRepo{m_connector};
    SqliteReminderRepository m_reminderRepo{m_connector};
    SqliteReminderConditionRepository m_conditionRepo{m_connector};
    std::unique_ptr<PracticeAssetController> m_practiceAssetController;
    QTemporaryDir m_errorLogStorage;
    std::unique_ptr<ApplicationErrorLog> m_errorLog;
    MockLauncher m_launcher;
};

#endif // TST_VIEWMODELSTEST_H
