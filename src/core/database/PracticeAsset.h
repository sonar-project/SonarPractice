#ifndef PRACTICEASSET_H
#define PRACTICEASSET_H

#include <QtGlobal>

/**
 * @brief Represents a composite practice asset that groups up to five
 *        media files of different kinds under one song.
 *
 * All media IDs are optional (0 = not set).
 */
struct PracticeAsset {
    qlonglong id{};
    qlonglong songId{};
    qlonglong guitarProId{}; ///< FK → media_files, kind = GuitarPro
    qlonglong audioId{};     ///< FK → media_files, kind = Audio
    qlonglong videoId{};     ///< FK → media_files, kind = Video
    qlonglong imageId{};     ///< FK → media_files, kind = Image
    qlonglong documentId{};  ///< FK → media_files, kind = Document

    [[nodiscard]] bool isNull() const { return id <= 0; }

    [[nodiscard]] bool hasAnyMedia() const {
        return guitarProId > 0 || audioId > 0 || videoId > 0 || imageId > 0 || documentId > 0;
    }

    /** First populated media slot in display-priority order. */
    [[nodiscard]] qlonglong primaryMediaId() const {
        if (guitarProId > 0)
            return guitarProId;
        if (audioId > 0)
            return audioId;
        if (videoId > 0)
            return videoId;
        if (imageId > 0)
            return imageId;
        if (documentId > 0)
            return documentId;
        return 0;
    }
};

#endif // PRACTICEASSET_H
