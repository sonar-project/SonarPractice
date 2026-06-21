#ifndef REMINDERCONTROLLER_H
#define REMINDERCONTROLLER_H

#include <QDate>
#include <QHash>
#include <QObject>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include "DayReminderModel.h"
#include "PracticeAssetController.h"
#include "ReminderDayEntry.h"
#include "ReminderListModel.h"
#include "interfaces/IReminderConditionRepository.h"
#include "interfaces/IReminderRepository.h"
#include "interfaces/ISongRepository.h"

class IReminderRepository;
class IReminderConditionRepository;
class ISongRepository;

/**
 * @brief QML facade for reminder CRUD, day/song lists, and schedule mapping.
 *
 * scheduleType strings: once, daily, weekly, monthly, interval.
 * weekday is 0=Sun … 6=Sat; intervalDays applies to interval only.
 */
class ReminderController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Instances are provided via the 'reminderController' context property.")

    Q_PROPERTY(qlonglong songId READ songId WRITE setSongId NOTIFY songIdChanged)
    Q_PROPERTY(qlonglong practiceAssetId READ practiceAssetId WRITE setPracticeAssetId NOTIFY
                   practiceAssetIdChanged)
    Q_PROPERTY(QDate filterDate READ filterDate WRITE setFilterDate NOTIFY filterDateChanged)
    Q_PROPERTY(ReminderListModel *songReminders READ songReminders CONSTANT)
    Q_PROPERTY(DayReminderModel *dayReminders READ dayReminders CONSTANT)
    Q_PROPERTY(int dayReminderCount READ dayReminderCount NOTIFY remindersChanged)
    Q_PROPERTY(int dailyReminderCount READ dailyReminderCount NOTIFY remindersChanged)
    Q_PROPERTY(int periodicReminderCount READ periodicReminderCount NOTIFY remindersChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    /**
     * @brief When true, shows all active reminders instead of one calendar day.
     */
    Q_PROPERTY(bool showAllReminders READ showAllReminders WRITE setShowAllReminders NOTIFY
                   showAllRemindersChanged)

  public:
    explicit ReminderController(IReminderRepository &reminderRepo,
                                IReminderConditionRepository &conditionRepo,
                                ISongRepository &songRepo, PracticeAssetController &assetController,
                                QObject *parent = nullptr);

    qlonglong songId() const;
    void setSongId(qlonglong songId);

    qlonglong practiceAssetId() const;
    void setPracticeAssetId(qlonglong practiceAssetId);

    QDate filterDate() const;
    void setFilterDate(const QDate &date);

    ReminderListModel *songReminders();
    DayReminderModel *dayReminders();
    int dayReminderCount() const;
    int dailyReminderCount() const;
    int periodicReminderCount() const;
    const QString &statusMessage() const;

    bool showAllReminders() const;
    void setShowAllReminders(bool showAll);

  public slots:
    void reloadSongReminders();
    void reloadDayReminders();
    /**
     * @brief Clears the per-day reminder cache.
     *
     * Call after create, update, or delete of a reminder.
     */
    void invalidateDayCache();
    /** Requires songId and practiceAssetId; optional bar/BPM conditions saved with the reminder. */
    bool createReminder(const QString &title, const QDate &date, const QString &scheduleType,
                        bool isActive, int startBar = 0, int endBar = 0, int minBpm = 0,
                        int minMinutes = 0, int intervalDays = 0, int weekday = -1);
    bool updateReminder(qlonglong reminderId, const QString &title, const QDate &date,
                        const QString &scheduleType, bool isActive, int intervalDays = 0,
                        int weekday = -1);
    bool deleteReminder(qlonglong reminderId);
    bool saveCondition(qlonglong reminderId, int startBar, int endBar, int minBpm, int minMinutes);
    /** Returns reminder fields and first condition for the edit form in QML. */
    Q_INVOKABLE QVariantMap reminderEditPayload(qlonglong reminderId) const;

    /** Keys: assetId, songId, guitarProId, audioId, videoId, imageId (0 = unset). */
    Q_INVOKABLE QVariantMap practiceAssetPayload(qlonglong assetId) const;

  signals:
    void songIdChanged();
    void practiceAssetIdChanged();
    void filterDateChanged();
    void statusMessageChanged();
    void showAllRemindersChanged();
    void remindersChanged();

  private:
    void setStatusMessage(const QString &message);
    QList<ReminderDayEntry> loadDayEntries(const QDate &date);
    static void applyScheduleType(const QString &scheduleType, const QDate &date,
                                  class Reminder &reminder, int intervalDays = 0,
                                  int weekday = -1);
    static QString scheduleTypeFromReminder(const Reminder &reminder);

    /** Cached per-day query results; cleared by invalidateDayCache(). */
    QHash<QDate, QList<ReminderDayEntry>> m_dayCache;
    QList<ReminderDayEntry> m_allActiveCache;
    bool m_allActiveCacheValid{false};
    PracticeAssetController &m_assetController;

    IReminderRepository &m_reminderRepo;
    IReminderConditionRepository &m_conditionRepo;
    ReminderListModel m_songReminders;
    DayReminderModel m_dayReminders;
    qlonglong m_songId{};
    qlonglong m_practiceAssetId{};
    QDate m_filterDate;
    QString m_statusMessage;
    bool m_showAllReminders{false};
};

#endif // REMINDERCONTROLLER_H
