#ifndef REPOSITORYUTILS_H
#define REPOSITORYUTILS_H

#include "interfaces/IDatabaseConnection.h"

#include <QSqlDatabase>

namespace RepositoryUtils {

    inline QSqlDatabase database(IDatabaseConnection &connection) {
        return QSqlDatabase::database(connection.connectionName());
    }

    inline bool ensureOpen(IDatabaseConnection &connection) {
        if (connection.isOpen()) {
            return true;
        }

        if (!QSqlDatabase::contains(connection.connectionName())) {
            return false;
        }

        QSqlDatabase db = QSqlDatabase::database(connection.connectionName(), false);
        return db.isValid() && db.open();
    }

} // namespace RepositoryUtils

#endif // REPOSITORYUTILS_H
