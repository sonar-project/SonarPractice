#ifndef IIMPORTSERVICE_H
#define IIMPORTSERVICE_H

#include "ImportTypes.h"

#include <QString>

class IImportService {
  public:
    virtual ~IImportService() = default;

    [[nodiscard]] virtual bool isBusy() const = 0;

    virtual ImportResult importFile(const QString &absolutePath,
                                    StorageStrategy strategy = StorageStrategy::Link) = 0;

    virtual void importDirectory(const QString &directoryPath,
                                 StorageStrategy strategy = StorageStrategy::Link) = 0;

    virtual void cancelImport() = 0;
};

#endif // IIMPORTSERVICE_H
