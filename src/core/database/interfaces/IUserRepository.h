#ifndef IUSERREPOSITORY_H
#define IUSERREPOSITORY_H

#include <optional>

#include "User.h"

class IUserRepository {
  public:
    virtual ~IUserRepository() = default;

    IUserRepository(const IUserRepository &) = delete;
    IUserRepository &operator=(const IUserRepository &) = delete;
    IUserRepository(IUserRepository &&) = delete;
    IUserRepository &operator=(IUserRepository &&) = delete;

    virtual std::optional<qlonglong> createUser(const User &user) = 0;
    virtual std::optional<User> getUser(qlonglong id) = 0;
    virtual bool updateUser(const User &user) = 0;

  protected:
    IUserRepository() = default;
};

#endif // IUSERREPOSITORY_H
