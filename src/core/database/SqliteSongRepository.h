#ifndef SQLITESONGREPOSITORY_H
#define SQLITESONGREPOSITORY_H

#include "interfaces/IDatabaseConnection.h"
#include "interfaces/ISongRepository.h"

class SqliteSongRepository : public ISongRepository {
  public:
    explicit SqliteSongRepository(IDatabaseConnection &connection);

    [[nodiscard]] std::optional<qlonglong> createSong(const Song &song) override;
    [[nodiscard]] std::optional<Song> getSong(qlonglong id) override;
    [[nodiscard]] QList<Song> getAllSongs() override;
    [[nodiscard]] bool updateSong(const Song &song) override;
    [[nodiscard]] bool deleteSong(qlonglong id) override;

  private:
    IDatabaseConnection &m_connection;
};

#endif // SQLITESONGREPOSITORY_H
