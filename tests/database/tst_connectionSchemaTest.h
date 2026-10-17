#ifndef TST_CONNECTIONSCHEMATEST_H
#define TST_CONNECTIONSCHEMATEST_H

#include "DatabaseTestFixture.h"

#include <QObject>

class TestConnectionSchema : public QObject, protected DatabaseTestFixture {
    Q_OBJECT

  public:
    TestConnectionSchema() : DatabaseTestFixture(QStringLiteral("ConnectionSchemaTest")) {}

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void testSqliteOpen();
    void testLastError();
    void testForeignKeyPragma();
    void testCreateAllTables();
};

#endif // TST_CONNECTIONSCHEMATEST_H
