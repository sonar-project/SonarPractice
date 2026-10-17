#ifndef USER_H
#define USER_H

#include "interfaces/IDatabaseConnection.h"
#include <QtGlobal>

class QString;

struct User {
    qlonglong id{};
    QString name{};
    QString role{}; // e.g. "admin" or "student"

    [[nodiscard]] bool isAdmin() const {
        return role.compare(QLatin1String("admin"), Qt::CaseInsensitive) == 0;
    }
};

#endif // USER_H
