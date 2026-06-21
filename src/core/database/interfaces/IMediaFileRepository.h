#ifndef IMEDIAFILEREPOSITORY_H
#define IMEDIAFILEREPOSITORY_H

#include <optional>

#include <QHash>
#include <QList>

#include "MediaFile.h"

class IMediaFileRepository {
  public:
    virtual ~IMediaFileRepository() = default;

    IMediaFileRepository(const IMediaFileRepository &) = delete;
    IMediaFileRepository &operator=(const IMediaFileRepository &) = delete;
    IMediaFileRepository(IMediaFileRepository &&) = delete;
    IMediaFileRepository &operator=(IMediaFileRepository &&) = delete;

    virtual std::optional<qlonglong> createMediaFile(const MediaFile &mediaFile) = 0;
    virtual std::optional<MediaFile> getMediaFile(qlonglong id) = 0;
    virtual std::optional<MediaFile> findByHash(const QString &fileHash) = 0;
    virtual std::optional<MediaFile> findByPath(const QString &filePath) = 0;
    virtual QList<MediaFile> getMediaFilesBySongId(qlonglong songId) = 0;
    /** @brief Returns every media file row in the database. */
    virtual QList<MediaFile> getAllMediaFiles() = 0;
    /// Returns the first media file id for each song id (same order as getMediaFilesBySongId).
    virtual QHash<qlonglong, qlonglong> firstMediaIdBySongIds(const QList<qlonglong> &songIds) = 0;

  protected:
    IMediaFileRepository() = default;
};

#endif // IMEDIAFILEREPOSITORY_H
