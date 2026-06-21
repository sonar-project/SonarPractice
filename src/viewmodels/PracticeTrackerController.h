#ifndef PRACTICETRACKERCONTROLLER_H
#define PRACTICETRACKERCONTROLLER_H

#include <QDate>
#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <qtmetamacros.h>

#include "JournalTableModel.h"
#include "PracticeConstants.h"
#include "PracticeTrackerService.h"

class ApplicationErrorLog;
class IPracticeJournalRepository;
class IPracticeNoticeRepository;
class IReminderRepository;
class IReminderConditionRepository;

/**
 * @brief Controls the exercise timer, journal save, and training defaults for QML.
 */
class PracticeTrackerController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Instances are provided via the 'practiceTracker' context property.")

    Q_PROPERTY(qlonglong songId READ songId WRITE setSongId NOTIFY songIdChanged)
    Q_PROPERTY(qlonglong assetId READ assetId WRITE setAssetId NOTIFY assetIdChanged)
    Q_PROPERTY(
        QDate selectedDate READ selectedDate WRITE setSelectedDate NOTIFY selectedDateChanged)
    Q_PROPERTY(int startBar READ startBar WRITE setStartBar NOTIFY paramsChanged)
    Q_PROPERTY(int endBar READ endBar WRITE setEndBar NOTIFY paramsChanged)
    Q_PROPERTY(int targetBpm READ targetBpm WRITE setTargetBpm NOTIFY paramsChanged)
    Q_PROPERTY(int totalReps READ totalReps WRITE setTotalReps NOTIFY paramsChanged)
    Q_PROPERTY(int successfulReps READ successfulReps WRITE setSuccessfulReps NOTIFY paramsChanged)
    Q_PROPERTY(bool timerRunning READ timerRunning NOTIFY timerStateChanged)
    Q_PROPERTY(QString elapsedDisplay READ elapsedDisplay NOTIFY timerTick)
    Q_PROPERTY(JournalTableModel *journalModel READ journalModel CONSTANT)
    Q_PROPERTY(QString journalMarkdown READ journalMarkdown WRITE setJournalMarkdown NOTIFY
                   journalMarkdownChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool statusIsError READ statusIsError NOTIFY statusMessageChanged)

  public:
    explicit PracticeTrackerController(IPracticeJournalRepository &journalRepo,
                                       IPracticeNoticeRepository &noticeRepo,
                                       IReminderRepository &reminderRepo,
                                       IReminderConditionRepository &conditionRepo,
                                       ApplicationErrorLog &errorLog,
                                       QObject *parent = nullptr);

    qlonglong songId() const;
    void setSongId(qlonglong songId);

    qlonglong assetId() const;
    void setAssetId(qlonglong assetId);

    QDate selectedDate() const;
    void setSelectedDate(const QDate &date);

    int startBar() const;
    void setStartBar(int value);

    int endBar() const;
    void setEndBar(int value);

    int targetBpm() const;
    void setTargetBpm(int value);

    int totalReps() const;
    void setTotalReps(int value);

    int successfulReps() const;
    void setSuccessfulReps(int value);

    bool timerRunning() const;
    const QString &elapsedDisplay() const;

    JournalTableModel *journalModel();

    const QString &journalMarkdown() const;
    void setJournalMarkdown(const QString &markdown);

    const QString &statusMessage() const;
    bool statusIsError() const;

  public slots:
    bool startTimer();
    bool stopAndSave();
    void stopAndSaveWithAssetId(qlonglong assetId);
    void cancelTimer();
    void reloadJournal();
    void reloadJournalNote();
    bool saveJournalNote();
    void loadTrainingDefaults(int fallbackBpm = 0);

  signals:
    void songIdChanged();
    void assetIdChanged();
    void selectedDateChanged();
    void paramsChanged();
    void timerStateChanged();
    void timerTick();
    void journalSaved(qlonglong entryId);
    void journalMarkdownChanged();
    void statusMessageChanged();
    void saveFailed(const QString &message);

  private:
    void setStatusMessage(const QString &message, bool isError);
    void reportError(const QString &context, const QString &message);
    void updateElapsedDisplay();
    PracticeTrackerParams buildParams() const;
    std::optional<class ReminderCondition> conditionForSong(qlonglong songId) const;

    PracticeTrackerService m_service;
    JournalTableModel m_journalModel;
    IPracticeJournalRepository &m_journalRepo;
    IPracticeNoticeRepository &m_noticeRepo;
    IReminderRepository &m_reminderRepo;
    IReminderConditionRepository &m_conditionRepo;
    ApplicationErrorLog &m_errorLog;
    qlonglong m_songId{};
    qlonglong m_assetId{};
    QDate m_selectedDate{};
    QString m_journalMarkdown{};
    int m_startBar{PracticeConstants::kDefaultStartBar};
    int m_endBar{PracticeConstants::kDefaultEndBar};
    int m_targetBpm{PracticeConstants::kDefaultTargetBpm};
    int m_totalReps{};
    int m_successfulReps{};
    QString m_elapsedDisplay{QStringLiteral("00:00")};
    QString m_statusMessage;
    bool m_statusIsError{false};
};

#endif // PRACTICETRACKERCONTROLLER_H
