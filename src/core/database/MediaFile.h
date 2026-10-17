#ifndef MEDIAFILE_H
#define MEDIAFILE_H

#include <QString>
#include <QtGlobal>
#include <cstdint>

enum class MediaSourceType : std::uint8_t { Local, Url };

enum class MediaKind : std::uint8_t { GuitarPro, Audio, Video, Image, Document, Unknown };

struct MediaFile {
    qlonglong id{0};
    qlonglong songId{0};
    QString filePath;
    QString fileType;
    MediaKind mediaKind{MediaKind::Unknown};
    qlonglong fileSize{0};
    QString fileHash;
    MediaSourceType sourceType{MediaSourceType::Local};
    bool isManaged{false};
    bool canBePracticed{false};
    QString importRoot;
    QString sourceRelativePath;
    bool hasVideo{false};
    bool hasAudio{false};

    /**
     * \brief Equality is defined over all fields that can change independently
     *        of the primary key.
     *
     * id alone would suffice for identity checks, but reload() needs to detect
     * metadata changes (e.g. canBePracticed toggled, filePath updated) so that
     * emitChangedKinds() fires even when the set of ids is identical.
     */
    [[nodiscard]] bool operator==(const MediaFile &other) const noexcept {
        return id == other.id && songId == other.songId && filePath == other.filePath &&
               fileType == other.fileType && mediaKind == other.mediaKind &&
               fileSize == other.fileSize && fileHash == other.fileHash &&
               sourceType == other.sourceType && isManaged == other.isManaged &&
               canBePracticed == other.canBePracticed && importRoot == other.importRoot &&
               sourceRelativePath == other.sourceRelativePath && hasVideo == other.hasVideo &&
               hasAudio == other.hasAudio;
    }

    [[nodiscard]] bool operator!=(const MediaFile &other) const noexcept {
        return !(*this == other);
    }
};

#endif // MEDIAFILE_H
