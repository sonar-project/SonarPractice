#ifndef SQLITETUNINGREPOSITORY_H
#define SQLITETUNINGREPOSITORY_H

#include "interfaces/IDatabaseConnection.h"
#include "interfaces/ITuningRepository.h"

class SqliteTuningRepository : public ITuningRepository {
  public:
    explicit SqliteTuningRepository(IDatabaseConnection &connection);

    [[nodiscard]] std::optional<qlonglong> createTuning(const Tuning &tuning) override;
    [[nodiscard]] std::optional<Tuning> findTuningByName(const QString &name) override;

  private:
    IDatabaseConnection &m_connection;
};

#endif // SQLITETUNINGREPOSITORY_H
