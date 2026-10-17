#ifndef APPLICATIONERRORLOG_H
#define APPLICATIONERRORLOG_H

#include <QObject>
#include <QString>

/**
 * @brief Persists application errors to disk and notifies the UI when appropriate.
 */
class ApplicationErrorLog : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString logFilePath READ logFilePath CONSTANT)

  public:
    explicit ApplicationErrorLog(QObject *parent = nullptr);
    explicit ApplicationErrorLog(const QString &logFilePath, QObject *parent = nullptr);

    [[nodiscard]] static QString defaultLogFilePath();

    [[nodiscard]] QString logFilePath() const;

    void logError(const QString &context, const QString &message, bool notifyUser = true);

  signals:
    void userNoticeRequested(const QString &message);

  private:
    void appendLine(const QString &line);

    QString m_logFilePath;
};

#endif // APPLICATIONERRORLOG_H
