#ifndef IPATHRESOLVER_H
#define IPATHRESOLVER_H

#include "MediaFile.h"

class IPathResolver {
  public:
    virtual ~IPathResolver() = default;

    IPathResolver(const IPathResolver &) = delete;
    IPathResolver &operator=(const IPathResolver &) = delete;
    IPathResolver(IPathResolver &&) = delete;
    IPathResolver &operator=(IPathResolver &&) = delete;

    [[nodiscard]] virtual QString resolve(const MediaFile &mediaFile) const = 0;

  protected:
    IPathResolver() = default;
};

#endif // IPATHRESOLVER_H
