#ifndef QMLTESTSETUP_H
#define QMLTESTSETUP_H

#include "PracticeConstants.h"

#include <QObject>
#include <QQmlEngine>

class MockImportService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage CONSTANT)
    Q_PROPERTY(int progressCurrent READ progressCurrent CONSTANT)
    Q_PROPERTY(int progressTotal READ progressTotal CONSTANT)
    Q_PROPERTY(QString progressFileName READ progressFileName CONSTANT)
    Q_PROPERTY(int lastImportedCount READ lastImportedCount CONSTANT)
    Q_PROPERTY(int lastSkippedCount READ lastSkippedCount CONSTANT)
    Q_PROPERTY(int lastFailedCount READ lastFailedCount CONSTANT)

  public:
    explicit MockImportService(QObject *parent = nullptr);

    [[nodiscard]] bool busy() const { return false; }
    [[nodiscard]] QString statusMessage() const { return QString(); }
    [[nodiscard]] int progressCurrent() const { return 0; }
    [[nodiscard]] int progressTotal() const { return 0; }
    [[nodiscard]] QString progressFileName() const { return QString(); }
    [[nodiscard]] int lastImportedCount() const { return 0; }
    [[nodiscard]] int lastSkippedCount() const { return 0; }
    [[nodiscard]] int lastFailedCount() const { return 0; }
    [[nodiscard]] const QStringList &lastImportedPaths() const { return m_lastPaths; }

  public slots:
    void importPaths(const QStringList &paths);
    void clearStatusMessage() {}
    void cancelImport() {}

  signals:
    void busyChanged();
    void importFinished();

  private:
    QStringList m_lastPaths;
};

class MockJournalModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)

  public:
    explicit MockJournalModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount() const { return m_rowCount; }
    void setRowCount(int count);

  public slots:
    void clear() { setRowCount(0); }

  signals:
    void rowCountChanged();

  private:
    int m_rowCount{0};
};

class MockPracticeTracker : public QObject {
    Q_OBJECT
    Q_PROPERTY(int startBar READ startBar WRITE setStartBar NOTIFY paramsChanged)
    Q_PROPERTY(int endBar READ endBar WRITE setEndBar NOTIFY paramsChanged)
    Q_PROPERTY(int targetBpm READ targetBpm WRITE setTargetBpm NOTIFY paramsChanged)
    Q_PROPERTY(bool timerRunning READ timerRunning NOTIFY timerStateChanged)
    Q_PROPERTY(QString elapsedDisplay READ elapsedDisplay NOTIFY timerTick)
    Q_PROPERTY(MockJournalModel *journalModel READ journalModel CONSTANT)
    Q_PROPERTY(QString journalMarkdown READ journalMarkdown WRITE setJournalMarkdown NOTIFY
                   journalMarkdownChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool statusIsError READ statusIsError NOTIFY statusMessageChanged)

  public:
    explicit MockPracticeTracker(QObject *parent = nullptr);

    int startBar() const { return m_startBar; }
    void setStartBar(int value);

    int endBar() const { return m_endBar; }
    void setEndBar(int value);

    int targetBpm() const { return m_targetBpm; }
    void setTargetBpm(int value);

    bool timerRunning() const { return m_timerRunning; }
    const QString &elapsedDisplay() const { return m_elapsedDisplay; }
    const QString &journalMarkdown() const { return m_journalMarkdown; }
    const QString &statusMessage() const { return m_statusMessage; }
    bool statusIsError() const { return m_statusIsError; }

    MockJournalModel *journalModel() { return &m_journalModel; }

    void setJournalMarkdown(const QString &markdown) {
        if (m_journalMarkdown == markdown) {
            return;
        }
        m_journalMarkdown = markdown;
        emit journalMarkdownChanged();
    }

  public slots:
    bool startTimer();
    bool stopAndSave();
    void cancelTimer();
    void reloadJournal() {}
    void reloadJournalNote() {}
    bool saveJournalNote() { return true; }
    void loadTrainingDefaults(int fallbackBpm);

  signals:
    void paramsChanged();
    void timerStateChanged();
    void timerTick();
    void journalSaved(qlonglong entryId);
    void journalMarkdownChanged();
    void statusMessageChanged();
    void saveFailed(const QString &message);

  private:
    int m_startBar{PracticeConstants::kDefaultStartBar};
    int m_endBar{PracticeConstants::kDefaultEndBar};
    int m_targetBpm{PracticeConstants::kDefaultTargetBpm};
    bool m_timerRunning{false};
    QString m_elapsedDisplay{QStringLiteral("00:00")};
    QString m_journalMarkdown;
    QString m_statusMessage;
    bool m_statusIsError{false};
    MockJournalModel m_journalModel;
};

class MockStartupController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool needsConfiguration READ needsConfiguration CONSTANT)
    Q_PROPERTY(bool isReady READ isReady NOTIFY readyChanged)

  public:
    explicit MockStartupController(QObject *parent = nullptr);

    [[nodiscard]] bool needsConfiguration() const { return false; }
    [[nodiscard]] bool isReady() const { return m_ready; }

  public slots:
    bool initializeDatabase();
    void registerContextProperties() {}

  signals:
    void readyChanged();

  private:
    bool m_ready{true};
};

class QmlTestEnvironment : public QObject {
    Q_OBJECT

  public slots:
    void qmlEngineAvailable(QQmlEngine *engine);
};

class QmlTestSetup : public QObject {
    Q_OBJECT

  public:
    static QmlTestSetup *instance();

    void engineAvailable(QQmlEngine *engine);

  private:
    explicit QmlTestSetup(QObject *parent = nullptr);

    MockImportService m_importService;
    MockPracticeTracker m_practiceTracker;
    MockStartupController m_startupController;
};

#endif // QMLTESTSETUP_H
