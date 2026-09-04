#ifndef IMPORTTYPES_H
#define IMPORTTYPES_H

#include "MediaFile.h"

#include <QList>
#include <QMetaType>
#include <QString>
#include <QtGlobal>

enum class StorageStrategy : std::uint8_t { Link, Copy, Move };

enum class ImportStatus : std::uint8_t { Imported, Skipped, Failed };

struct ImportResult {
    ImportStatus status{ImportStatus::Failed};
    QString sourcePath{};
    qlonglong songId{};
    qlonglong mediaFileId{};
    QString message{};
    QString importRoot{};
    QString sourceRelativePath{};
    MediaKind mediaKind{MediaKind::Unknown};
    QString songTitle{};
};

struct ImportFailureDetail {
    QString path{};
    QString message{};
};

struct ImportSummary {
    int importedCount{};
    int skippedCount{};
    int failedCount{};
    QList<ImportFailureDetail> failures{};
};

struct ImportFileEntry {
    QString absolutePath{};
    QString importRoot{};
    QString sourceRelativePath{};
};

Q_DECLARE_METATYPE(ImportResult)
Q_DECLARE_METATYPE(ImportSummary)
Q_DECLARE_METATYPE(ImportFailureDetail)

#endif // IMPORTTYPES_H
