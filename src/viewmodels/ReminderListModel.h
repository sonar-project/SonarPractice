#ifndef REMINDERLISTMODEL_H
#define REMINDERLISTMODEL_H

#include "Reminder.h"

#include <QAbstractListModel>
#include <QDate>
#include <QLocale>
#include <QString>
#include <QtGlobal>
#include <QtQml/qqmlregistration.h>

class IReminderRepository;
class IReminderConditionRepository;
class ISongRepository;

/**
 * @brief Reminders for one song, practice asset, or calendar day.
 *
 * Filter mode is chosen in reload(): date filter, practice asset, or song id.
 */
class ReminderListModel : public QAbstractListModel {
    Q_GADGET
    QML_ELEMENT
    QML_UNCREATABLE("Instances are provided via reminderController.songReminders.")

  public:
    enum Roles : uint16_t {
        ReminderIdRole = Qt::UserRole + 1,
        SongIdRole,
        TitleRole,
        ReminderDateRole,
        ScheduleLabelRole,
        IsDailyRole,
        IsWeeklyRole,
        IsMonthlyRole,
        IsActiveRole,
        IntervalDaysRole,
        WeekdayRole,
        HasConditionRole,
        ConditionStartBarRole,
        ConditionEndBarRole,
        ConditionMinBpmRole,
        ConditionMinMinutesRole
    };
    Q_ENUM(Roles)

    explicit ReminderListModel(IReminderRepository &reminderRepo,
                               IReminderConditionRepository &conditionRepo,
                               ISongRepository *songRepo = nullptr, QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void setSongId(qlonglong songId);
    void setPracticeAssetId(qlonglong practiceAssetId);
    void setFilterDate(const QDate &date);
    void setFilterByDate(bool enabled);
    void reload();

    /** Human-readable recurrence label; once-off dates use the system locale short format. */
    [[nodiscard]] static QString buildScheduleLabel(const Reminder &reminder) {
        if (reminder.isDaily) {
            return ReminderListModel::tr("Daily");
        }
        if (reminder.isWeekly) {
            static const QStringList days = {
                ReminderListModel::tr("Sun"), ReminderListModel::tr("Mon"), ReminderListModel::tr("Tue"),
                ReminderListModel::tr("Wed"), ReminderListModel::tr("Thu"), ReminderListModel::tr("Fri"),
                ReminderListModel::tr("Sat")};
            if (reminder.weekday >= 0 && reminder.weekday < days.size()) {
                return ReminderListModel::tr("Weekly (%1)").arg(days.at(reminder.weekday));
            }
            return ReminderListModel::tr("Weekly");
        }
        if (reminder.isMonthly) {
            return ReminderListModel::tr("Monthly");
        }
        if (reminder.intervalDays > 0) {
            return ReminderListModel::tr("Every %1 days").arg(reminder.intervalDays);
        }
        if (reminder.reminderDate.isValid()) {
            return QLocale().toString(reminder.reminderDate, QLocale::ShortFormat);
        }
        return ReminderListModel::tr("Once");
    }

  private:
    struct ReminderRow {
        qlonglong id{};
        qlonglong songId{};
        QString title{};
        QDate reminderDate{};
        QString scheduleLabel{};
        bool isDaily{false};
        bool isWeekly{false};
        bool isMonthly{false};
        bool isActive{true};
        int intervalDays{};
        int weekday{-1};
        bool hasCondition{false};
        int conditionStartBar{};
        int conditionEndBar{};
        int conditionMinBpm{};
        int conditionMinMinutes{};
    };

    IReminderRepository &m_reminderRepo;
    IReminderConditionRepository &m_conditionRepo;
    ISongRepository *m_songRepo{};
    QList<ReminderRow> m_rows{};
    qlonglong m_songId{};
    qlonglong m_practiceAssetId{};
    QDate m_filterDate{};
    bool m_filterByDate{false};
};

#endif // REMINDERLISTMODEL_H
