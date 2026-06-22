#ifndef TST_TUNINGREPOSITORYTEST_H
#define TST_TUNINGREPOSITORYTEST_H

#include "DatabaseTestFixture.h"

#include <QObject>

class TestTuningRepository : public QObject, protected DatabaseTestFixture {
    Q_OBJECT

  public:
    TestTuningRepository() : DatabaseTestFixture(QStringLiteral("TuningRepositoryTest")) {}

  private slots:
    void init();
    void cleanup();

    void testCreateTuning();
    void testGetTuning();
    void testUpdateTuning();
    void testDeleteTuning();
    void testGetTuningNotFound();
    void testCreateTuningDuplicateName();
    void testListAllTunings();
};

#endif // TST_TUNINGREPOSITORYTEST_H
