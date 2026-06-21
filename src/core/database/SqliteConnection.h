#ifndef SQLITECONNECTION_H
#define SQLITECONNECTION_H

#include "interfaces/IDatabaseConnection.h"

#include <QtSql/QSqlDatabase>

class QString;

class SqliteConnection : public IDatabaseConnection {
  public:
    explicit SqliteConnection(QString connectionName = QStringLiteral("SonarPractice_Default"));

    bool open(const QString &path) override;
    void close() override;

    [[nodiscard]] bool isOpen() const override;
    [[nodiscard]] QString lastError() const override;
    [[nodiscard]] QString connectionName() const override { return m_connectionName; }

  private:
    QString m_connectionName;
};

#endif // SQLITECONNECTION_H
