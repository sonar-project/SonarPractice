#ifndef MANAGEDFILESTORAGE_H
#define MANAGEDFILESTORAGE_H

#include "ImportTypes.h"

#include <QCoreApplication>
#include <QString>

struct ManagedFileResult {
    bool success{false};
    QString storedPath{};
    QString relativePath{};
    bool isManaged{false};
    bool duplicateContent{false};
    QString message{};
};

struct StorageParameters {
    const QString &fileHash{};
    const QString &extension{};
    const QString &sourcePath{};
    const QString &relativeSubPath{};
    const QString &managedRoot{};
};

class ManagedFileStorage {
    Q_DECLARE_TR_FUNCTIONS(ManagedFileStorage)

  public:
    [[nodiscard]] static ManagedFileResult storeFile(StorageStrategy strategy,
                                                     const StorageParameters &params);
};

#endif // MANAGEDFILESTORAGE_H