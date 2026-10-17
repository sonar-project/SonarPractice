#ifndef TST_FILEHANDLETEST_H
#define TST_FILEHANDLETEST_H

#include <QTest>

class TestFileHandle : public QObject {
    Q_OBJECT
  public:
  private slots:
    void testEmptyFileHash();
    void testHashCalculation();
    void testNonExistentFile();

  private:
};

#endif // TST_FILEHANDLETEST_H
