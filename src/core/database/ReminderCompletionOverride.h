#ifndef REMINDERCOMPLETIONOVERRIDE_H
#define REMINDERCOMPLETIONOVERRIDE_H

#include <QDate>
#include <QtGlobal>

struct ReminderCompletionOverride {
    qlonglong id{};
    qlonglong reminderId{};
    QDate completionDate{};
    bool accepted{true};
};

#endif // REMINDERCOMPLETIONOVERRIDE_H
