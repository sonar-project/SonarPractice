#include "StartupController.h"

#include "StorageDirectoryPicker.h"

#include <QDir>
#include <QQmlContext>

// -----------------------------------------------------------------------------
// Construction
// -----------------------------------------------------------------------------

StartupController::StartupController(AppSettings &appSettings, ApplicationBootstrap &bootstrap,
                                     QObject *parent)
    : QObject(parent), m_appSettings(appSettings), m_bootstrap(bootstrap) {
    connectBootstrapSignals();
}

// -----------------------------------------------------------------------------
// Property accessors
// -----------------------------------------------------------------------------

bool StartupController::needsConfiguration() const { return !AppSettings::databaseExists(); }
bool StartupController::shellReady() const { return m_bootstrap.isShellReady(); }
bool StartupController::catalogReady() const { return m_bootstrap.isCatalogReady(); }
bool StartupController::initializing() const { return m_initializing; }

const QString &StartupController::initStatusText() const { return m_initStatusText; }

void StartupController::setQmlContext(QQmlContext *context) { m_qmlContext = context; }

// -----------------------------------------------------------------------------
// Bootstrap signal wiring
// -----------------------------------------------------------------------------

void StartupController::connectBootstrapSignals() {
    connect(&m_bootstrap, &ApplicationBootstrap::shellReadyChanged, this, [this]() {
        registerContextProperties();
        emit shellReadyChanged();
        updateInitializingState();
    });

    connect(&m_bootstrap, &ApplicationBootstrap::catalogReadyChanged, this, [this]() {
        emit catalogReadyChanged();
        updateInitializingState();
    });

    connect(&m_bootstrap, &ApplicationBootstrap::catalogLoadRunningChanged, this,
            [this]() { updateInitializingState(); });

    connect(&m_bootstrap, &ApplicationBootstrap::readyChanged, this,
            &StartupController::readyChanged);

    connect(&m_bootstrap, &ApplicationBootstrap::catalogLoadFailed, this,
            [this](const QString &error) {
                // Surface the error in the status text so QML can show it
                m_initStatusText = tr("Load error: %1").arg(error);
                m_initializing = false;
                emit initializingChanged();
                emit initStatusTextChanged();
            });
}

void StartupController::updateInitializingState() {
    const bool nowInitializing = m_bootstrap.isCatalogLoadRunning() ||
                                 (m_bootstrap.isShellReady() && !m_bootstrap.isCatalogReady());

    const QString status = nowInitializing ? tr("Loading library…") : QString();

    if (m_initializing == nowInitializing && m_initStatusText == status) {
        return;
    }

    m_initializing = nowInitializing;
    m_initStatusText = status;
    emit initializingChanged();
    emit initStatusTextChanged();
}

// -----------------------------------------------------------------------------
// Private helpers
// -----------------------------------------------------------------------------

void StartupController::ensureManagedStorageDirectoryIfNeeded(const AppSettings &appSettings) {
    if (appSettings.storageStrategyEnum() == StorageStrategy::Link) {
        return;
    }

    const QString root = appSettings.managedStorageRoot();
    if (!root.isEmpty()) {
        QDir().mkpath(root);
    }
}

// -----------------------------------------------------------------------------
// Invokables – first-run path (setup wizard)
// -----------------------------------------------------------------------------

bool StartupController::initializeDatabase() {
    m_appSettings.ensureDefaults();
    ensureManagedStorageDirectoryIfNeeded(m_appSettings);

    if (!m_bootstrap.initializeShell()) {
        return false;
    }

    m_bootstrap.scheduleLoadShellData();
    m_bootstrap.beginCatalogLoad();
    return true;
}

// -----------------------------------------------------------------------------
// Invokables – normal async startup path
// -----------------------------------------------------------------------------

void StartupController::beginAsyncInitialization() {
    if (needsConfiguration()) {
        return;
    }

    m_appSettings.ensureDefaults();
    ensureManagedStorageDirectoryIfNeeded(m_appSettings);

    // initializeShell() emits shellReadyChanged → connectBootstrapSignals
    // registers context properties and forwards the signal to QML
    if (!m_bootstrap.initializeShell()) {
        return;
    }

    m_bootstrap.scheduleLoadShellData();
    m_bootstrap.beginCatalogLoad();
}

// -----------------------------------------------------------------------------
// Context property registration
// -----------------------------------------------------------------------------

void StartupController::registerContextProperties() {
    if (m_qmlContext == nullptr || !m_bootstrap.isShellReady()) {
        return;
    }

    m_qmlContext->setContextProperty(QStringLiteral("importService"), m_bootstrap.importService());
    m_qmlContext->setContextProperty(QStringLiteral("songModel"), m_bootstrap.songModel());
    m_qmlContext->setContextProperty(QStringLiteral("mediaFileModel"),
                                     m_bootstrap.mediaFileModel());
    m_qmlContext->setContextProperty(QStringLiteral("practiceSession"),
                                     m_bootstrap.practiceSession());
    m_qmlContext->setContextProperty(QStringLiteral("practiceTracker"),
                                     m_bootstrap.practiceTracker());
    m_qmlContext->setContextProperty(QStringLiteral("reminderController"),
                                     m_bootstrap.reminderController());
    m_qmlContext->setContextProperty(QStringLiteral("practiceAssetController"),
                                     m_bootstrap.practiceAssetController());
    m_qmlContext->setContextProperty(QStringLiteral("audioConfigController"),
                                     m_bootstrap.audioConfigController());
    m_qmlContext->setContextProperty(QStringLiteral("linkGroupService"),
                                     m_bootstrap.linkGroupService());
    m_qmlContext->setContextProperty(QStringLiteral("libraryLinkModel"),
                                     m_bootstrap.libraryLinkModel());
    m_qmlContext->setContextProperty(QStringLiteral("startupController"), this);
}

// -----------------------------------------------------------------------------
// File / directory helpers
// -----------------------------------------------------------------------------

QString StartupController::browseStorageDirectory(const QString &startDir) {
    const QString start = startDir.trimmed().isEmpty() ? m_appSettings.defaultManagedStoragePath()
                                                       : startDir.trimmed();
    return StorageDirectoryPicker::browse(start, tr("Choose storage folder"));
}

QString StartupController::browseImportFolder(const QString &startDir) {
    const QString start = startDir.trimmed().isEmpty() ? QDir::homePath() : startDir.trimmed();
    return StorageDirectoryPicker::browse(start, tr("Import folder"));
}

bool StartupController::createStorageDirectory(const QString &path) {
    return m_appSettings.createStorageDirectory(path);
}
