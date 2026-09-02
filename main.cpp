#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QIcon>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTimer>
#include <qcoreapplication.h>
#include <memory>

#ifdef SONARPRACTICE_HAS_WEBENGINE
#include <QtWebEngineQuick/qtwebenginequickglobal.h>
#include "SoundFontSchemeHandler.h"
#include <QWebEngineProfile>
#endif

#include "AppSettings.h"
#include "ApplicationBootstrap.h"
#include "StartupController.h"
#include "TranslationManager.h"
#include "AppInfo.h"

/**
 * @file main.cpp
 * @brief Entry point for the SonarPractice desktop application.
 */

namespace {

    Q_LOGGING_CATEGORY(lcStartup, "sonarp.startup")

    void logStartupCheckpoint(QElapsedTimer &timer, const char *label) {
        qCInfo(lcStartup) << label << timer.elapsed() << "ms";
    }

    /**
     * @brief Collects file and directory paths from command-line arguments.
     * @param arguments Raw command-line arguments (without the program name).
     * @return Absolute paths that exist on disk.
     */
    QStringList collectStartupPaths(const QStringList &arguments) {
        QStringList paths;
        paths.reserve(arguments.size());

        for (const QString &arg : arguments) {
            const QFileInfo info(arg);
            if (info.exists() && (info.isFile() || info.isDir())) {
                paths.append(info.absoluteFilePath());
            }
        }

        return paths;
    }

} // namespace

int main(int argc, char *argv[]) {
    QElapsedTimer startupTimer;
    startupTimer.start();

    QCoreApplication::setOrganizationName(QStringLiteral("sonarp"));
    QCoreApplication::setApplicationName(QStringLiteral("SonarPractice"));

#ifdef SONARPRACTICE_HAS_WEBENGINE
    // Must run before QCoreApplication / QApplication construction.
    SoundFontSchemeHandler::registerScheme();
    QtWebEngineQuick::initialize();
#endif

    QApplication app(argc, argv);

#ifdef SONARPRACTICE_HAS_WEBENGINE
    auto soundFontSchemeHandler = std::make_unique<SoundFontSchemeHandler>(&app);
    QWebEngineProfile::defaultProfile()->installUrlSchemeHandler(
        QByteArrayLiteral("sp-soundfont"), soundFontSchemeHandler.get());
#endif

    QApplication::setWindowIcon(
        QIcon(QStringLiteral(":/qt/qml/com/sonarp/sonarpractice/assets/svg/icon.svg")));

    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    AppSettings appSettings;
    logStartupCheckpoint(startupTimer, "main: AppSettings");

#ifdef SONARPRACTICE_HAS_WEBENGINE
    if (!appSettings.usesBuiltInSoundFont()) {
        const QString soundFontPath = appSettings.effectiveSoundFontPath();
        QElapsedTimer soundFontTimer;
        soundFontTimer.start();
        soundFontSchemeHandler->setFilePath(soundFontPath);
        logStartupCheckpoint(startupTimer, "main: SoundFont preload");
        qCInfo(lcStartup) << "SoundFont cache"
                          << soundFontSchemeHandler->cachedByteCount() << "bytes from"
                          << soundFontPath << "in" << soundFontTimer.elapsed() << "ms";
    }
    // Path changes require a restart to rebuild the cache; do not reload huge banks
    // on the UI thread while the app is running.
    QObject::connect(&appSettings, &AppSettings::settingsChanged, &app,
                     [&appSettings, &soundFontSchemeHandler]() {
                         if (appSettings.usesBuiltInSoundFont()) {
                             soundFontSchemeHandler->clear();
                             return;
                         }
                         const QString path = appSettings.effectiveSoundFontPath();
                         if (!soundFontSchemeHandler->matchesPath(path)) {
                             // Drop stale cache so the player cannot play the wrong bank.
                             soundFontSchemeHandler->clear();
                         }
                     });
#endif

    ApplicationBootstrap bootstrap(
        appSettings,
#ifdef SONARPRACTICE_HAS_WEBENGINE
        soundFontSchemeHandler.get()
#else
        nullptr
#endif
    );
    StartupController startupController(appSettings, bootstrap);

    const QStringList startupPaths = collectStartupPaths(QCoreApplication::arguments().mid(1));

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(QStringLiteral("appSettings"), &appSettings);

    TranslationManager translationManager(appSettings);
    translationManager.setEngine(&engine);
    translationManager.applySavedLanguage();

    startupController.setQmlContext(engine.rootContext());

    engine.rootContext()->setContextProperty(QStringLiteral("translationManager"),
                                             &translationManager);

    engine.rootContext()->setContextProperty(QStringLiteral("startupController"),
                                             &startupController);

    engine.rootContext()->setContextProperty(QStringLiteral("importService"),
                                             bootstrap.importService());

    engine.rootContext()->setContextProperty(QStringLiteral("songModel"), bootstrap.songModel());

    engine.rootContext()->setContextProperty(QStringLiteral("mediaFileModel"),
                                             bootstrap.mediaFileModel());

    engine.rootContext()->setContextProperty(QStringLiteral("practiceSession"),
                                             bootstrap.practiceSession());

    engine.rootContext()->setContextProperty(QStringLiteral("errorLog"), bootstrap.errorLog());

    engine.rootContext()->setContextProperty(QStringLiteral("practiceTracker"),
                                             bootstrap.practiceTracker());

    engine.rootContext()->setContextProperty(QStringLiteral("reminderController"),
                                             bootstrap.reminderController());

    engine.rootContext()->setContextProperty(QStringLiteral("practiceAssetController"),
                                             bootstrap.practiceAssetController());

    engine.rootContext()->setContextProperty(QStringLiteral("audioConfigController"),
                                             bootstrap.audioConfigController());

    engine.rootContext()->setContextProperty(QStringLiteral("linkGroupService"),
                                             bootstrap.linkGroupService());

    engine.rootContext()->setContextProperty(QStringLiteral("libraryLinkModel"),
                                             bootstrap.libraryLinkModel());

    // register for menu build and qt version.
    // Heap-allocate to ensure the QObject lifetime is always valid for QML bindings.
    auto *appInfo = new AppInfo(&engine);
    engine.rootContext()->setContextProperty(QStringLiteral("appInfo"), appInfo);

    startupController.registerContextProperties();

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("com.sonarp.sonarpractice"), QStringLiteral("Main"));
    logStartupCheckpoint(startupTimer, "main: loadFromModule");

    translationManager.applySavedLanguage();

    const QList<QObject *> rootObjects = engine.rootObjects();
    if (rootObjects.isEmpty()) {
        qCritical() << "Failed to load QML root object from com.sonarp.sonarpractice module";
        return -1;
    }

    if (auto *window = qobject_cast<QQuickWindow *>(rootObjects.first())) {
        window->setIcon(QIcon(QStringLiteral(":/icon")));
    }

    if (!startupController.needsConfiguration()) {
        QTimer::singleShot(0, &startupController, &StartupController::beginAsyncInitialization);
    }

    logStartupCheckpoint(startupTimer, "main: before exec");
    return QApplication::exec();
}
