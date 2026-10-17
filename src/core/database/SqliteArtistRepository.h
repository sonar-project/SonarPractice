#ifndef SQLITEARTISTREPOSITORY_H
#define SQLITEARTISTREPOSITORY_H

#include "interfaces/IArtistRepository.h"
#include "interfaces/IDatabaseConnection.h"

class SqliteArtistRepository : public IArtistRepository {
  public:
    explicit SqliteArtistRepository(IDatabaseConnection &connection);

    std::optional<qlonglong> createArtist(const Artist &artist) override;
    std::optional<Artist> getArtist(qlonglong id) override;
    QList<Artist> getAllArtists() override;
    std::optional<Artist> findArtistByName(const QString &name) override;
    bool updateArtist(const Artist &artist) override;
    bool deleteArtist(qlonglong id) override;

  private:
    IDatabaseConnection &m_connection;
};

#endif // SQLITEARTISTREPOSITORY_H
