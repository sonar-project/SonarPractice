#include "ApplicationBootstrap.h"

#include "CatalogSnapshot.h"
#include "CatalogViewCache.h"
#include "DatabaseSchema.h"
#include "RepositoryFactory.h"
#include "User.h"

#include <QDate>
#include <QDir>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QSqlDatabase>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>

namespace {

    Q_LOGGING_CATEGORY(lcStartup, "sonarp.startup")

    class WorkerConnectionGuard {
      public:
        WorkerConnectionGuard(SqliteConnection &connection, QString connectionName)
            : m_connection(connection), m_connectionName(std::move(connectionName)) {}

        ~WorkerConnectionGuard() { release(); }

        WorkerConnectionGuard(const WorkerConnectionGuard &) = delete;
        WorkerConnectionGuard &operator=(const WorkerConnectionGuard &) = delete;

      private:
        void release() {
            m_connection.close();
            if (QSqlDatabase::contains(m_connectionName)) {
                QSqlDatabase::removeDatabase(m_connectionName);
            }
        }

        SqliteConnection &m_connection;
        QString m_connectionName;
    };

    void logStartupCheckpoint(QElapsedTimer &timer, const char *label) {
        qInfo(lcStartup) << label << timer.elapsed() << "ms";
    }

    void ensureDefaultAdminUser(SqliteUserRepository &userRepo) {
        const std::optional<User> existing = userRepo.getUser(1);
        if (!existing.has_value()) {
            User admin;
            admin.name = QStringLiteral("admin");
            admin.role = QStringLiteral("admin");
            userRepo.createUser(admin);
            return;
        }

        if (!existing->isAdmin()) {
            User admin = *existing;
            admin.name = QStringLiteral("admin");
            admin.role = QStringLiteral("admin");
            userRepo.updateUser(admin);
        }
    }

} // namespace

// -----------------------------------------------------------------------------
// Construction
// -----------------------------------------------------------------------------

ApplicationBootstrap::ApplicationBootstrap(AppSettings &appSettings, QObject *parent)
    : QObject(parent), m_appSettings(appSettings) {
    qRegisterMetaType<CatalogViewCachePtr>();
}

// -----------------------------------------------------------------------------
// State queries
// -----------------------------------------------------------------------------

bool ApplicationBootstrap::isShellReady() const { return m_shellReady; }
bool ApplicationBootstrap::isCatalogReady() const { return m_catalogReady; }
bool ApplicationBootstrap::isCatalogLoadRunning() const { return m_catalogLoadRunning; }

// -----------------------------------------------------------------------------
// Shell initialization
// -----------------------------------------------------------------------------

bool ApplicationBootstrap::openCoreDatabase() {
    const QString appDataPath =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(appDataPath);

    const QString dbPath = AppSettings::databasePath();

    if (m_connection == nullptr) {
        m_connection = std::make_unique<SqliteConnection>(QStringLiteral("SonarPracticeMain"));
    }

    if (!m_connection->isOpen()) {
        if (!m_connection->open(dbPath)) {
            qCritical(lcStartup) << "Failed to open database:" << m_connection->lastError();
            return false;
        }
    }

    if (m_userRepo != nullptr) {
        return true;
    }

    DatabaseSchema schema(*m_connection);
    if (!schema.createAllTables()) {
        qCritical(lcStartup) << "Failed to create database schema";
        return false;
    }

    auto repos = RepositoryFactory::create({.connection = *m_connection});
    m_artistRepo = std::move(repos.artist);
    m_tuningRepo = std::move(repos.tuning);
    m_songRepo = std::move(repos.song);
    m_mediaFileRepo = std::move(repos.mediaFile);
    m_linkGroupRepo = std::move(repos.linkGroup);
    m_fileRelationRepo = std::move(repos.fileRelation);
    m_journalRepo = std::move(repos.journal);
    m_practiceAssetRepo = std::move(repos.practiceAsset);
    m_noticeRepo = std::move(repos.notice);
    m_reminderRepo = std::move(repos.reminder);
    m_reminderConditionRepo = std::move(repos.reminderCondition);
    m_reminderCompletionRepo = std::move(repos.reminderCompletion);
    m_audioPresetRepo = std::move(repos.audioPreset);
    m_userRepo = std::move(repos.user);

    m_libraryManager = std::make_unique<LibraryManager>();
    ensureDefaultAdminUser(*m_userRepo);

    m_configProvider = std::make_unique<SettingsConfigProvider>(m_appSettings);

    m_linkGroupService = std::make_unique<LinkGroupService>(LinkGroupService::Dependencies{
        .linkGroupRepo = *m_linkGroupRepo,
        .fileRelationRepo = *m_fileRelationRepo,
        .mediaFileRepo = *m_mediaFileRepo,
        .songRepo = *m_songRepo,
    });

    m_importService = std::make_unique<ImportService>(ImportService::Dependencies{
        .artistRepo = *m_artistRepo,
        .tuningRepo = *m_tuningRepo,
        .songRepo = *m_songRepo,
        .mediaFileRepo = *m_mediaFileRepo,
        .libraryManager = *m_libraryManager,
        .config = *m_configProvider,
        .appSettings = m_appSettings,
        .databasePath = dbPath,
    });

    m_pathResolver = std::make_unique<PathResolver>(m_appSettings.managedStorageRoot());
    m_launcher = std::make_unique<DesktopLauncher>();
    m_errorLog = std::make_unique<ApplicationErrorLog>(this);

    m_songModel = std::make_unique<SongModel>(*m_songRepo, *m_mediaFileRepo, *m_artistRepo,
                                              *m_linkGroupRepo, *m_fileRelationRepo);

    m_mediaFileModel = std::make_unique<MediaFileListModel>(*m_mediaFileRepo, *m_linkGroupRepo,
                                                            *m_fileRelationRepo);

    m_libraryLinkModel = std::make_unique<LibraryLinkModel>(
        *m_songRepo, *m_mediaFileRepo, *m_linkGroupRepo, *m_fileRelationRepo, *m_artistRepo);

    // PracticeAssetController must be created before ReminderController
    m_practiceAssetController =
        std::make_unique<PracticeAssetController>(*m_practiceAssetRepo, *m_mediaFileRepo, nullptr);

    m_practiceSession =
        std::make_unique<PracticeSessionController>(PracticeSessionController::Dependencies{
            .mediaRepo = *m_mediaFileRepo,
            .pathResolver = *m_pathResolver,
            .launcher = *m_launcher,
            .errorLog = *m_errorLog,
        });

    m_practiceTracker = std::make_unique<PracticeTrackerController>(
        *m_journalRepo, *m_noticeRepo, *m_reminderRepo, *m_reminderConditionRepo, *m_errorLog);

    m_reminderController = std::make_unique<ReminderController>(
        *m_reminderRepo, *m_reminderConditionRepo, *m_journalRepo, *m_reminderCompletionRepo,
        *m_songRepo, *m_practiceAssetController);

    m_audioConfigController =
        std::make_unique<AudioConfigController>(AudioConfigController::Dependencies{
            .mediaRepo = *m_mediaFileRepo,
            .presetRepo = *m_audioPresetRepo,
            .pathResolver = *m_pathResolver,
            .songRepo = *m_songRepo,
            .tuningRepo = *m_tuningRepo,
        });

    wireServices();
    return true;
}

bool ApplicationBootstrap::initializeShell() {
    if (m_shellReady) {
        return true;
    }

    QElapsedTimer timer;
    timer.start();

    if (!openCoreDatabase()) {
        return false;
    }
    logStartupCheckpoint(timer, "initializeShell: openCoreDatabase");

    m_shellReady = true;
    emit shellReadyChanged();
    logStartupCheckpoint(timer, "initializeShell: complete");
    return true;
}

void ApplicationBootstrap::loadShellData() {
    m_reminderController->setFilterDate(QDate::currentDate());
    m_reminderController->reloadDayReminders();
    if (m_audioConfigController != nullptr) {
        m_audioConfigController->refreshTabTuningList();
    }
}

void ApplicationBootstrap::scheduleLoadShellData() {
    QTimer::singleShot(0, this, [this]() {
        if (!m_shellReady) {
            return;
        }
        QElapsedTimer timer;
        timer.start();
        loadShellData();
        logStartupCheckpoint(timer, "scheduleLoadShellData: loadShellData");
    });
}

// -----------------------------------------------------------------------------
// Catalog loading
// -----------------------------------------------------------------------------

void ApplicationBootstrap::beginCatalogLoad() {
    if (m_catalogReady || m_catalogLoadRunning) {
        return;
    }

    if (!m_shellReady && !initializeShell()) {
        return;
    }

    m_catalogLoadRunning = true;
    emit catalogLoadRunningChanged();

    const QString dbPath = AppSettings::databasePath();

    QThread *thread = QThread::create([this, dbPath]() {
        QElapsedTimer workerTimer;
        workerTimer.start();

        const QString connectionName =
            QStringLiteral("SonarPracticeCatalogWorker_%1")
                .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));

        SqliteConnection workerConnection(connectionName);
        WorkerConnectionGuard connectionGuard(workerConnection, connectionName);

        if (!workerConnection.open(dbPath)) {
            const QString error = workerConnection.lastError();
            QMetaObject::invokeMethod(this, "onCatalogLoadFailed", Qt::QueuedConnection,
                                      Q_ARG(QString, error));
            return;
        }

        SqliteSongRepository songRepo(workerConnection);
        SqliteMediaFileRepository mediaFileRepo(workerConnection);
        SqliteArtistRepository artistRepo(workerConnection);
        SqliteLinkGroupRepository linkGroupRepo(workerConnection);
        SqliteFileRelationRepository fileRelationRepo(workerConnection);

        auto cache =
            std::make_shared<CatalogViewCache>(CatalogViewCache::load(CatalogSnapshot::Dependencies{
                .songRepo = songRepo,
                .mediaFileRepo = mediaFileRepo,
                .artistRepo = artistRepo,
                .linkGroupRepo = linkGroupRepo,
                .fileRelationRepo = fileRelationRepo,
            }));

        logStartupCheckpoint(workerTimer, "worker: CatalogViewCache::load");

        QMetaObject::invokeMethod(
            this, [this, cache]() { onCatalogLoaded(cache); }, Qt::QueuedConnection);
    });

    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void ApplicationBootstrap::applyCatalogViewCache(const CatalogViewCachePtr &cache) {
    if (cache == nullptr) {
        return;
    }

    QElapsedTimer timer;
    timer.start();

    m_cachedViewCache = cache;
    m_songModel->applyViewCache(*cache);
    m_libraryLinkModel->applyViewCache(*cache);
    logStartupCheckpoint(timer, "applyCatalogViewCache");

    const bool wasReady = m_catalogReady;
    m_catalogReady = true;
    if (!wasReady) {
        emit catalogReadyChanged();
        emit readyChanged();
    }
}

void ApplicationBootstrap::onCatalogLoaded(const CatalogViewCachePtr &cache) {
    m_catalogLoadRunning = false;
    emit catalogLoadRunningChanged();
    applyCatalogViewCache(cache);
}

void ApplicationBootstrap::onCatalogLoadFailed(const QString &errorMessage) {
    m_catalogLoadRunning = false;
    emit catalogLoadRunningChanged();
    qCritical(lcStartup) << "Catalog load failed:" << errorMessage;
    emit catalogLoadFailed(errorMessage);
}

// -----------------------------------------------------------------------------
// Service wiring
// -----------------------------------------------------------------------------

void ApplicationBootstrap::wireServices() {
    m_reminderController->setFilterDate(m_practiceTracker->selectedDate());

    QObject::connect(m_practiceTracker.get(), &PracticeTrackerController::selectedDateChanged,
                     m_reminderController.get(), [this]() {
                         m_reminderController->setFilterDate(m_practiceTracker->selectedDate());
                     });

    QObject::connect(m_importService.get(), &ImportService::importFinished, m_songModel.get(),
                     &SongModel::reload);
    QObject::connect(m_importService.get(), &ImportService::importFinished,
                     m_libraryLinkModel.get(), &LibraryLinkModel::reload);
    QObject::connect(m_importService.get(), &ImportService::importFinished,
                     m_reminderController.get(), &ReminderController::invalidateDayCache);
    QObject::connect(m_importService.get(), &ImportService::importFinished,
                     m_reminderController.get(), &ReminderController::reloadDayReminders);

    QObject::connect(m_practiceTracker.get(), &PracticeTrackerController::journalSaved,
                     m_reminderController.get(), &ReminderController::invalidateDayCache);
    QObject::connect(m_practiceTracker.get(), &PracticeTrackerController::journalSaved,
                     m_reminderController.get(), &ReminderController::reloadDayReminders);

    QObject::connect(m_linkGroupService.get(), &LinkGroupService::groupsChanged, m_songModel.get(),
                     &SongModel::reload);
    QObject::connect(m_linkGroupService.get(), &LinkGroupService::groupsChanged,
                     m_libraryLinkModel.get(), &LibraryLinkModel::reload);
}

// -----------------------------------------------------------------------------
// Accessors
// -----------------------------------------------------------------------------

ImportService *ApplicationBootstrap::importService() const { return m_importService.get(); }
SongModel *ApplicationBootstrap::songModel() const { return m_songModel.get(); }
MediaFileListModel *ApplicationBootstrap::mediaFileModel() const { return m_mediaFileModel.get(); }
SqliteMediaFileRepository *ApplicationBootstrap::mediaFileRepo() const {
    return m_mediaFileRepo.get();
}
PracticeSessionController *ApplicationBootstrap::practiceSession() const {
    return m_practiceSession.get();
}
ApplicationErrorLog *ApplicationBootstrap::errorLog() const { return m_errorLog.get(); }
PracticeTrackerController *ApplicationBootstrap::practiceTracker() const {
    return m_practiceTracker.get();
}
ReminderController *ApplicationBootstrap::reminderController() const {
    return m_reminderController.get();
}
PracticeAssetController *ApplicationBootstrap::practiceAssetController() const {
    return m_practiceAssetController.get();
}
AudioConfigController *ApplicationBootstrap::audioConfigController() const {
    return m_audioConfigController.get();
}
LinkGroupService *ApplicationBootstrap::linkGroupService() const {
    return m_linkGroupService.get();
}
LibraryLinkModel *ApplicationBootstrap::libraryLinkModel() const {
    return m_libraryLinkModel.get();
}
PathResolver *ApplicationBootstrap::pathResolver() const { return m_pathResolver.get(); }
