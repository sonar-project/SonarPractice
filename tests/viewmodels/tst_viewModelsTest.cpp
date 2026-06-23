#include "tst_viewModelsTest.h"

#include "Artist.h"
#include "ApplicationErrorLog.h"
#include "CatalogViewCache.h"
#include "DatabaseSchema.h"
#include "JournalEntry.h"
#include "JournalTableModel.h"
#include "LibraryLinkModel.h"
#include "LibrarySearchExpression.h"
#include "LinkGroup.h"
#include "LinkGroupService.h"
#include "MediaFileListModel.h"
#include "PathResolver.h"
#include "PracticeAsset.h"
#include "PracticeConstants.h"
#include "PracticeSessionController.h"
#include "PracticeTrackerController.h"
#include "Reminder.h"
#include "ReminderCondition.h"
#include "ReminderController.h"
#include "ReminderListModel.h"
#include "SongModel.h"
#include "SqliteUserRepository.h"
#include "TestConstants.h"
#include "Tuning.h"
#include "User.h"

#include <QDateTime>
#include <QLocale>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTest>
#include <QTime>

void TestViewModels::initTestCase() {
    QVERIFY(m_errorLogStorage.isValid());
    m_errorLog = std::make_unique<ApplicationErrorLog>(
        m_errorLogStorage.filePath(QStringLiteral("errors.log")));
}

void TestViewModels::init() {
    QVERIFY(m_connector.open(QStringLiteral(":memory:")));

    DatabaseSchema schema(m_connector);
    QVERIFY(schema.createAllTables());

    User user;
    user.name = QStringLiteral("ViewModelUser");
    user.role = QStringLiteral("student");
    SqliteUserRepository userRepo(m_connector);
    QVERIFY(userRepo.createUser(user).has_value());

    m_launcher = MockLauncher{};
    m_practiceAssetController =
        std::make_unique<PracticeAssetController>(m_practiceAssetRepo, m_mediaFileRepo);
}

void TestViewModels::cleanup() {
    m_connector.close();
    QSqlDatabase::removeDatabase(m_connector.connectionName());
}

void TestViewModels::testSongModelEmpty() {
    SongModel model(m_songRepo, m_mediaFileRepo, m_artistRepo, m_linkGroupRepo, m_fileRelationRepo);
    model.reload();

    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.totalCount(), 0);
    QVERIFY(model.catalogReady());
}

void TestViewModels::testSongModelCatalogReady() {
    SongModel model(m_songRepo, m_mediaFileRepo, m_artistRepo, m_linkGroupRepo, m_fileRelationRepo);
    QVERIFY(!model.catalogReady());
    model.reload();
    QVERIFY(model.catalogReady());
}

void TestViewModels::testCatalogSnapshotMatchesSongModel() {
    const std::optional<qlonglong> songId =
        createSong(QStringLiteral("Snapshot Song"), TestBpm::kPractice);
    QVERIFY(songId.has_value());
    QVERIFY(createMediaFile(*songId, QStringLiteral("/tmp/snap.gp5"), MediaKind::GuitarPro, true)
                .has_value());

    const CatalogSnapshot snapshot = CatalogSnapshot::load(CatalogSnapshot::Dependencies{
        .songRepo = m_songRepo,
        .mediaFileRepo = m_mediaFileRepo,
        .artistRepo = m_artistRepo,
        .linkGroupRepo = m_linkGroupRepo,
        .fileRelationRepo = m_fileRelationRepo,
    });
    QCOMPARE(snapshot.songs().size(), 1);

    SongModel model(m_songRepo, m_mediaFileRepo, m_artistRepo, m_linkGroupRepo, m_fileRelationRepo);
    model.applySnapshot(snapshot);

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.totalCount(), 1);
    QVERIFY(model.catalogReady());
}

void TestViewModels::testSongModelAssetSummary() {
    const std::optional<qlonglong> songId =
        createSong(QStringLiteral("Container Song"), TestBpm::kPractice);
    QVERIFY(songId.has_value());

    QVERIFY(createMediaFile(*songId, QStringLiteral("/tmp/a.gp5"), MediaKind::GuitarPro, true)
                .has_value());
    QVERIFY(
        createMediaFile(*songId, QStringLiteral("/tmp/a.pdf"), MediaKind::Document).has_value());
    QVERIFY(createMediaFile(*songId, QStringLiteral("/tmp/a.mp3"), MediaKind::Audio).has_value());

    SongModel model(m_songRepo, m_mediaFileRepo, m_artistRepo, m_linkGroupRepo, m_fileRelationRepo);
    model.reload();

    QCOMPARE(model.rowCount(), 1);

    const QModelIndex index = model.index(0);
    QCOMPARE(model.data(index, SongModel::TitleRole).toString(), QStringLiteral("Container Song"));
    QCOMPARE(model.data(index, SongModel::BaseBpmRole).toInt(), TestBpm::kPractice);

    const QVariantList summary = model.data(index, SongModel::AssetSummaryRole).toList();
    QCOMPARE(summary.size(), 3);

    int totalCount = 0;
    for (const QVariant &entry : summary) {
        totalCount += entry.toMap().value(QStringLiteral("count")).toInt();
    }
    QCOMPARE(totalCount, 3);
}

void TestViewModels::testSongModelTuningName() {
    Tuning tuning;
    tuning.name = QStringLiteral("Drop D");
    const std::optional<qlonglong> tuningId = m_tuningRepo.createTuning(tuning);
    QVERIFY(tuningId.has_value());

    Song song;
    song.title = QStringLiteral("Tuned Song");
    song.baseBpm = 100;
    song.tuningId = *tuningId;
    const std::optional<qlonglong> songId = m_songRepo.createSong(song);
    QVERIFY(songId.has_value());

    SongModel model(m_songRepo, m_mediaFileRepo, m_artistRepo, m_linkGroupRepo, m_fileRelationRepo);
    model.reload();

    const QModelIndex index = model.index(0);
    QCOMPARE(model.data(index, SongModel::TuningNameRole).toString(), QStringLiteral("Drop D"));
    QCOMPARE(model.data(index, SongModel::TuningIdRole).toLongLong(), *tuningId);
}

void TestViewModels::testSongModelSearchFilter() {
    Artist artist;
    artist.name = QStringLiteral("Metallica");
    const std::optional<qlonglong> artistId = m_artistRepo.createArtist(artist);
    QVERIFY(artistId.has_value());

    Tuning tuning;
    tuning.name = QStringLiteral("Standard E");
    const std::optional<qlonglong> tuningId = m_tuningRepo.createTuning(tuning);
    QVERIFY(tuningId.has_value());

    Song songA;
    songA.title = QStringLiteral("Enter Sandman");
    songA.artistId = *artistId;
    songA.tuningId = *tuningId;
    QVERIFY(m_songRepo.createSong(songA).has_value());

    Song songB;
    songB.title = QStringLiteral("Practice Etude");
    songB.tuningId = *tuningId;
    QVERIFY(m_songRepo.createSong(songB).has_value());

    SongModel model(m_songRepo, m_mediaFileRepo, m_artistRepo, m_linkGroupRepo, m_fileRelationRepo);
    model.reload();
    QCOMPARE(model.totalCount(), 2);

    model.setSearchText(QStringLiteral("metallica"));
    QTest::qWait(250);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), SongModel::TitleRole).toString(),
             QStringLiteral("Enter Sandman"));

    model.setSearchText(QStringLiteral("standard"));
    QTest::qWait(250);
    QCOMPARE(model.rowCount(), 2);

    model.setSearchText(QStringLiteral("etude"));
    QTest::qWait(250);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), SongModel::TitleRole).toString(),
             QStringLiteral("Practice Etude"));

    model.setSearchText(QString());
    QTest::qWait(250);
    QCOMPARE(model.rowCount(), 2);
}

void TestViewModels::testMediaFileListModelFiltersBySong() {
    const std::optional<qlonglong> songA = createSong(QStringLiteral("Song A"));
    const std::optional<qlonglong> songB = createSong(QStringLiteral("Song B"));
    QVERIFY(songA.has_value());
    QVERIFY(songB.has_value());

    QVERIFY(createMediaFile(*songA, QStringLiteral("/tmp/a.gp5"), MediaKind::GuitarPro, true)
                .has_value());
    QVERIFY(createMediaFile(*songB, QStringLiteral("/tmp/b.pdf"), MediaKind::Document).has_value());

    MediaFileListModel model(m_mediaFileRepo, m_linkGroupRepo, m_fileRelationRepo);
    model.setSongId(*songA);
    model.reload();

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), MediaFileListModel::MediaKindRole).toString(),
             QStringLiteral("guitarpro"));
}

void TestViewModels::testMediaFileListModelDisplayNameFromRelativePath() {
    MediaFile file;
    file.filePath = QStringLiteral("/abs/path/to/intro.mp3");
    file.sourceRelativePath = QStringLiteral("Course/Advanced/intro.mp3");
    file.sourceType = MediaSourceType::Local;

    QCOMPARE(MediaFileListModel::displayNameForFile(file), QStringLiteral("intro.mp3"));

    MediaFile urlFile;
    urlFile.filePath = QStringLiteral("https://example.com/tracks/demo.mp3");
    urlFile.sourceType = MediaSourceType::Url;
    QCOMPARE(MediaFileListModel::displayNameForFile(urlFile),
             QStringLiteral("example.com/tracks/demo.mp3"));
}

void TestViewModels::testSongModelHidesSecondaryLinkedSongs() {
    const std::optional<qlonglong> primarySongId = createSong(QStringLiteral("Primary Song"));
    const std::optional<qlonglong> secondarySongId = createSong(QStringLiteral("Secondary Song"));
    QVERIFY(primarySongId.has_value());
    QVERIFY(secondarySongId.has_value());

    const std::optional<qlonglong> primaryMediaId = createMediaFile(
        *primarySongId, QStringLiteral("/tmp/primary.gp5"), MediaKind::GuitarPro, true);
    const std::optional<qlonglong> secondaryMediaId =
        createMediaFile(*secondarySongId, QStringLiteral("/tmp/secondary.mp4"), MediaKind::Video);
    QVERIFY(primaryMediaId.has_value());
    QVERIFY(secondaryMediaId.has_value());

    LinkGroupService linkService(LinkGroupService::Dependencies{
        m_linkGroupRepo,
        m_fileRelationRepo,
        m_mediaFileRepo,
        m_songRepo,
    });
    QVERIFY(linkService
                .createGroup(QStringLiteral("Linked Group"), *primarySongId, *primaryMediaId,
                             {*secondaryMediaId})
                .has_value());

    SongModel model(m_songRepo, m_mediaFileRepo, m_artistRepo, m_linkGroupRepo, m_fileRelationRepo);
    model.reload();

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), SongModel::DisplayTitleRole).toString(),
             QStringLiteral("Linked Group"));
    QCOMPARE(model.data(model.index(0), SongModel::LinkedMediaCountRole).toInt(), 2);
}

void TestViewModels::testSongModelContainersOnlyShowsHub() {
    const std::optional<qlonglong> primarySongId = createSong(QStringLiteral("Hub Only Primary"));
    const std::optional<qlonglong> secondarySongId = createSong(QStringLiteral("Hub Only Secondary"));
    QVERIFY(primarySongId.has_value());
    QVERIFY(secondarySongId.has_value());

    const std::optional<qlonglong> primaryMediaId = createMediaFile(
        *primarySongId, QStringLiteral("/tmp/hub_only.gp5"), MediaKind::GuitarPro, true);
    const std::optional<qlonglong> secondaryMediaId =
        createMediaFile(*secondarySongId, QStringLiteral("/tmp/hub_only.mp4"), MediaKind::Video);
    QVERIFY(primaryMediaId.has_value());
    QVERIFY(secondaryMediaId.has_value());

    LinkGroupService linkService(LinkGroupService::Dependencies{
        m_linkGroupRepo,
        m_fileRelationRepo,
        m_mediaFileRepo,
        m_songRepo,
    });
    QVERIFY(linkService
                .createGroup(QStringLiteral("Hub Only Group"), *primarySongId, *primaryMediaId,
                             {*secondaryMediaId})
                .has_value());

    SongModel model(m_songRepo, m_mediaFileRepo, m_artistRepo, m_linkGroupRepo, m_fileRelationRepo);
    model.reload();
    model.setContainersOnly(true);

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), SongModel::DisplayTitleRole).toString(),
             QStringLiteral("Hub Only Group"));
    QVERIFY(model.data(model.index(0), SongModel::IsLinkedGroupRole).toBool());
}

void TestViewModels::testSongModelExpandAllGroupsRevealsMembers() {
    const std::optional<qlonglong> primarySongId = createSong(QStringLiteral("Expand Primary"));
    const std::optional<qlonglong> secondarySongId = createSong(QStringLiteral("Expand Secondary"));
    QVERIFY(primarySongId.has_value());
    QVERIFY(secondarySongId.has_value());

    const std::optional<qlonglong> primaryMediaId = createMediaFile(
        *primarySongId, QStringLiteral("/tmp/expand.gp5"), MediaKind::GuitarPro, true);
    const std::optional<qlonglong> secondaryMediaId =
        createMediaFile(*secondarySongId, QStringLiteral("/tmp/expand.mp4"), MediaKind::Video);
    QVERIFY(primaryMediaId.has_value());
    QVERIFY(secondaryMediaId.has_value());

    LinkGroupService linkService(LinkGroupService::Dependencies{
        m_linkGroupRepo,
        m_fileRelationRepo,
        m_mediaFileRepo,
        m_songRepo,
    });
    const std::optional<qlonglong> groupId = linkService.createGroup(
        QStringLiteral("Expand Group"), *primarySongId, *primaryMediaId, {*secondaryMediaId});
    QVERIFY(groupId.has_value());

    SongModel model(m_songRepo, m_mediaFileRepo, m_artistRepo, m_linkGroupRepo, m_fileRelationRepo);
    model.reload();
    QCOMPARE(model.rowCount(), 1);

    model.setExpandAllGroups(true);
    QCOMPARE(model.rowCount(), 2);

    model.setExpandAllGroups(false);
    model.setGroupExpanded(*groupId, true);
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.isGroupExpanded(*groupId), true);
}

void TestViewModels::testSongModelHideContainersShowsMembers() {
    const std::optional<qlonglong> primarySongId = createSong(QStringLiteral("Hide Hub Primary"));
    const std::optional<qlonglong> secondarySongId =
        createSong(QStringLiteral("Hide Hub Secondary"));
    QVERIFY(primarySongId.has_value());
    QVERIFY(secondarySongId.has_value());

    const std::optional<qlonglong> primaryMediaId = createMediaFile(
        *primarySongId, QStringLiteral("/tmp/hide_hub.gp5"), MediaKind::GuitarPro, true);
    const std::optional<qlonglong> secondaryMediaId =
        createMediaFile(*secondarySongId, QStringLiteral("/tmp/hide_hub.mp4"), MediaKind::Video);
    QVERIFY(primaryMediaId.has_value());
    QVERIFY(secondaryMediaId.has_value());

    LinkGroupService linkService(LinkGroupService::Dependencies{
        m_linkGroupRepo,
        m_fileRelationRepo,
        m_mediaFileRepo,
        m_songRepo,
    });
    QVERIFY(linkService
                .createGroup(QStringLiteral("Hide Hub Group"), *primarySongId, *primaryMediaId,
                             {*secondaryMediaId})
                .has_value());

    SongModel model(m_songRepo, m_mediaFileRepo, m_artistRepo, m_linkGroupRepo, m_fileRelationRepo);
    model.reload();
    model.setHideContainers(true);

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), SongModel::TitleRole).toString(),
             QStringLiteral("Hide Hub Secondary"));
    QVERIFY(model.data(model.index(0), SongModel::IsContainerMemberRole).toBool());
}

void TestViewModels::testMediaFileListModelIncludesLinkedMedia() {
    const std::optional<qlonglong> primarySongId = createSong(QStringLiteral("Hub Song"));
    const std::optional<qlonglong> secondarySongId = createSong(QStringLiteral("Video Song"));
    QVERIFY(primarySongId.has_value());
    QVERIFY(secondarySongId.has_value());

    const std::optional<qlonglong> primaryMediaId =
        createMediaFile(*primarySongId, QStringLiteral("/tmp/hub.gp5"), MediaKind::GuitarPro, true);
    const std::optional<qlonglong> secondaryMediaId =
        createMediaFile(*secondarySongId, QStringLiteral("/tmp/video.mp4"), MediaKind::Video);
    QVERIFY(primaryMediaId.has_value());
    QVERIFY(secondaryMediaId.has_value());

    LinkGroupService linkService(LinkGroupService::Dependencies{
        m_linkGroupRepo,
        m_fileRelationRepo,
        m_mediaFileRepo,
        m_songRepo,
    });
    QVERIFY(linkService
                .createGroup(QStringLiteral("Hub Group"), *primarySongId, *primaryMediaId,
                             {*secondaryMediaId})
                .has_value());

    MediaFileListModel model(m_mediaFileRepo, m_linkGroupRepo, m_fileRelationRepo);
    model.setSongId(*primarySongId);
    model.reload();

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.filesForKind(QStringLiteral("video")).size(), 1);
    QCOMPARE(model.filesForKind(QStringLiteral("guitarpro")).size(), 1);
}

void TestViewModels::testPracticeSessionStartPracticeLaunches() {
    const std::optional<qlonglong> songId = createSong(QStringLiteral("Practice Song"));
    QVERIFY(songId.has_value());

    const std::optional<qlonglong> mediaId =
        createMediaFile(*songId, QStringLiteral("/tmp/practice.gp5"), MediaKind::GuitarPro, true);
    QVERIFY(mediaId.has_value());

    PathResolver resolver(QStringLiteral("/managed/root"));
    PracticeSessionController controller({
        .mediaRepo = m_mediaFileRepo,
        .pathResolver = resolver,
        .launcher = m_launcher,
        .errorLog = *m_errorLog,
    });

    QVERIFY(controller.startPractice(*mediaId));
    QCOMPARE(m_launcher.openCallCount, 1);
    QCOMPARE(m_launcher.lastTarget, QStringLiteral("/tmp/practice.gp5"));
}

void TestViewModels::testPracticeSessionRejectsNonPracticeableMedia() {
    const std::optional<qlonglong> songId = createSong(QStringLiteral("PDF Song"));
    QVERIFY(songId.has_value());

    const std::optional<qlonglong> mediaId =
        createMediaFile(*songId, QStringLiteral("/tmp/sheet.pdf"), MediaKind::Document);
    QVERIFY(mediaId.has_value());

    PathResolver resolver(QStringLiteral("/managed/root"));
    PracticeSessionController controller({
        .mediaRepo = m_mediaFileRepo,
        .pathResolver = resolver,
        .launcher = m_launcher,
        .errorLog = *m_errorLog,
    });

    QSignalSpy failedSpy(&controller, &PracticeSessionController::launchFailed);
    QSignalSpy noticeSpy(m_errorLog.get(), &ApplicationErrorLog::userNoticeRequested);

    QVERIFY(!controller.startPractice(*mediaId));
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(noticeSpy.count(), 1);
    QCOMPARE(m_launcher.openCallCount, 0);
}

void TestViewModels::testOpenAssetUsesPathResolver() {
    const std::optional<qlonglong> songId = createSong(QStringLiteral("Managed Song"));
    QVERIFY(songId.has_value());

    const std::optional<qlonglong> mediaId =
        createMediaFile(*songId, QStringLiteral("managed.gp5"), MediaKind::GuitarPro, false,
                        MediaSourceType::Local, true);
    QVERIFY(mediaId.has_value());

    PathResolver resolver(QStringLiteral("/managed/root"));
    PracticeSessionController controller({
        .mediaRepo = m_mediaFileRepo,
        .pathResolver = resolver,
        .launcher = m_launcher,
        .errorLog = *m_errorLog,
    });

    QVERIFY(controller.openAsset(*mediaId));
    QCOMPARE(m_launcher.lastTarget, QStringLiteral("/managed/root/managed.gp5"));
}

void TestViewModels::testPracticeTrackerTimerSignals() {
    const std::optional<qlonglong> songId = createSong(QStringLiteral("Tracker Song"));
    QVERIFY(songId.has_value());

    PracticeTrackerController controller(m_journalRepo, m_noticeRepo, m_reminderRepo,
                                         m_conditionRepo, *m_errorLog);
    controller.setSongId(*songId);

    QSignalSpy stateSpy(&controller, &PracticeTrackerController::timerStateChanged);

    QVERIFY(controller.startTimer());
    QVERIFY(controller.timerRunning());
    QCOMPARE(stateSpy.count(), 1);

    controller.cancelTimer();
    QVERIFY(!controller.timerRunning());
}

void TestViewModels::testPracticeTrackerStopAndSaveUpdatesJournal() {
    const std::optional<qlonglong> songId = createSong(QStringLiteral("Journal Song"));
    QVERIFY(songId.has_value());

    const std::optional<qlonglong> mediaId =
        createMediaFile(*songId, QStringLiteral("/tmp/journal.gp5"), MediaKind::GuitarPro, true);
    QVERIFY(mediaId.has_value());

    PracticeAsset practiceAsset;
    practiceAsset.songId = *songId;
    practiceAsset.guitarProId = *mediaId;
    const std::optional<qlonglong> assetId = m_practiceAssetRepo.upsert(practiceAsset);
    QVERIFY(assetId.has_value());

    PracticeTrackerController controller(m_journalRepo, m_noticeRepo, m_reminderRepo,
                                         m_conditionRepo, *m_errorLog);
    controller.setSongId(*songId);
    controller.setAssetId(*assetId);
    controller.setStartBar(PracticeConstants::kDefaultStartBar);
    controller.setEndBar(TestBars::kEnd);
    controller.setTargetBpm(TestBpm::kSession);
    controller.setTotalReps(TestReps::kTotal);
    controller.setSuccessfulReps(TestReps::kPartialSuccess + 1);

    QSignalSpy savedSpy(&controller, &PracticeTrackerController::journalSaved);

    QVERIFY(controller.startTimer());
    QVERIFY(controller.stopAndSave());
    QCOMPARE(savedSpy.count(), 1);
    QCOMPARE(controller.statusMessage(), QStringLiteral("Practice session saved."));
    QVERIFY(!controller.statusIsError());
    QCOMPARE(controller.journalModel()->rowCount(), 1);
    QCOMPARE(controller.journalModel()
                 ->data(controller.journalModel()->index(
                     0, static_cast<int>(JournalTableModel::DisplayColumn::Bpm)))
                 .toInt(),
             TestBpm::kSession);
}

void TestViewModels::testPracticeTrackerRejectsInvalidBarRange() {
    const std::optional<qlonglong> songId = createSong(QStringLiteral("Invalid Bars"));
    QVERIFY(songId.has_value());

    PracticeTrackerController controller(m_journalRepo, m_noticeRepo, m_reminderRepo,
                                         m_conditionRepo, *m_errorLog);
    controller.setSongId(*songId);
    controller.setStartBar(TestDates::kDay10);
    controller.setEndBar(TestCounts::kTwo);

    QSignalSpy failedSpy(&controller, &PracticeTrackerController::saveFailed);

    QVERIFY(controller.startTimer());
    QVERIFY(!controller.stopAndSave());
    QCOMPARE(failedSpy.count(), 1);
    QVERIFY(controller.statusIsError());
    QVERIFY(!controller.statusMessage().isEmpty());
    QCOMPARE(controller.journalModel()->rowCount(), 0);
}

void TestViewModels::testPracticeTrackerLoadDefaultsFromLastSession() {
    const std::optional<qlonglong> songId = createSong(QStringLiteral("Defaults Session Song"));
    QVERIFY(songId.has_value());

    const std::optional<qlonglong> mediaId = createMediaFile(
        *songId, QStringLiteral("/tmp/defaults_session.gp5"), MediaKind::GuitarPro, true);
    QVERIFY(mediaId.has_value());

    PracticeAsset practiceAsset;
    practiceAsset.songId = *songId;
    practiceAsset.guitarProId = *mediaId;
    const std::optional<qlonglong> assetId = m_practiceAssetRepo.upsert(practiceAsset);
    QVERIFY(assetId.has_value());

    JournalEntry olderEntry;
    olderEntry.assetId = *assetId;
    olderEntry.practiceDate =
        QDateTime(QDate(TestDates::kYear, TestDates::kFebruary, TestDates::kDay1),
                  QTime(TestDates::kHour10, TestDates::kMinute0));
    olderEntry.startBar = PracticeConstants::kDefaultStartBar;
    olderEntry.endBar = PracticeConstants::kDefaultEndBar;
    olderEntry.practicedBpm = TestBpm::kSlow;
    QVERIFY(m_journalRepo.createEntry(olderEntry).has_value());

    JournalEntry latestEntry;
    latestEntry.assetId = *assetId;
    latestEntry.practiceDate =
        QDateTime(QDate(TestDates::kYear, TestDates::kMarch, TestDates::kDay1),
                  QTime(TestDates::kHour10, TestDates::kMinute0));
    latestEntry.startBar = TestBars::kStart;
    latestEntry.endBar = TestBars::kEndExtended;
    latestEntry.practicedBpm = TestBpm::kUpdated;
    QVERIFY(m_journalRepo.createEntry(latestEntry).has_value());

    PracticeTrackerController controller(m_journalRepo, m_noticeRepo, m_reminderRepo,
                                         m_conditionRepo, *m_errorLog);
    controller.setSongId(*songId);
    controller.setAssetId(*assetId);
    controller.loadTrainingDefaults(TestBpm::kDefaultSong);

    QCOMPARE(controller.startBar(), TestBars::kStart);
    QCOMPARE(controller.endBar(), TestBars::kEndExtended);
    QCOMPARE(controller.targetBpm(), TestBpm::kUpdated);
}

void TestViewModels::testPracticeTrackerLoadDefaultsFromReminder() {
    const std::optional<qlonglong> songId = createSong(QStringLiteral("Defaults Reminder Song"));
    QVERIFY(songId.has_value());

    Reminder reminder;
    reminder.songId = *songId;
    reminder.title = QStringLiteral("Daily");
    reminder.isActive = true;
    reminder.isDaily = true;
    const std::optional<qlonglong> reminderId = m_reminderRepo.createReminder(reminder);
    QVERIFY(reminderId.has_value());

    ReminderCondition condition;
    condition.reminderId = *reminderId;
    condition.startBar = TestCounts::kThree;
    condition.endBar = TestBars::kEndSection;
    condition.minBpm = TestReminder::kConditionMinBpm;
    QVERIFY(m_conditionRepo.createCondition(condition).has_value());

    PracticeTrackerController controller(m_journalRepo, m_noticeRepo, m_reminderRepo,
                                         m_conditionRepo, *m_errorLog);
    controller.setSongId(*songId);
    controller.loadTrainingDefaults(TestBpm::kDefaultSong);

    QCOMPARE(controller.startBar(), TestCounts::kThree);
    QCOMPARE(controller.endBar(), TestBars::kEndSection);
    QCOMPARE(controller.targetBpm(), TestReminder::kConditionMinBpm);
}

void TestViewModels::testJournalTableModelUsesLocaleDateFormat() {
    const std::optional<qlonglong> songId = createSong(QStringLiteral("Journal Locale Song"));
    QVERIFY(songId.has_value());

    const std::optional<qlonglong> mediaId =
        createMediaFile(*songId, QStringLiteral("/tmp/journal_locale.gp5"), MediaKind::GuitarPro,
                        true);
    QVERIFY(mediaId.has_value());

    PracticeAsset practiceAsset;
    practiceAsset.songId = *songId;
    practiceAsset.guitarProId = *mediaId;
    const std::optional<qlonglong> assetId = m_practiceAssetRepo.upsert(practiceAsset);
    QVERIFY(assetId.has_value());

    const QDate practiceDay(2024, 3, 15);
    JournalEntry entry;
    entry.assetId = *assetId;
    entry.practiceDate = QDateTime(practiceDay, QTime(10, 0));
    entry.startBar = TestBars::kStart;
    entry.endBar = TestBars::kEnd;
    entry.practicedBpm = TestBpm::kPractice;
    entry.successfulStreaks = TestReps::kPartialSuccess;
    entry.durationSeconds = TestDurations::kPracticeSeconds;
    QVERIFY(m_journalRepo.createEntry(entry).has_value());

    JournalTableModel model(m_journalRepo);
    model.setAssetId(*assetId);
    model.setSelectedDate(practiceDay);
    QCOMPARE(model.rowCount(), 1);

    const QModelIndex dateIndex =
        model.index(0, static_cast<int>(JournalTableModel::DisplayColumn::Date));
    const QLocale savedLocale = QLocale();

    QLocale::setDefault(QLocale(QLocale::German, QLocale::Germany));
    const QString germanLabel = model.data(dateIndex).toString();
    QCOMPARE(germanLabel, QLocale(QLocale::German, QLocale::Germany)
                              .toString(practiceDay, QLocale::ShortFormat));

    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    const QString englishLabel = model.data(dateIndex).toString();
    QCOMPARE(englishLabel, QLocale(QLocale::English, QLocale::UnitedStates)
                                .toString(practiceDay, QLocale::ShortFormat));

    QVERIFY(germanLabel != englishLabel);

    QLocale::setDefault(savedLocale);
}

void TestViewModels::testReminderControllerIntervalAndWeekdayRoundTrip() {
    const std::optional<qlonglong> songId = createSong(QStringLiteral("Interval Reminder Song"));
    QVERIFY(songId.has_value());

    const std::optional<qlonglong> mediaId =
        createMediaFile(*songId, QStringLiteral("/tmp/interval.gpx"), MediaKind::GuitarPro, true);
    QVERIFY(mediaId.has_value());

    PracticeAsset practiceAsset;
    practiceAsset.songId = *songId;
    practiceAsset.guitarProId = *mediaId;
    const std::optional<qlonglong> assetId = m_practiceAssetRepo.upsert(practiceAsset);
    QVERIFY(assetId.has_value());

    ReminderController controller(m_reminderRepo, m_conditionRepo, m_journalRepo, m_completionRepo,
                                  m_songRepo, *m_practiceAssetController);
    controller.setSongId(*songId);
    controller.setPracticeAssetId(*assetId);

    const QDate anchorDate(2024, 6, 1);
    QVERIFY(controller.createReminder(QStringLiteral("Every two days"), anchorDate,
                                      QStringLiteral("interval"), true, TestBars::kStart,
                                      TestBars::kEnd, TestBpm::kDefaultSong, 0, 2));

    const qlonglong reminderId =
        controller.songReminders()
            ->data(controller.songReminders()->index(0, 0), ReminderListModel::ReminderIdRole)
            .toLongLong();

    const QVariantMap payload = controller.reminderEditPayload(reminderId);
    QCOMPARE(payload.value(QStringLiteral("intervalDays")).toInt(), 2);
    QCOMPARE(payload.value(QStringLiteral("weekday")).toInt(), -1);
    QCOMPARE(payload.value(QStringLiteral("reminderDate")).toDate(), anchorDate);
    QCOMPARE(payload.value(QStringLiteral("scheduleType")).toString(), QStringLiteral("interval"));

    const std::optional<Reminder> stored = m_reminderRepo.getReminder(reminderId);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->intervalDays, 2);
    QVERIFY(stored->isDueOn(anchorDate));
    QVERIFY(stored->isDueOn(anchorDate.addDays(2)));
    QVERIFY(!stored->isDueOn(anchorDate.addDays(1)));

    QVERIFY(controller.updateReminder(reminderId, QStringLiteral("Weekly drill"), anchorDate,
                                      QStringLiteral("weekly"), true, 0, 5));

    const std::optional<Reminder> weekly = m_reminderRepo.getReminder(reminderId);
    QVERIFY(weekly.has_value());
    QVERIFY(weekly->isWeekly);
    QCOMPARE(weekly->weekday, 5);
    QCOMPARE(weekly->intervalDays, 0);
    QVERIFY(weekly->isDueOn(QDate(2024, 6, 7)));
    QVERIFY(!weekly->isDueOn(QDate(2024, 6, 6)));
}

void TestViewModels::testReminderListModelBuildScheduleLabelUsesLocaleDateFormat() {
    Reminder once;
    once.reminderDate = QDate(2024, 3, 15);

    const QLocale savedLocale = QLocale();
    const auto expectedFor = [](const Reminder &reminder, const QLocale &locale) {
        return locale.toString(reminder.reminderDate, QLocale::ShortFormat);
    };

    QLocale::setDefault(QLocale(QLocale::German, QLocale::Germany));
    const QString germanLabel = ReminderListModel::buildScheduleLabel(once);
    QCOMPARE(germanLabel, expectedFor(once, QLocale(QLocale::German, QLocale::Germany)));

    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    const QString englishLabel = ReminderListModel::buildScheduleLabel(once);
    QCOMPARE(englishLabel, expectedFor(once, QLocale(QLocale::English, QLocale::UnitedStates)));

    QVERIFY(germanLabel != englishLabel);

    QLocale::setDefault(savedLocale);
}

void TestViewModels::testReminderListModelFiltersByPracticeAsset() {
    const std::optional<qlonglong> songId = createSong(QStringLiteral("Asset Reminder Song"));
    QVERIFY(songId.has_value());

    const std::optional<qlonglong> gpxId =
        createMediaFile(*songId, QStringLiteral("/tmp/Song1.gpx"), MediaKind::GuitarPro, true);
    const std::optional<qlonglong> gpId =
        createMediaFile(*songId, QStringLiteral("/tmp/Song2.gp"), MediaKind::GuitarPro, true);
    QVERIFY(gpxId.has_value());
    QVERIFY(gpId.has_value());

    PracticeAsset gpxAsset;
    gpxAsset.songId = *songId;
    gpxAsset.guitarProId = *gpxId;
    const auto assetIdGpx = m_practiceAssetRepo.upsert(gpxAsset);
    QVERIFY(assetIdGpx.has_value());

    PracticeAsset gpAsset;
    gpAsset.songId = *songId;
    gpAsset.guitarProId = *gpId;
    const auto assetIdGp = m_practiceAssetRepo.upsert(gpAsset);
    QVERIFY(assetIdGp.has_value());

    Reminder gpxReminder;
    gpxReminder.songId = *songId;
    gpxReminder.practiceAssetId = *assetIdGpx;
    gpxReminder.title = QStringLiteral("GPX warmup");
    gpxReminder.isDaily = true;
    QVERIFY(m_reminderRepo.createReminder(gpxReminder).has_value());

    Reminder gpReminder;
    gpReminder.songId = *songId;
    gpReminder.practiceAssetId = *assetIdGp;
    gpReminder.title = QStringLiteral("GP drill");
    gpReminder.isDaily = true;
    QVERIFY(m_reminderRepo.createReminder(gpReminder).has_value());

    ReminderListModel model(m_reminderRepo, m_conditionRepo, &m_songRepo);
    model.setSongId(*songId);
    model.setPracticeAssetId(*assetIdGpx);
    model.reload();
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 0), ReminderListModel::TitleRole).toString(),
             QStringLiteral("GPX warmup"));

    model.setPracticeAssetId(*assetIdGp);
    model.reload();
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 0), ReminderListModel::TitleRole).toString(),
             QStringLiteral("GP drill"));
}

void TestViewModels::testReminderControllerRequiresPracticeAssetId() {
    const std::optional<qlonglong> songId = createSong(QStringLiteral("Reminder CRUD Song"));
    QVERIFY(songId.has_value());

    const std::optional<qlonglong> mediaId =
        createMediaFile(*songId, QStringLiteral("/tmp/reminder.gpx"), MediaKind::GuitarPro, true);
    QVERIFY(mediaId.has_value());

    PracticeAsset practiceAsset;
    practiceAsset.songId = *songId;
    practiceAsset.guitarProId = *mediaId;
    const std::optional<qlonglong> assetId = m_practiceAssetRepo.upsert(practiceAsset);
    QVERIFY(assetId.has_value());

    ReminderController controller(m_reminderRepo, m_conditionRepo, m_journalRepo, m_completionRepo,
                                  m_songRepo, *m_practiceAssetController);

    controller.setSongId(*songId);

    // createReminder must fail without practiceAssetId
    QVERIFY(!controller.createReminder(QStringLiteral("Weekly Warmup"), QDate::currentDate(),
                                       QStringLiteral("daily"), true, TestBars::kStart,
                                       TestBars::kEnd, TestBpm::kDefaultSong, 0));

    controller.setPracticeAssetId(*assetId);
    QVERIFY(controller.createReminder(QStringLiteral("Weekly Warmup"), QDate::currentDate(),
                                      QStringLiteral("daily"), true, TestBars::kStart,
                                      TestBars::kEnd, TestBpm::kDefaultSong, 0));
    QCOMPARE(controller.songReminders()->rowCount(), 1);
}

void TestViewModels::testReminderControllerDeleteReminderUpdatesCounts() {
    const std::optional<qlonglong> songId = createSong(QStringLiteral("Day Count Song"));
    QVERIFY(songId.has_value());

    const std::optional<qlonglong> mediaId =
        createMediaFile(*songId, QStringLiteral("/tmp/day_count.gpx"), MediaKind::GuitarPro, true);
    QVERIFY(mediaId.has_value());

    PracticeAsset practiceAsset;
    practiceAsset.songId = *songId;
    practiceAsset.guitarProId = *mediaId;
    const std::optional<qlonglong> assetId = m_practiceAssetRepo.upsert(practiceAsset);
    QVERIFY(assetId.has_value());

    ReminderController controller(m_reminderRepo, m_conditionRepo, m_journalRepo, m_completionRepo,
                                  m_songRepo, *m_practiceAssetController);
    controller.setSongId(*songId);
    controller.setPracticeAssetId(*assetId);
    controller.setShowAllReminders(true);

    QVERIFY(controller.createReminder(QStringLiteral("Daily Drill"), QDate::currentDate(),
                                      QStringLiteral("daily"), true, TestBars::kStart,
                                      TestBars::kEnd, TestBpm::kDefaultSong, 0));
    const qlonglong dailyReminderId =
        controller.songReminders()
            ->data(controller.songReminders()->index(0, 0), ReminderListModel::ReminderIdRole)
            .toLongLong();
    QVERIFY(controller.createReminder(QStringLiteral("Weekly Drill"), QDate::currentDate(),
                                      QStringLiteral("weekly"), true, TestBars::kStart,
                                      TestBars::kEnd, TestBpm::kDefaultSong, 0));

    QCOMPARE(controller.dayReminderCount(), 2);
    QCOMPARE(controller.dailyReminderCount(), 1);
    QCOMPARE(controller.periodicReminderCount(), 1);

    QVERIFY(controller.deleteReminder(dailyReminderId));

    QCOMPARE(controller.dayReminderCount(), 1);
    QCOMPARE(controller.dailyReminderCount(), 0);
    QCOMPARE(controller.periodicReminderCount(), 1);
    QCOMPARE(controller.songReminders()->rowCount(), 1);
}

void TestViewModels::testPracticeAssetControllerCompositeUpsert() {
    const std::optional<qlonglong> songId = createSong(QStringLiteral("Composite Song"));
    QVERIFY(songId.has_value());

    const std::optional<qlonglong> gpId =
        createMediaFile(*songId, QStringLiteral("/tmp/composite.gp5"), MediaKind::GuitarPro, true);
    const std::optional<qlonglong> audioId =
        createMediaFile(*songId, QStringLiteral("/tmp/composite.mp3"), MediaKind::Audio);
    const std::optional<qlonglong> videoId =
        createMediaFile(*songId, QStringLiteral("/tmp/composite.mp4"), MediaKind::Video);
    QVERIFY(gpId.has_value());
    QVERIFY(audioId.has_value());
    QVERIFY(videoId.has_value());

    const qlonglong assetId = m_practiceAssetController->upsertCompositeAsset(
        *songId, *gpId, *audioId, *videoId, 0, 0);
    QVERIFY(assetId > 0);

    const QVariantMap assetMap = m_practiceAssetController->assetById(assetId);
    QCOMPARE(assetMap.value(QStringLiteral("guitarProId")).toLongLong(), *gpId);
    QCOMPARE(assetMap.value(QStringLiteral("audioId")).toLongLong(), *audioId);
    QCOMPARE(assetMap.value(QStringLiteral("videoId")).toLongLong(), *videoId);

    QCOMPARE(m_practiceAssetController->mediaFileIdForAsset(assetId), *gpId);

    ReminderController reminderController(m_reminderRepo, m_conditionRepo, m_journalRepo,
                                          m_completionRepo, m_songRepo,
                                          *m_practiceAssetController);
    const QVariantMap payload = reminderController.practiceAssetPayload(assetId);
    QCOMPARE(payload.value(QStringLiteral("assetId")).toLongLong(), assetId);
    QCOMPARE(payload.value(QStringLiteral("songId")).toLongLong(), *songId);
    QCOMPARE(payload.value(QStringLiteral("guitarProId")).toLongLong(), *gpId);
    QCOMPARE(payload.value(QStringLiteral("audioId")).toLongLong(), *audioId);
}

void TestViewModels::testPracticeAssetControllerFilteredAudioFiles() {
    const std::optional<qlonglong> songId = createSong(QStringLiteral("Audio Filter Song"));
    QVERIFY(songId.has_value());

    MediaFile backingTrack;
    backingTrack.songId = *songId;
    backingTrack.filePath = QStringLiteral("/tmp/backing_track.mp3");
    backingTrack.fileType = QStringLiteral("mp3");
    backingTrack.mediaKind = MediaKind::Audio;
    backingTrack.sourceType = MediaSourceType::Local;
    backingTrack.sourceRelativePath = QStringLiteral("Audio/backing_track.mp3");
    QVERIFY(m_mediaFileRepo.createMediaFile(backingTrack).has_value());

    MediaFile clickTrack;
    clickTrack.songId = *songId;
    clickTrack.filePath = QStringLiteral("/tmp/metronome.wav");
    clickTrack.fileType = QStringLiteral("wav");
    clickTrack.mediaKind = MediaKind::Audio;
    clickTrack.sourceType = MediaSourceType::Local;
    clickTrack.sourceRelativePath = QStringLiteral("Audio/metronome.wav");
    QVERIFY(m_mediaFileRepo.createMediaFile(clickTrack).has_value());

    createMediaFile(*songId, QStringLiteral("/tmp/not_audio.pdf"), MediaKind::Document);

    const QVariantList backingMatches =
        m_practiceAssetController->filteredAudioFiles(QStringLiteral("backing"));
    QCOMPARE(backingMatches.size(), 1);
    QCOMPARE(backingMatches.first().toMap().value(QStringLiteral("displayName")).toString(),
             QStringLiteral("backing_track.mp3"));

    const QVariantList allAudio = m_practiceAssetController->filteredAudioFiles(QString());
    QCOMPARE(allAudio.size(), 2);
}

std::optional<qlonglong> TestViewModels::createSong(const QString &title, int baseBpm) {
    Song song;
    song.title = title;
    song.baseBpm = baseBpm;
    return m_songRepo.createSong(song);
}

std::optional<qlonglong> TestViewModels::createMediaFile(qlonglong songId, const QString &path,
                                                         MediaKind kind, bool canBePracticed,
                                                         MediaSourceType sourceType,
                                                         bool isManaged) {
    MediaFile file;
    file.songId = songId;
    file.filePath = path;
    file.fileType = path.section(QLatin1Char('.'), -1);
    file.mediaKind = kind;
    file.sourceType = sourceType;
    file.isManaged = isManaged;
    file.canBePracticed = canBePracticed;
    if (isManaged) {
        file.fileHash = QStringLiteral("hash-%1").arg(path);
    }
    return m_mediaFileRepo.createMediaFile(file);
}

void TestViewModels::testLibrarySearchExpressionAndOr() {
    const QString haystack = QStringLiteral("picking licks lesson/picking_01.mp4 guitarpro");

    QVERIFY(LibrarySearchExpression::matches(QStringLiteral("Picking Licks && mp4"), haystack));
    QVERIFY(
        LibrarySearchExpression::matches(QStringLiteral("Picking Licks && mp4 || pdf"), haystack));
    QVERIFY(!LibrarySearchExpression::matches(QStringLiteral("Picking Licks && pdf"), haystack));
    QVERIFY(LibrarySearchExpression::matches(QStringLiteral("pdf || mp4"), haystack));
}

void TestViewModels::testLibrarySearchExpressionRegex() {
    const QString haystack = QStringLiteral("picking licks/picking_01.mp4 video");

    QVERIFY(LibrarySearchExpression::matches(QStringLiteral("/pick.*mp4/"), haystack));
    QVERIFY(!LibrarySearchExpression::matches(QStringLiteral("/nomatch/"), haystack));
}

void TestViewModels::testLibraryLinkModelBooleanSearch() {
    const std::optional<qlonglong> gpSongId = createSong(QStringLiteral("Picking Licks"));
    const std::optional<qlonglong> videoSongId = createSong(QStringLiteral("Picking Lick 1"));
    const std::optional<qlonglong> docSongId = createSong(QStringLiteral("Picking Sheet"));
    QVERIFY(gpSongId.has_value());
    QVERIFY(videoSongId.has_value());
    QVERIFY(docSongId.has_value());

    MediaFile gpFile;
    gpFile.songId = *gpSongId;
    gpFile.filePath = QStringLiteral("/course/picking.gp5");
    gpFile.fileType = QStringLiteral("gp5");
    gpFile.mediaKind = MediaKind::GuitarPro;
    gpFile.sourceType = MediaSourceType::Local;
    gpFile.sourceRelativePath = QStringLiteral("Picking Licks/picking.gp5");
    gpFile.canBePracticed = true;
    QVERIFY(m_mediaFileRepo.createMediaFile(gpFile).has_value());

    MediaFile videoFile;
    videoFile.songId = *videoSongId;
    videoFile.filePath = QStringLiteral("/course/lick1.mp4");
    videoFile.fileType = QStringLiteral("mp4");
    videoFile.mediaKind = MediaKind::Video;
    videoFile.sourceType = MediaSourceType::Local;
    videoFile.sourceRelativePath = QStringLiteral("Picking Licks/lick1.mp4");
    QVERIFY(m_mediaFileRepo.createMediaFile(videoFile).has_value());

    MediaFile docFile;
    docFile.songId = *docSongId;
    docFile.filePath = QStringLiteral("/course/sheet.pdf");
    docFile.fileType = QStringLiteral("pdf");
    docFile.mediaKind = MediaKind::Document;
    docFile.sourceType = MediaSourceType::Local;
    docFile.sourceRelativePath = QStringLiteral("Other/sheet.pdf");
    QVERIFY(m_mediaFileRepo.createMediaFile(docFile).has_value());

    LibraryLinkModel model(m_songRepo, m_mediaFileRepo, m_linkGroupRepo, m_fileRelationRepo,
                           m_artistRepo);
    model.reload();

    QCOMPARE(model.rowCount(), 3);

    model.setSearchText(QStringLiteral("Picking Licks && mp4"));
    QTest::qWait(250);
    QCOMPARE(model.rowCount(), 1);

    model.setSearchText(QStringLiteral("Picking Licks && mp4 || pdf"));
    QTest::qWait(250);
    QCOMPARE(model.rowCount(), 2);

    const QVariantList visibleIds = model.visibleUnlinkedSongIds();
    QCOMPARE(visibleIds.size(), 2);

    const QVariantList ordered = model.orderSongIdsForLinking(visibleIds);
    QCOMPARE(ordered.size(), 2);
    QCOMPARE(ordered.first().toLongLong(), *docSongId);

    model.setSearchText(QStringLiteral("Picking"));
    const QVariantList allVisible = model.visibleUnlinkedSongIds();
    QCOMPARE(allVisible.size(), 3);
    QCOMPARE(model.orderSongIdsForLinking(allVisible).first().toLongLong(), *gpSongId);

    model.setSearchText(QStringLiteral("Picking Licks"));
    QCOMPARE(model.defaultLinkTitle(), QStringLiteral("Picking Licks"));

    model.setSearchText(QStringLiteral("Picking && mp4"));
    QVERIFY(model.defaultLinkTitle().isEmpty());
}

void TestViewModels::testLibraryLinkModelContainersOnlyShowsGroupTitle() {
    const std::optional<qlonglong> primarySongId = createSong(QStringLiteral("Primary File Name"));
    const std::optional<qlonglong> secondarySongId = createSong(QStringLiteral("Secondary File"));
    QVERIFY(primarySongId.has_value());
    QVERIFY(secondarySongId.has_value());

    MediaFile primaryFile;
    primaryFile.songId = *primarySongId;
    primaryFile.filePath = QStringLiteral("/group/primary.gp5");
    primaryFile.fileType = QStringLiteral("gp5");
    primaryFile.mediaKind = MediaKind::GuitarPro;
    primaryFile.sourceType = MediaSourceType::Local;
    primaryFile.sourceRelativePath = QStringLiteral("Group/primary.gp5");
    primaryFile.canBePracticed = true;
    const std::optional<qlonglong> primaryMediaId = m_mediaFileRepo.createMediaFile(primaryFile);
    QVERIFY(primaryMediaId.has_value());

    MediaFile secondaryFile;
    secondaryFile.songId = *secondarySongId;
    secondaryFile.filePath = QStringLiteral("/group/secondary.mp4");
    secondaryFile.fileType = QStringLiteral("mp4");
    secondaryFile.mediaKind = MediaKind::Video;
    secondaryFile.sourceType = MediaSourceType::Local;
    secondaryFile.sourceRelativePath = QStringLiteral("Group/secondary.mp4");
    const std::optional<qlonglong> secondaryMediaId =
        m_mediaFileRepo.createMediaFile(secondaryFile);
    QVERIFY(secondaryMediaId.has_value());

    LinkGroup group;
    group.title = QStringLiteral("Course Group Title");
    group.primarySongId = *primarySongId;
    group.primaryMediaId = *primaryMediaId;
    const std::optional<qlonglong> groupId = m_linkGroupRepo.createGroup(group);
    QVERIFY(groupId.has_value());
    QVERIFY(m_fileRelationRepo.linkToPrimary(*primaryMediaId, *secondaryMediaId));

    LibraryLinkModel model(m_songRepo, m_mediaFileRepo, m_linkGroupRepo, m_fileRelationRepo,
                           m_artistRepo);
    model.reload();
    model.setContainersOnly(true);

    QCOMPARE(model.rowCount(), 1);

    const QModelIndex index = model.index(0, 0);
    QCOMPARE(model.data(index, LibraryLinkModel::TitleRole).toString(),
             QStringLiteral("Course Group Title"));

    const QVariantList memberIds{*primarySongId, *secondarySongId};
    model.clearSongsLinkState(memberIds);
    model.setContainersOnly(false);

    QCOMPARE(model.isSongLinked(*primarySongId), false);
    QCOMPARE(model.isSongLinked(*secondarySongId), false);
}

void TestViewModels::testLibraryLinkModelFolderPrefixFilter() {
    const std::optional<qlonglong> parentSongId = createSong(QStringLiteral("Parent Song"));
    const std::optional<qlonglong> childSongId = createSong(QStringLiteral("Child Song"));
    const std::optional<qlonglong> otherSongId = createSong(QStringLiteral("Other Song"));
    QVERIFY(parentSongId.has_value());
    QVERIFY(childSongId.has_value());
    QVERIFY(otherSongId.has_value());

    MediaFile parentFile;
    parentFile.songId = *parentSongId;
    parentFile.filePath = QStringLiteral("/course/parent.gp5");
    parentFile.fileType = QStringLiteral("gp5");
    parentFile.mediaKind = MediaKind::GuitarPro;
    parentFile.sourceType = MediaSourceType::Local;
    parentFile.sourceRelativePath = QStringLiteral("Course/parent.gp5");
    parentFile.canBePracticed = true;
    QVERIFY(m_mediaFileRepo.createMediaFile(parentFile).has_value());

    MediaFile childFile;
    childFile.songId = *childSongId;
    childFile.filePath = QStringLiteral("/course/advanced/child.mp4");
    childFile.fileType = QStringLiteral("mp4");
    childFile.mediaKind = MediaKind::Video;
    childFile.sourceType = MediaSourceType::Local;
    childFile.sourceRelativePath = QStringLiteral("Course/Advanced/child.mp4");
    QVERIFY(m_mediaFileRepo.createMediaFile(childFile).has_value());

    MediaFile otherFile;
    otherFile.songId = *otherSongId;
    otherFile.filePath = QStringLiteral("/other/sheet.pdf");
    otherFile.fileType = QStringLiteral("pdf");
    otherFile.mediaKind = MediaKind::Document;
    otherFile.sourceType = MediaSourceType::Local;
    otherFile.sourceRelativePath = QStringLiteral("Other/sheet.pdf");
    QVERIFY(m_mediaFileRepo.createMediaFile(otherFile).has_value());

    LibraryLinkModel model(m_songRepo, m_mediaFileRepo, m_linkGroupRepo, m_fileRelationRepo,
                           m_artistRepo);
    model.reload();
    QCOMPARE(model.rowCount(), 3);

    model.setFolderFilter(QStringLiteral("Course"));
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.fileCountForFolder(QStringLiteral("Course"), true), 2);
    QCOMPARE(model.fileCountForFolder(QStringLiteral("Course/Advanced"), true), 1);
    QCOMPARE(model.fileCountForFolder(QStringLiteral("Course"), false), 1);

    model.setFolderFilter(QStringLiteral("Course/Advanced"));
    QCOMPARE(model.rowCount(), 1);

    model.setFolderFilter(QStringLiteral("Other"));
    QCOMPARE(model.rowCount(), 1);
}

void TestViewModels::testCatalogViewCacheFeedsBothModels() {
    const std::optional<qlonglong> songId =
        createSong(QStringLiteral("Shared Cache Song"), TestBpm::kPractice);
    QVERIFY(songId.has_value());
    QVERIFY(createMediaFile(*songId, QStringLiteral("/tmp/shared.gp5"), MediaKind::GuitarPro, true)
                .has_value());

    const CatalogViewCache cache = CatalogViewCache::load(CatalogSnapshot::Dependencies{
        .songRepo = m_songRepo,
        .mediaFileRepo = m_mediaFileRepo,
        .artistRepo = m_artistRepo,
        .linkGroupRepo = m_linkGroupRepo,
        .fileRelationRepo = m_fileRelationRepo,
    });

    SongModel songModel(m_songRepo, m_mediaFileRepo, m_artistRepo, m_linkGroupRepo,
                        m_fileRelationRepo);
    LibraryLinkModel libraryModel(m_songRepo, m_mediaFileRepo, m_linkGroupRepo, m_fileRelationRepo,
                                  m_artistRepo);

    songModel.applyViewCache(cache);
    libraryModel.applyViewCache(cache);

    QCOMPARE(songModel.rowCount(), 1);
    QCOMPARE(libraryModel.rowCount(), 1);
    QVERIFY(songModel.catalogReady());
    QVERIFY(libraryModel.loaded());
    QCOMPARE(libraryModel.fileCountForFolder(QStringLiteral("/"), true), 1);
}

void TestViewModels::testLibraryLinkModelSkipsReloadWhenLoaded() {
    const std::optional<qlonglong> songId = createSong(QStringLiteral("Loaded Flag Song"));
    QVERIFY(songId.has_value());
    QVERIFY(createMediaFile(*songId, QStringLiteral("/tmp/loaded.gp5"), MediaKind::GuitarPro, true)
                .has_value());

    LibraryLinkModel model(m_songRepo, m_mediaFileRepo, m_linkGroupRepo, m_fileRelationRepo,
                           m_artistRepo);
    QVERIFY(!model.loaded());

    const CatalogViewCache cache = CatalogViewCache::load(CatalogSnapshot::Dependencies{
        .songRepo = m_songRepo,
        .mediaFileRepo = m_mediaFileRepo,
        .artistRepo = m_artistRepo,
        .linkGroupRepo = m_linkGroupRepo,
        .fileRelationRepo = m_fileRelationRepo,
    });
    model.applyViewCache(cache);

    QVERIFY(model.loaded());
    QCOMPARE(model.rowCount(), 1);
}

QTEST_MAIN(TestViewModels)
