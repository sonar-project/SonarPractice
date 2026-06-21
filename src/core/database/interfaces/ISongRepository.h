#ifndef ISONGREPOSITORY_H
#define ISONGREPOSITORY_H

#include <optional>

#include <QList>

#include "Song.h"

class ISongRepository {
  public:
    virtual ~ISongRepository() = default;

    ISongRepository(const ISongRepository &) = delete;
    ISongRepository &operator=(const ISongRepository &) = delete;
    ISongRepository(ISongRepository &&) = delete;
    ISongRepository &operator=(ISongRepository &&) = delete;

    virtual std::optional<qlonglong> createSong(const Song &song) = 0;
    virtual std::optional<Song> getSong(qlonglong id) = 0;
    virtual QList<Song> getAllSongs() = 0;
    virtual bool updateSong(const Song &song) = 0;
    virtual bool deleteSong(qlonglong id) = 0;

  protected:
    ISongRepository() = default;
};

#endif // ISONGREPOSITORY_H
