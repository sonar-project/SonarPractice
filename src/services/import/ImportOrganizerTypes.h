#ifndef IMPORTORGANIZERTYPES_H
#define IMPORTORGANIZERTYPES_H

#include "ImportTypes.h"

#include <QString>
#include <QtGlobal>
#include <cstdint>

enum class OrganizerEntryStatus : std::uint8_t {
    Ready = 0,
    DuplicateInBatch,
    AlreadyInDatabase,
    Defect
};

enum class OrganizerFilterMode : std::uint8_t { All = 0, Duplicates, Errors };

struct OrganizerPlannedEntry {
    QString absolutePath{};
    QString importRoot{};
    QString sourceRelativePath{};
    QString fileHash{};
};

[[nodiscard]] inline ImportFileEntry toImportFileEntry(const OrganizerPlannedEntry &entry) {
    return {.absolutePath = entry.absolutePath,
            .importRoot = entry.importRoot,
            .sourceRelativePath = entry.sourceRelativePath};
}

#endif // IMPORTORGANIZERTYPES_H
