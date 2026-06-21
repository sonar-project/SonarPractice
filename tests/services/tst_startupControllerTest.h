#ifndef TST_STARTUPCONTROLLERTEST_H
#define TST_STARTUPCONTROLLERTEST_H

#include <QObject>
#include <QTemporaryDir>

class ApplicationBootstrap;

class TestStartupController : public QObject {
    Q_OBJECT

  private slots:
    void init();
    void cleanup();

    void testNeedsConfigurationWhenDatabaseMissing();
    void testBeginAsyncInitializationSkipsWithoutDatabase();
    void testInitializeDatabaseCreatesSchemaAndLoadsCatalog();
    void testBeginAsyncInitializationWhenDatabaseExists();

  private:
    static bool waitForCatalogReady(ApplicationBootstrap &bootstrap, int timeoutMs = 5000);

    QString m_settingsPath;
    QString m_previousDataHome;
    QTemporaryDir m_dataHome;
};

#endif // TST_STARTUPCONTROLLERTEST_H
