#ifndef SQLITEUSERREPOSITORY_H
#define SQLITEUSERREPOSITORY_H

#include "interfaces/IDatabaseConnection.h"
#include "interfaces/IUserRepository.h"

class SqliteUserRepository : public IUserRepository {
  public:
    explicit SqliteUserRepository(IDatabaseConnection &connection);

    [[nodiscard]] std::optional<qlonglong> createUser(const User &user) override;
    [[nodiscard]] std::optional<User> getUser(qlonglong id) override;
    [[nodiscard]] bool updateUser(const User &user) override;

  private:
    IDatabaseConnection &m_connection;
};

#endif // SQLITEUSERREPOSITORY_H
