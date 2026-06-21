#ifndef ILAUNCHER_H
#define ILAUNCHER_H

#include "MediaFile.h"

class ILauncher {
  public:
    virtual ~ILauncher() = default;

    virtual bool open(const QString &target, MediaSourceType sourceType) = 0;
};

#endif // ILAUNCHER_H
