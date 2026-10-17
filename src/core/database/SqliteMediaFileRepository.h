#ifndef SQLITEMEDIAFILEREPOSITORY_H
#define SQLITEMEDIAFILEREPOSITORY_H

#include "interfaces/IDatabaseConnection.h"
#include "interfaces/IMediaFileRepository.h"

class SqliteMediaFileRepository : public IMediaFileRepository {
  public:
    explicit SqliteMediaFileRepository(IDatabaseConnection &connection);

    [[nodiscard]] std::optional<qlonglong> createMediaFile(const MediaFile &mediaFile) override;
    [[nodiscard]] std::optional<MediaFile> getMediaFile(qlonglong id) override;
    [[nodiscard]] std::optional<MediaFile> findByHash(const QString &fileHash) override;
    [[nodiscard]] std::optional<MediaFile> findByPath(const QString &filePath) override;
    [[nodiscard]] QList<MediaFile> getMediaFilesBySongId(qlonglong songId) override;
    [[nodiscard]] QList<MediaFile> getAllMediaFiles() override;
    [[nodiscard]] QHash<qlonglong, qlonglong> firstMediaIdBySongIds(const QList<qlonglong> &songIds) override;

  private:
    static MediaFile mapRow(class QSqlQuery &query);
    IDatabaseConnection &m_connection;
};

#endif // SQLITEMEDIAFILEREPOSITORY_H
