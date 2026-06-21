#ifndef TST_LIBMANTEST_H
#define TST_LIBMANTEST_H

#include <QTest>

class TestLibManager : public QObject {
    Q_OBJECT
  public:
  private slots:
    void testParseGPXFile();
    void testParseGP3File();
    void testParseGP4File();
    void testParseGP5File();
    void testParseGPFile();

  private:
};

#endif // TST_LIBMANTEST_H
