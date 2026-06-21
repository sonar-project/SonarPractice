#ifndef TST_APPLICATIONERRORLOGTEST_H
#define TST_APPLICATIONERRORLOGTEST_H

#include <QObject>

class TestApplicationErrorLog : public QObject {
    Q_OBJECT

  private slots:
    void testLogErrorWritesFileAndEmitsNotice();
    void testLogErrorCanSkipUserNotice();
};

#endif // TST_APPLICATIONERRORLOGTEST_H
