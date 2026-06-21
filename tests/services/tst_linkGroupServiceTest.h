#ifndef TST_LINKGROUPSERVICETEST_H
#define TST_LINKGROUPSERVICETEST_H

#include <QObject>

#include "LinkGroupService.h"
#include "SqliteConnection.h"
#include "SqliteFileRelationRepository.h"
#include "SqliteLinkGroupRepository.h"
#include "SqliteMediaFileRepository.h"
#include "SqliteSongRepository.h"

class TestLinkGroupService : public QObject {
    Q_OBJECT

  private slots:
    void init();
    void cleanup();

    void testCreateGroupLinksSecondaryMedia();
    void testDissolveGroupRemovesRelations();
    void testDissolveGroupForSongReturnsMemberIds();
    void testAddSongsToExistingGroup();

  private:
    std::optional<qlonglong> createSong(const QString &title);
    std::optional<qlonglong> createMedia(qlonglong songId, const QString &path, MediaKind kind,
                                         const QString &importRoot = {},
                                         const QString &relativePath = {});

    SqliteConnection m_connector{QStringLiteral("LinkGroupServiceTestDb")};
    SqliteSongRepository m_songRepo{m_connector};
    SqliteMediaFileRepository m_mediaRepo{m_connector};
    SqliteLinkGroupRepository m_linkGroupRepo{m_connector};
    SqliteFileRelationRepository m_fileRelationRepo{m_connector};
    LinkGroupService m_service{LinkGroupService::Dependencies{
        m_linkGroupRepo,
        m_fileRelationRepo,
        m_mediaRepo,
        m_songRepo,
    }};
};

#endif // TST_LINKGROUPSERVICETEST_H
