#ifndef TST_MEDIAFILEREPOSITORYTEST_H
#define TST_MEDIAFILEREPOSITORYTEST_H

#include "DatabaseTestFixture.h"

#include <QObject>

class TestMediaFileRepository : public QObject, protected DatabaseTestFixture {
    Q_OBJECT

  public:
    TestMediaFileRepository() : DatabaseTestFixture(QStringLiteral("MediaFileRepositoryTest")) {}

  private slots:
    void init();
    void cleanup();

    void testMediaFilesTable();
    void testMediaFilesCascadeDelete();
    void testGetMediaFilesBySongId();
    void testMediaFileHasVideoRoundtrip();
    void testGetMediaFilesBySongIdEmpty();
    void testFirstMediaIdBySongIds();
};

#endif // TST_MEDIAFILEREPOSITORYTEST_H
