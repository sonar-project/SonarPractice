#ifndef TST_PRACTICEASSETREPOSITORYTEST_H
#define TST_PRACTICEASSETREPOSITORYTEST_H

#include "DatabaseTestFixture.h"

#include <QObject>

class TestPracticeAssetRepository : public QObject, protected DatabaseTestFixture {
    Q_OBJECT

  public:
    TestPracticeAssetRepository() : DatabaseTestFixture(QStringLiteral("PracticeAssetRepositoryTest")) {}

  private slots:
    void init();
    void cleanup();

    void testUpsertSongOnlyAsset();
    void testUpsertCompositeNaturalKey();
    void testGetById();
    void testLastPrimaryMediaIdForSong();
};

#endif // TST_PRACTICEASSETREPOSITORYTEST_H
