#ifndef ITUNINGREPOSITORY_H
#define ITUNINGREPOSITORY_H

#include <optional>

#include "Tuning.h"

class ITuningRepository {
  public:
    virtual ~ITuningRepository() = default;

    ITuningRepository(const ITuningRepository &) = delete;
    ITuningRepository &operator=(const ITuningRepository &) = delete;
    ITuningRepository(ITuningRepository &&) = delete;
    ITuningRepository &operator=(ITuningRepository &&) = delete;

    virtual std::optional<qlonglong> createTuning(const Tuning &tuning) = 0;
    virtual std::optional<Tuning> getTuning(qlonglong id) = 0;
    virtual std::optional<Tuning> findTuningByName(const QString &name) = 0;
    virtual bool updateTuning(const Tuning &tuning) = 0;
    virtual bool deleteTuning(qlonglong id) = 0;

  protected:
    ITuningRepository() = default;
};

#endif // ITUNINGREPOSITORY_H
