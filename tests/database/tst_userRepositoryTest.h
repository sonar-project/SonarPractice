#ifndef TST_USERREPOSITORYTEST_H
#define TST_USERREPOSITORYTEST_H

#include "DatabaseTestFixture.h"

#include <QObject>

class TestUserRepository : public QObject, protected DatabaseTestFixture {
    Q_OBJECT

  public:
    TestUserRepository() : DatabaseTestFixture(QStringLiteral("UserRepositoryTest")) {}

  private slots:
    void init();
    void cleanup();

    void testCreateUser();
    void testGetUser();
    void testUpdateUser();
    void testGetUserNotFound();
    void testCreateUserDuplicateName();
    void testCreateUserEmptyName();
    void testCreateUserWithoutConnection();
    void testCreateUserAfterConnectionClosed();
};

#endif // TST_USERREPOSITORYTEST_H
