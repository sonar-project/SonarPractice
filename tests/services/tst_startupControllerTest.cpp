#include "tst_startupControllerTest.h"

#include "AppSettings.h"
#include "ApplicationBootstrap.h"
#include "SqliteConnection.h"
#include "StartupController.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryFile>
#include <QTest>

namespace {

    void closeMainDatabaseConnection() {
        if (QSqlDatabase::contains(QStringLiteral("SonarPracticeMain"))) {
            QSqlDatabase::database(QStringLiteral("SonarPracticeMain")).close();
            QSqlDatabase::removeDatabase(QStringLiteral("SonarPracticeMain"));
        }
    }

    bool tableExists(const QString &connectionName, const QString &tableName) {
        QSqlQuery query(QSqlDatabase::database(connectionName));
        query.prepare(
            QStringLiteral("SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = :name"));
        query.bindValue(QStringLiteral(":name"), tableName);
        if (!query.exec()) {
            return false;
        }
        return query.next();
    }

} // namespace

bool TestStartupController::waitForCatalogReady(ApplicationBootstrap &bootstrap, int timeoutMs) {
    if (bootstrap.isCatalogReady()) {
        return true;
    }

    QElapsedTimer timer;
    timer.start();
    while (!bootstrap.isCatalogReady() && timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    return bootstrap.isCatalogReady();
}

void TestStartupController::init() {
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    m_settingsPath = tempFile.fileName();
    tempFile.close();
    QFile::remove(m_settingsPath);

    m_previousDataHome = qEnvironmentVariable("XDG_DATA_HOME");
    QVERIFY(m_dataHome.isValid());
    qputenv("XDG_DATA_HOME", m_dataHome.path().toUtf8());
}

void TestStartupController::cleanup() {
    if (m_previousDataHome.isNull()) {
        qunsetenv("XDG_DATA_HOME");
    } else {
        qputenv("XDG_DATA_HOME", m_previousDataHome.toUtf8());
    }

    closeMainDatabaseConnection();

    const QString dbPath = AppSettings::databasePath();
    QFile::remove(dbPath);
    QFile::remove(m_settingsPath);
}

void TestStartupController::testNeedsConfigurationWhenDatabaseMissing() {
    QFile::remove(AppSettings::databasePath());

    AppSettings settings(m_settingsPath);
    ApplicationBootstrap bootstrap(settings);
    StartupController controller(settings, bootstrap);

    QVERIFY(controller.needsConfiguration());
    QVERIFY(!controller.shellReady());
    QVERIFY(!controller.catalogReady());
    QVERIFY(!controller.initializing());
}

void TestStartupController::testBeginAsyncInitializationSkipsWithoutDatabase() {
    QFile::remove(AppSettings::databasePath());

    AppSettings settings(m_settingsPath);
    ApplicationBootstrap bootstrap(settings);
    StartupController controller(settings, bootstrap);

    controller.beginAsyncInitialization();

    QVERIFY(!bootstrap.isShellReady());
    QVERIFY(!bootstrap.isCatalogReady());
    QVERIFY(!controller.shellReady());
}

void TestStartupController::testInitializeDatabaseCreatesSchemaAndLoadsCatalog() {
    QFile::remove(AppSettings::databasePath());

    AppSettings settings(m_settingsPath);
    ApplicationBootstrap bootstrap(settings);
    StartupController controller(settings, bootstrap);

    QVERIFY(controller.needsConfiguration());
    QVERIFY(controller.initializeDatabase());

    QVERIFY(controller.shellReady());
    QVERIFY(bootstrap.isShellReady());
    QVERIFY(!controller.catalogReady());

    QVERIFY(waitForCatalogReady(bootstrap));
    QVERIFY(controller.catalogReady());
    QVERIFY(!controller.initializing());
    QVERIFY(controller.initStatusText().isEmpty());

    const QString dbPath = AppSettings::databasePath();
    QVERIFY(QFileInfo::exists(dbPath));

    SqliteConnection connection(QStringLiteral("StartupControllerVerify"));
    QVERIFY(connection.open(dbPath));
    QVERIFY(tableExists(QStringLiteral("StartupControllerVerify"), QStringLiteral("songs")));
    QVERIFY(tableExists(QStringLiteral("StartupControllerVerify"), QStringLiteral("media_files")));
    connection.close();
    QSqlDatabase::removeDatabase(QStringLiteral("StartupControllerVerify"));
}

void TestStartupController::testBeginAsyncInitializationWhenDatabaseExists() {
    QFile::remove(AppSettings::databasePath());

    AppSettings settings(m_settingsPath);
    settings.ensureDefaults();

    {
        ApplicationBootstrap bootstrap(settings);
        StartupController controller(settings, bootstrap);
        QVERIFY(controller.initializeDatabase());
        QVERIFY(waitForCatalogReady(bootstrap));
    }

    closeMainDatabaseConnection();
    QVERIFY(QFileInfo::exists(AppSettings::databasePath()));

    ApplicationBootstrap bootstrap(settings);
    StartupController controller(settings, bootstrap);

    QVERIFY(!controller.needsConfiguration());
    controller.beginAsyncInitialization();

    QVERIFY(controller.shellReady());
    QVERIFY(waitForCatalogReady(bootstrap));
    QVERIFY(controller.catalogReady());
    QVERIFY(bootstrap.songModel() != nullptr);
    QVERIFY(bootstrap.libraryLinkModel() != nullptr);
}

QTEST_MAIN(TestStartupController)
