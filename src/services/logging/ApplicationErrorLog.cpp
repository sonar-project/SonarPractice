#include "ApplicationErrorLog.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>

namespace {

    Q_LOGGING_CATEGORY(lcErrorLog, "sonarp.error")

    QMutex &writeMutex() {
        static QMutex mutex;
        return mutex;
    }

} // namespace

ApplicationErrorLog::ApplicationErrorLog(QObject *parent)
    : QObject(parent), m_logFilePath(defaultLogFilePath()) {}

ApplicationErrorLog::ApplicationErrorLog(const QString &logFilePath, QObject *parent)
    : QObject(parent), m_logFilePath(logFilePath) {}

QString ApplicationErrorLog::defaultLogFilePath() {
    const QString appDataPath =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(appDataPath);
    return QDir(appDataPath).filePath(QStringLiteral("errors.log"));
}

QString ApplicationErrorLog::logFilePath() const { return m_logFilePath; }

void ApplicationErrorLog::logError(const QString &context, const QString &message,
                                   const bool notifyUser) {
    const QString trimmedContext = context.trimmed();
    const QString trimmedMessage = message.trimmed();
    if (trimmedMessage.isEmpty()) {
        return;
    }

    const QString line =
        QStringLiteral("%1 [%2] %3")
            .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs),
                 trimmedContext.isEmpty() ? QStringLiteral("Application") : trimmedContext,
                 trimmedMessage);

    qCWarning(lcErrorLog) << line;
    appendLine(line);

    if (notifyUser) {
        emit userNoticeRequested(trimmedMessage);
    }
}

void ApplicationErrorLog::appendLine(const QString &line) {
    if (m_logFilePath.isEmpty()) {
        return;
    }

    QMutexLocker locker(&writeMutex());

    const QFileInfo fileInfo(m_logFilePath);
    QDir().mkpath(fileInfo.absolutePath());

    QFile file(m_logFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qCWarning(lcErrorLog) << "Failed to open error log:" << m_logFilePath;
        return;
    }

    QTextStream stream(&file);
    stream << line << '\n';
}
