#ifndef DAYREMINDERMODEL_H
#define DAYREMINDERMODEL_H

#include "ReminderDayEntry.h"

#include <QAbstractListModel>
#include <QDate>
#include <QtGlobal>
#include <QtQml/qqmlregistration.h>

class QString;

/**
 * @brief Shows reminders for one day or all active reminders with song titles.
 */
class DayReminderModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int dailyCount READ dailyCount NOTIFY countsChanged)
    Q_PROPERTY(int periodicCount READ periodicCount NOTIFY countsChanged)

  public:
    enum class Roles : quint16 {
        ReminderIdRole = Qt::UserRole + 1,
        SongIdRole,
        ReminderTitleRole,
        SongTitleRole,
        ScheduleLabelRole,
        IsDailyRole,
        IsWeeklyRole,
        IsMonthlyRole,
        IntervalDaysRole,
        BaseBpmRole,
        PracticeAssetIdRole
    };
    Q_ENUM(Roles)

    explicit DayReminderModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index,
                                int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] int dailyCount() const;
    [[nodiscard]] int periodicCount() const;

    void setFilterDate(const QDate &date);
    /**
     * @brief Fills the model from pre-loaded reminder rows (cache or repository JOIN).
     * @param entries Reminder rows with song title and BPM.
     */
    void applyEntries(const QList<ReminderDayEntry> &entries);
    void clearRows();

  signals:
    void countsChanged();

  private:
    struct DayReminderRow {
        qlonglong reminderId{};
        qlonglong songId{};
        QString reminderTitle{};
        QString songTitle{};
        QString scheduleLabel{};
        bool isDaily{false};
        bool isWeekly{false};
        bool isMonthly{false};
        int intervalDays{};
        int baseBpm{};
        qlonglong practiceAssetId{};
    };

    [[nodiscard]] static DayReminderRow rowFromEntry(const ReminderDayEntry &entry);
    void rebuildCounts();

    QList<DayReminderRow> m_rows{};
    QDate m_filterDate{};
    int m_dailyCount{};
    int m_periodicCount{};
};

#endif // DAYREMINDERMODEL_H
