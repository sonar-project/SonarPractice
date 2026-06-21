#ifndef DESKTOPLAUNCHER_H
#define DESKTOPLAUNCHER_H

#include "ILauncher.h"

class DesktopLauncher : public ILauncher {
  public:
    bool open(const QString &target, MediaSourceType sourceType) override;
};

#endif // DESKTOPLAUNCHER_H
