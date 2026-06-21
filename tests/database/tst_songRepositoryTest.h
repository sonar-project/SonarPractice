#ifndef TST_SONGREPOSITORYTEST_H
#define TST_SONGREPOSITORYTEST_H

#include "DatabaseTestFixture.h"

#include <QObject>

class TestSongRepository : public QObject, protected DatabaseTestFixture {
    Q_OBJECT

  public:
    TestSongRepository() : DatabaseTestFixture(QStringLiteral("SongRepositoryTest")) {}

  private slots:
    void init();
    void cleanup();

    void testCreateSong();
    void testGetSong();
    void testUpdateSong();
    void testDeleteSong();
    void testGetAllSongsEmpty();
    void testGetAllSongs();
    void testGetSongNotFound();
    void testCreateSongWithoutConnection();
};

#endif // TST_SONGREPOSITORYTEST_H
