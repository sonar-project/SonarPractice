#ifndef APPLICATIONBOOTSTRAP_H
#define APPLICATIONBOOTSTRAP_H

#include "AppSettings.h"
#include "AudioConfigController.h"
#include "CatalogViewCache.h"
#include "ImportService.h"
#include "LibraryLinkModel.h"
#include "LinkGroupService.h"
#include "MediaFileListModel.h"
#include "PathResolver.h"
#include "PracticeAssetController.h"
#include "PracticeSessionController.h"
#include "PracticeTrackerController.h"
#include "ReminderController.h"
#include "SongModel.h"
#include "SqliteMediaFileRepository.h"

// Repositories
#include "SqliteArtistRepository.h"
#include "SqliteAudioConfigPresetRepository.h"
#include "SqliteConnection.h"
#include "SqliteFileRelationRepository.h"
#include "SqliteLinkGroupRepository.h"
#include "SqlitePracticeAssetRepository.h"
#include "SqlitePracticeJournalRepository.h"
#include "SqlitePracticeNoticeRepository.h"
#include "SqliteReminderConditionRepository.h"
#include "SqliteReminderCompletionRepository.h"
#include "SqliteReminderRepository.h"
#include "SqliteSongRepository.h"
#include "SqliteTuningRepository.h"
#include "SqliteUserRepository.h"

// Services
#include "DesktopLauncher.h"
#include "LibraryManager.h"
#include "ApplicationErrorLog.h"
#include "SettingsConfigProvider.h"

#include <QObject>
#include <QString>
#include <memory>

/**
 * \brief Initializes and manages the application's core components.
 *
 * Responsible for bootstrapping the application in two phases:
 *
 * 1. Shell phase (initializeShell): Opens the database, creates repositories,
 *    services and models. The UI shell can be shown once this completes.
 *
 * 2. Catalog phase (beginCatalogLoad): Loads the full catalog view cache
 *    asynchronously on a worker thread so the UI stays responsive.
 *
 * Typical call sequence:
 * \code
 *   bootstrap.initializeShell();
 *   bootstrap.scheduleLoadShellData();
 *   bootstrap.beginCatalogLoad();
 * \endcode
 */
class ApplicationBootstrap : public QObject {
    Q_OBJECT

  public:
    /**
     * \brief Constructs an ApplicationBootstrap object.
     *
     * \param appSettings The application settings to be used.
     * \param parent      The parent QObject.
     */
    explicit ApplicationBootstrap(AppSettings &appSettings, QObject *parent = nullptr);

    // Non-copyable, non-movable – owns unique resources
    ApplicationBootstrap(const ApplicationBootstrap &) = delete;
    ApplicationBootstrap &operator=(const ApplicationBootstrap &) = delete;
    ApplicationBootstrap(ApplicationBootstrap &&) = delete;
    ApplicationBootstrap &operator=(ApplicationBootstrap &&) = delete;

    // -------------------------------------------------------------------------
    // State queries
    // -------------------------------------------------------------------------

    /** \return True once initializeShell() has succeeded. */
    [[nodiscard]] bool isShellReady() const;

    /** \return True once the catalog view cache has been applied. */
    [[nodiscard]] bool isCatalogReady() const;

    /** \return True while a background catalog load is in progress. */
    [[nodiscard]] bool isCatalogLoadRunning() const;

    // -------------------------------------------------------------------------
    // Initialization
    // -------------------------------------------------------------------------

    /**
     * \brief Opens the database and sets up all repositories, services and models.
     *
     * Must be called before beginCatalogLoad() or scheduleLoadShellData().
     * Emits shellReadyChanged() on success.
     *
     * \return True on success, false if the database could not be opened or
     *         the schema could not be created.
     */
    [[nodiscard]] bool initializeShell();

    /**
     * \brief Schedules loadShellData() via a zero-delay QTimer.
     *
     * Call after initializeShell() to let the event loop settle before
     * performing the first reminder / date reload.
     */
    void scheduleLoadShellData();

    /**
     * \brief Starts an asynchronous catalog load on a worker QThread.
     *
     * No-op if the catalog is already ready or a load is already running.
     * Calls initializeShell() implicitly if the shell is not yet ready.
     *
     * Emits catalogLoadRunningChanged() when the worker starts,
     * and catalogReadyChanged() + readyChanged() when it finishes.
     * Emits catalogLoadFailed() on error.
     */
    void beginCatalogLoad();

    // -------------------------------------------------------------------------
    // Accessors – raw pointers are non-owning, never null after initializeShell()
    // -------------------------------------------------------------------------

    [[nodiscard]] ImportService *importService() const;
    [[nodiscard]] SongModel *songModel() const;
    [[nodiscard]] MediaFileListModel *mediaFileModel() const;
    [[nodiscard]] SqliteMediaFileRepository *mediaFileRepo() const;
    [[nodiscard]] PracticeSessionController *practiceSession() const;
    [[nodiscard]] ApplicationErrorLog *errorLog() const;
    [[nodiscard]] PracticeTrackerController *practiceTracker() const;
    [[nodiscard]] ReminderController *reminderController() const;
    [[nodiscard]] PracticeAssetController *practiceAssetController() const;
    [[nodiscard]] AudioConfigController *audioConfigController() const;
    [[nodiscard]] LinkGroupService *linkGroupService() const;
    [[nodiscard]] LibraryLinkModel *libraryLinkModel() const;
    [[nodiscard]] PathResolver *pathResolver() const;

  signals:
    /** Emitted once the shell (database + services) is ready. */
    void shellReadyChanged();

    /** Emitted once the catalog view cache has been applied. */
    void catalogReadyChanged();

    /** Emitted when a background catalog load starts or finishes. */
    void catalogLoadRunningChanged();

    /** Emitted when both shell and catalog are ready. */
    void readyChanged();

    /**
     * \brief Emitted when the background catalog load fails.
     * \param errorMessage A human-readable description of the error.
     */
    void catalogLoadFailed(const QString &errorMessage);

  private:
    // Called on the main thread by the worker thread (QueuedConnection)
    void onCatalogLoaded(const CatalogViewCachePtr &cache);
    void onCatalogLoadFailed(const QString &errorMessage);

    [[nodiscard]] bool openCoreDatabase();
    void loadShellData();
    void applyCatalogViewCache(const CatalogViewCachePtr &cache);
    void wireServices();

    // -------------------------------------------------------------------------
    // Data members
    // -------------------------------------------------------------------------
    AppSettings &m_appSettings;

    bool m_shellReady{false};
    bool m_catalogReady{false};
    bool m_catalogLoadRunning{false};
    CatalogViewCachePtr m_cachedViewCache;

    // Database
    std::unique_ptr<SqliteConnection> m_connection;

    // Repositories
    std::unique_ptr<SqliteArtistRepository> m_artistRepo;
    std::unique_ptr<SqliteTuningRepository> m_tuningRepo;
    std::unique_ptr<SqliteSongRepository> m_songRepo;
    std::unique_ptr<SqliteMediaFileRepository> m_mediaFileRepo;
    std::unique_ptr<SqliteLinkGroupRepository> m_linkGroupRepo;
    std::unique_ptr<SqliteFileRelationRepository> m_fileRelationRepo;
    std::unique_ptr<SqlitePracticeJournalRepository> m_journalRepo;
    std::unique_ptr<SqlitePracticeAssetRepository> m_practiceAssetRepo;
    std::unique_ptr<SqlitePracticeNoticeRepository> m_noticeRepo;
    std::unique_ptr<SqliteReminderRepository> m_reminderRepo;
    std::unique_ptr<SqliteReminderConditionRepository> m_reminderConditionRepo;
    std::unique_ptr<SqliteReminderCompletionRepository> m_reminderCompletionRepo;
    std::unique_ptr<SqliteAudioConfigPresetRepository> m_audioPresetRepo;
    std::unique_ptr<SqliteUserRepository> m_userRepo;

    // Services & helpers
    std::unique_ptr<LibraryManager> m_libraryManager;
    std::unique_ptr<SettingsConfigProvider> m_configProvider;
    std::unique_ptr<ImportService> m_importService;
    std::unique_ptr<LinkGroupService> m_linkGroupService;
    std::unique_ptr<PathResolver> m_pathResolver;
    std::unique_ptr<DesktopLauncher> m_launcher;
    std::unique_ptr<ApplicationErrorLog> m_errorLog;

    // Models
    std::unique_ptr<SongModel> m_songModel;
    std::unique_ptr<MediaFileListModel> m_mediaFileModel;
    std::unique_ptr<LibraryLinkModel> m_libraryLinkModel;

    // Controllers
    std::unique_ptr<PracticeSessionController> m_practiceSession;
    std::unique_ptr<PracticeTrackerController> m_practiceTracker;
    std::unique_ptr<ReminderController> m_reminderController;
    std::unique_ptr<PracticeAssetController> m_practiceAssetController;
    std::unique_ptr<AudioConfigController> m_audioConfigController;
};

#endif // APPLICATIONBOOTSTRAP_H