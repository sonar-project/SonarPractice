#ifndef IARTISTREPOSITORY_H
#define IARTISTREPOSITORY_H

#include <optional>

#include "Artist.h"

#include <QList>

class IArtistRepository {
  public:
    virtual ~IArtistRepository() = default;

    IArtistRepository(const IArtistRepository &) = delete;
    IArtistRepository &operator=(const IArtistRepository &) = delete;
    IArtistRepository(IArtistRepository &&) = delete;
    IArtistRepository &operator=(IArtistRepository &&) = delete;

    virtual std::optional<qlonglong> createArtist(const Artist &artist) = 0;
    virtual std::optional<Artist> getArtist(qlonglong id) = 0;
    /** @brief Returns every artist row in the database. */
    virtual QList<Artist> getAllArtists() = 0;
    virtual std::optional<Artist> findArtistByName(const QString &name) = 0;
    virtual bool updateArtist(const Artist &artist) = 0;
    virtual bool deleteArtist(qlonglong id) = 0;

  protected:
    IArtistRepository() = default;
};

#endif // IARTISTREPOSITORY_H
