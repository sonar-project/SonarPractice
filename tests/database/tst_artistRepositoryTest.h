#ifndef TST_ARTISTREPOSITORYTEST_H
#define TST_ARTISTREPOSITORYTEST_H

#include "DatabaseTestFixture.h"

#include <QObject>

class TestArtistRepository : public QObject, protected DatabaseTestFixture {
    Q_OBJECT

  public:
    TestArtistRepository() : DatabaseTestFixture(QStringLiteral("ArtistRepositoryTest")) {}

  private slots:
    void init();
    void cleanup();

    void testCreateArtist();
    void testGetArtist();
    void testUpdateArtist();
    void testDeleteArtist();
    void testGetArtistNotFound();
    void testCreateArtistDuplicateName();
    void testCreateArtistEmptyName();
};

#endif // TST_ARTISTREPOSITORYTEST_H
