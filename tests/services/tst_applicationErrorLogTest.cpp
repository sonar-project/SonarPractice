#include "tst_applicationErrorLogTest.h"

#include "ApplicationErrorLog.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

void TestApplicationErrorLog::testLogErrorWritesFileAndEmitsNotice() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString logPath = tempDir.filePath(QStringLiteral("errors.log"));
    ApplicationErrorLog errorLog(logPath);

    QSignalSpy noticeSpy(&errorLog, &ApplicationErrorLog::userNoticeRequested);

    errorLog.logError(QStringLiteral("PracticeSession.openAsset"),
                      QStringLiteral("Media file not found"));

    QCOMPARE(noticeSpy.count(), 1);
    QCOMPARE(noticeSpy.at(0).at(0).toString(), QStringLiteral("Media file not found"));

    QFile file(logPath);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString contents = QString::fromUtf8(file.readAll());
    QVERIFY(contents.contains(QStringLiteral("[PracticeSession.openAsset]")));
    QVERIFY(contents.contains(QStringLiteral("Media file not found")));
}

void TestApplicationErrorLog::testLogErrorCanSkipUserNotice() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    ApplicationErrorLog errorLog(tempDir.filePath(QStringLiteral("errors.log")));
    QSignalSpy noticeSpy(&errorLog, &ApplicationErrorLog::userNoticeRequested);

    errorLog.logError(QStringLiteral("Startup"), QStringLiteral("Database unavailable"), false);

    QCOMPARE(noticeSpy.count(), 0);
}

QTEST_MAIN(TestApplicationErrorLog)
