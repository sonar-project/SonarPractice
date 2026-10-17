#include "tst_userRepositoryTest.h"

#include "SqliteConnection.h"
#include "SqliteUserRepository.h"
#include "User.h"

#include <QSqlDatabase>
#include <QTest>

void TestUserRepository::init() { setUp(); }

void TestUserRepository::cleanup() { tearDown(); }

void TestUserRepository::testCreateUser() {
    User admUser;
    admUser.name = QStringLiteral("TestAdmin");
    admUser.role = QStringLiteral("admin");

    const std::optional<qlonglong> adminId = m_userRepo.createUser(admUser);

    User noAdmUser;
    noAdmUser.name = QStringLiteral("StudentUser");
    noAdmUser.role = QStringLiteral("student");

    const std::optional<qlonglong> noAdminId = m_userRepo.createUser(noAdmUser);

    QVERIFY(adminId.has_value());
    QVERIFY(noAdminId.has_value());
    QVERIFY(*adminId > 0);
    QVERIFY(*noAdminId != *adminId);
}

void TestUserRepository::testGetUser() {
    User admUser;
    admUser.name = QStringLiteral("TestAdmin");
    admUser.role = QStringLiteral("admin");

    const std::optional<qlonglong> admUserId = m_userRepo.createUser(admUser);
    QVERIFY(admUserId.has_value());
    QVERIFY(*admUserId > 0);

    const std::optional<User> loadedUser = m_userRepo.getUser(*admUserId);
    QVERIFY(loadedUser.has_value());

    QCOMPARE(loadedUser->id, *admUserId);
    QCOMPARE(loadedUser->name, QStringLiteral("TestAdmin"));
    QVERIFY(loadedUser->isAdmin());
}

void TestUserRepository::testUpdateUser() {
    User user;
    user.name = QStringLiteral("BeforeUpdate");
    user.role = QStringLiteral("student");

    const std::optional<qlonglong> userId = m_userRepo.createUser(user);
    QVERIFY(userId.has_value());
    QVERIFY(*userId > 0);

    user.id = *userId;
    user.name = QStringLiteral("AfterUpdate");
    user.role = QStringLiteral("admin");

    QVERIFY(m_userRepo.updateUser(user));

    const std::optional<User> loadedUser = m_userRepo.getUser(*userId);
    QVERIFY(loadedUser.has_value());
    QCOMPARE(loadedUser->name, QStringLiteral("AfterUpdate"));
    QVERIFY(loadedUser->isAdmin());
}

void TestUserRepository::testGetUserNotFound() {
    const std::optional<User> loadedUser = m_userRepo.getUser(99999);
    QVERIFY(!loadedUser.has_value());
}

void TestUserRepository::testCreateUserDuplicateName() {
    User firstUser;
    firstUser.name = QStringLiteral("DuplicateName");
    firstUser.role = QStringLiteral("student");

    User secondUser;
    secondUser.name = QStringLiteral("DuplicateName");
    secondUser.role = QStringLiteral("admin");

    const std::optional<qlonglong> firstId = m_userRepo.createUser(firstUser);
    const std::optional<qlonglong> secondId = m_userRepo.createUser(secondUser);

    QVERIFY(firstId.has_value());
    QVERIFY(!secondId.has_value());
}

void TestUserRepository::testCreateUserEmptyName() {
    User user;
    user.name = QStringLiteral("");
    user.role = QStringLiteral("student");

    const std::optional<qlonglong> userId = m_userRepo.createUser(user);
    QVERIFY(!userId.has_value());
}

void TestUserRepository::testCreateUserWithoutConnection() {
    SqliteConnection closedConnector(QStringLiteral("ClosedUserDb"));
    SqliteUserRepository repo(closedConnector);

    User user;
    user.name = QStringLiteral("GhostUser");
    user.role = QStringLiteral("student");

    const std::optional<qlonglong> userId = repo.createUser(user);
    QVERIFY(!userId.has_value());
}

void TestUserRepository::testCreateUserAfterConnectionClosed() {
    User user;
    user.name = QStringLiteral("ClosedConnectionUser");
    user.role = QStringLiteral("student");

    m_connector.close();
    QSqlDatabase::removeDatabase(m_connector.connectionName());

    const std::optional<qlonglong> userId = m_userRepo.createUser(user);
    QVERIFY(!userId.has_value());
}

QTEST_MAIN(TestUserRepository)
