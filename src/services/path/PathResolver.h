#ifndef PATHRESOLVER_H
#define PATHRESOLVER_H

#include "IPathResolver.h"

class PathResolver : public IPathResolver {
  public:
    explicit PathResolver(QString managedStorageRoot);

    [[nodiscard]] QString resolve(const MediaFile &mediaFile) const override;

  private:
    QString m_managedStorageRoot{};
};

#endif // PATHRESOLVER_H
