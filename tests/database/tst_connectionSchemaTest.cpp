#include "tst_connectionSchemaTest.h"

#include "DatabaseSchema.h"
#include "SqliteConnection.h"

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTest>

void TestConnectionSchema::initTestCase() {
    QVERIFY(QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")));
}

void TestConnectionSchema::init() { setUp(); }

void TestConnectionSchema::cleanup() { tearDown(); }

void TestConnectionSchema::testSqliteOpen() {
    SqliteConnection connector(QStringLiteral("TestOpenAndClose"));

    QVERIFY2(connector.open(QStringLiteral(":memory:")), "Could not open in-memory DB!");
    QVERIFY(connector.isOpen());

    connector.close();
    QVERIFY(!connector.isOpen());
}

void TestConnectionSchema::testLastError() {
    SqliteConnection connector(QStringLiteral("testLastError"));

    QVERIFY(!connector.open(QStringLiteral("/invalid/path/to/db.sqlite")));

    const QString error = connector.lastError();
    QVERIFY(!error.isEmpty());
}

void TestConnectionSchema::testForeignKeyPragma() {
    QSqlDatabase db = QSqlDatabase::database(m_connector.connectionName());
    QSqlQuery query(db);

    QVERIFY(query.exec(QStringLiteral("PRAGMA foreign_keys;")));

    if (query.next()) {
        QCOMPARE(query.value(0).toInt(), 1);
    } else {
        QFAIL("PRAGMA query returned no result.");
    }
}

void TestConnectionSchema::testCreateAllTables() {
    DatabaseSchema schema(m_connector);

    QVERIFY(schema.createAllTables());
    QVERIFY(schema.createAllTables());

    QSqlDatabase db = QSqlDatabase::database(m_connector.connectionName());

    const QStringList expectedTables = {QStringLiteral("users"),
                                        QStringLiteral("artists"),
                                        QStringLiteral("tunings"),
                                        QStringLiteral("songs"),
                                        QStringLiteral("media_files"),
                                        QStringLiteral("file_relations"),
                                        QStringLiteral("link_groups"),
                                        QStringLiteral("sections"),
                                        QStringLiteral("practice_assets"),
                                        QStringLiteral("practice_journal"),
                                        QStringLiteral("practice_notices"),
                                        QStringLiteral("reminders"),
                                        QStringLiteral("reminder_conditions"),
                                        QStringLiteral("audio_config_presets")};

    for (const QString &table : expectedTables) {
        QVERIFY2(db.tables().contains(table), qPrintable(QStringLiteral("tbl %1 missing!").arg(table)));
    }
}

QTEST_MAIN(TestConnectionSchema)
