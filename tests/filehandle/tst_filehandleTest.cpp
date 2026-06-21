#include "tst_filehandleTest.h"
#include "fnv1a.h"

#include <QTemporaryFile>
#include <QTextStream>

QTEST_MAIN(TestFileHandle)

void TestFileHandle::testEmptyFileHash() {
    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());

    QString expected = "0000000000000000";
    QString actual = FNV1a::calculate(tmpFile.fileName());

    QCOMPARE(actual, expected);
}

void TestFileHandle::testHashCalculation() {
    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());

    // input file
    QTextStream out(&tmpFile);
    out << "SonarPractice FNV1a Test";
    out.flush();

    QString hash1 = FNV1a::calculate(tmpFile.fileName());

    QVERIFY(!hash1.isEmpty());
    QVERIFY(hash1 != "0000000000000000");
}

void TestFileHandle::testNonExistentFile() {
    QString actual = FNV1a::calculate("nonExistsentFile.xyz");
    QVERIFY(actual.isEmpty());
}
