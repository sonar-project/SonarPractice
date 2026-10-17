#include "SqliteConnection.h"

#include "QSqlDatabase"
#include "QSqlError"
#include "QSqlQuery"
#include <utility>

namespace {

    bool applyConnectionPragmas(QSqlDatabase &db) {
        QSqlQuery query(db);
        if (!query.exec(QStringLiteral("PRAGMA foreign_keys = ON;"))) {
            return false;
        }

        query.exec(QStringLiteral("PRAGMA journal_mode = WAL;"));
        query.exec(QStringLiteral("PRAGMA synchronous = NORMAL;"));
        query.exec(QStringLiteral("PRAGMA cache_size = -64000;"));
        query.exec(QStringLiteral("PRAGMA temp_store = MEMORY;"));
        return true;
    }

} // namespace

SqliteConnection::SqliteConnection(QString connectionName)
    : m_connectionName(std::move(connectionName)) {}

bool SqliteConnection::open(const QString &path) {
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase existing = QSqlDatabase::database(m_connectionName, false);
        if (existing.isValid() && existing.isOpen() && existing.databaseName() == path) {
            return true;
        }

        if (existing.isValid() && !existing.isOpen()) {
            if (existing.databaseName() != path) {
                existing.setDatabaseName(path);
            }
            if (existing.open() && applyConnectionPragmas(existing)) {
                return true;
            }
        }

        if (existing.isValid()) {
            existing.close();
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(path);

    if (!db.open()) {
        return false;
    }

    if (!applyConnectionPragmas(db)) {
        db.close();
        QSqlDatabase::removeDatabase(m_connectionName);
        return false;
    }

    return true;
}

void SqliteConnection::close() {
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::database(m_connectionName).close();
    }
}

bool SqliteConnection::isOpen() const {
    if (!QSqlDatabase::contains(m_connectionName)) {
        return false;
    }

    return QSqlDatabase::database(m_connectionName, false).isOpen();
}

QString SqliteConnection::lastError() const {
    if (!QSqlDatabase::contains(m_connectionName)) {
        return {};
    }

    return QSqlDatabase::database(m_connectionName, false).lastError().text();
}
