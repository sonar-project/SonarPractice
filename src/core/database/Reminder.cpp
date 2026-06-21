#include "Reminder.h"
#include "ReminderConstants.h"

bool Reminder::isDueOn(const QDate &date) const {
    if (!isActive || !date.isValid()) {
        return false;
    }

    if (isDaily) {
        return true;
    }

    if (isWeekly && weekday >= 0) {
        const int qtWeekday = date.dayOfWeek();
        const int schemaWeekday = qtWeekday == ReminderConstants::kQtSundayDayOfWeek
                                      ? ReminderConstants::kReminderSundayIndex
                                      : qtWeekday;
        return schemaWeekday == weekday;
    }

    if (isMonthly && reminderDate.isValid()) {
        return date.day() == reminderDate.day();
    }

    if (reminderDate.isValid()) {
        if (intervalDays > 0) {
            const qint64 daysSince = reminderDate.daysTo(date);
            return daysSince >= 0 && daysSince % intervalDays == 0;
        }
        return date == reminderDate;
    }

    return false;
}
