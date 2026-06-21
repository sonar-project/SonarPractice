#include "qmltestsetup.h"

#include <AppSettings.h>
#include <QQmlContext>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QQmlComponent>
#include <QTemporaryFile>
#include <QtQml>

#ifndef SONAR_QML_SOURCE_DIR
#define SONAR_QML_SOURCE_DIR ""
#endif

namespace {

    QString isolatedSettingsPath() {
        static QString path;
        if (path.isEmpty()) {
            QCoreApplication::setOrganizationName(QStringLiteral("sonarp"));
            QCoreApplication::setApplicationName(QStringLiteral("SonarPracticeQmlTests"));

            QTemporaryFile tempFile;
            if (!tempFile.open()) {
                qFatal("Could not create temporary settings file for QML tests");
            }
            path = tempFile.fileName();
            tempFile.close();
            QFile::remove(path);
        }
        return path;
    }

} // namespace

MockImportService::MockImportService(QObject *parent) : QObject(parent) {}

void MockImportService::importPaths(const QStringList &paths) {
    m_lastPaths = paths;
    emit importFinished();
}

MockStartupController::MockStartupController(QObject *parent) : QObject(parent) {}

bool MockStartupController::initializeDatabase() {
    m_ready = true;
    emit readyChanged();
    return true;
}

MockJournalModel::MockJournalModel(QObject *parent) : QObject(parent) {}

void MockJournalModel::setRowCount(int count) {
    if (m_rowCount == count) {
        return;
    }
    m_rowCount = count;
    emit rowCountChanged();
}

MockPracticeTracker::MockPracticeTracker(QObject *parent) : QObject(parent), m_journalModel(this) {}

void MockPracticeTracker::setStartBar(int value) {
    if (m_startBar == value) {
        return;
    }
    m_startBar = value;
    emit paramsChanged();
}

void MockPracticeTracker::setEndBar(int value) {
    if (m_endBar == value) {
        return;
    }
    m_endBar = value;
    emit paramsChanged();
}

void MockPracticeTracker::setTargetBpm(int value) {
    if (m_targetBpm == value) {
        return;
    }
    m_targetBpm = value;
    emit paramsChanged();
}

bool MockPracticeTracker::startTimer() {
    if (m_timerRunning) {
        return false;
    }
    m_timerRunning = true;
    m_elapsedDisplay = QStringLiteral("00:01");
    emit timerStateChanged();
    emit timerTick();
    return true;
}

bool MockPracticeTracker::stopAndSave() {
    if (!m_timerRunning || m_endBar < m_startBar) {
        emit saveFailed(QStringLiteral("Invalid range"));
        return false;
    }
    m_timerRunning = false;
    m_elapsedDisplay = QStringLiteral("00:00");
    m_journalModel.setRowCount(1);
    emit timerStateChanged();
    emit timerTick();
    emit journalSaved(1);
    return true;
}

void MockPracticeTracker::cancelTimer() {
    m_timerRunning = false;
    m_elapsedDisplay = QStringLiteral("00:00");
    emit timerStateChanged();
    emit timerTick();
}

void MockPracticeTracker::loadTrainingDefaults(int fallbackBpm) {
    if (fallbackBpm > 0) {
        m_targetBpm = fallbackBpm;
        emit paramsChanged();
    }
}

void QmlTestEnvironment::qmlEngineAvailable(QQmlEngine *engine) {
    QmlTestSetup::instance()->engineAvailable(engine);
}

QmlTestSetup *QmlTestSetup::instance() {
    static QmlTestSetup setup;
    return &setup;
}

QmlTestSetup::QmlTestSetup(QObject *parent) : QObject(parent) {}

void QmlTestSetup::engineAvailable(QQmlEngine *engine) {
    QCoreApplication::setOrganizationName(QStringLiteral("sonarp"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("sonarp.com"));
    QCoreApplication::setApplicationName(QStringLiteral("SonarPracticeQmlTests"));

    const QString sourceDir = QString::fromUtf8(SONAR_QML_SOURCE_DIR);
    if (!sourceDir.isEmpty()) {
        const QUrl themeUrl =
            QUrl::fromLocalFile(QDir(sourceDir).filePath(QStringLiteral("src/ui/Theme.qml")));
        qmlRegisterSingletonType(themeUrl, "com.sonarp.sonarpractice", 1, 0, "Theme");

        QQmlComponent themeComponent(engine, themeUrl);
        if (themeComponent.isReady()) {
            if (QObject *theme = themeComponent.create()) {
                theme->setParent(engine);
                engine->rootContext()->setContextProperty(QStringLiteral("Theme"), theme);
            }
        } else {
            qWarning() << "Theme component load failed:" << themeComponent.errors();
        }
    }

    auto *appSettings = new AppSettings(isolatedSettingsPath(), engine);
    appSettings->ensureDefaults();
    appSettings->setStorageStrategy(QStringLiteral("link"));
    appSettings->saveConfiguration();
    appSettings->reload();
    appSettings->setParent(engine);

    engine->rootContext()->setContextProperty(QStringLiteral("importService"),
                                              &instance()->m_importService);
    engine->rootContext()->setContextProperty(QStringLiteral("practiceTracker"),
                                              &instance()->m_practiceTracker);
    engine->rootContext()->setContextProperty(QStringLiteral("practiceSession"), QVariant());
    engine->rootContext()->setContextProperty(QStringLiteral("appSettings"), appSettings);
    engine->rootContext()->setContextProperty(QStringLiteral("startupController"),
                                              &instance()->m_startupController);
    engine->rootContext()->setContextProperty(QStringLiteral("songModel"), QVariant());
    engine->rootContext()->setContextProperty(QStringLiteral("mediaFileModel"), QVariant());
}
