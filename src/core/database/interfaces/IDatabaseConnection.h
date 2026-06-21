#ifndef IDATABASECONNECTION_H
#define IDATABASECONNECTION_H

#include <QString>

class IDatabaseConnection {
  public:
    virtual ~IDatabaseConnection() = default;

    struct Dependencies {
        IDatabaseConnection &connection;  // ← statt SqliteConnection &
    };

    IDatabaseConnection(const IDatabaseConnection &) = delete;
    IDatabaseConnection &operator=(const IDatabaseConnection &) = delete;
    IDatabaseConnection(IDatabaseConnection &&) = delete;
    IDatabaseConnection &operator=(IDatabaseConnection &&) = delete;

    virtual bool open(const QString &path) = 0;
    virtual void close() = 0;
    [[nodiscard]] virtual bool isOpen() const = 0;
    [[nodiscard]] virtual QString lastError() const = 0;
    [[nodiscard]] virtual QString connectionName() const = 0;

  protected:
    IDatabaseConnection() = default;
};

#endif // IDATABASECONNECTION_H
