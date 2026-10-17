#ifndef REMINDER_H
#define REMINDER_H

#include <QDate>
#include <QtGlobal>

class QString;

struct Reminder {
    qlonglong id{};
    qlonglong userId{1};
    qlonglong songId{};
    qlonglong practiceAssetId{};
    QString title{};
    QDate reminderDate{};
    int intervalDays{};
    int weekday{-1};
    bool isDaily{false};
    bool isMonthly{false};
    bool isWeekly{false};
    bool isActive{true};

    [[nodiscard]] bool isDueOn(const QDate &date) const;
};

#endif // REMINDER_H
