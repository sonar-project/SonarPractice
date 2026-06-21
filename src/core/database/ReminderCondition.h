#ifndef REMINDERCONDITION_H
#define REMINDERCONDITION_H

#include <QtGlobal>

struct ReminderCondition {
    qlonglong id{};
    qlonglong reminderId{};
    int startBar{};
    int endBar{};
    int minBpm{};
    int minMinutes{};
};

#endif // REMINDERCONDITION_H
