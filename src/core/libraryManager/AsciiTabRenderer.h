#ifndef ASCII_TAB_RENDERER_H
#define ASCII_TAB_RENDERER_H

#include <QString>
#include <QStringList>
#include <optional>
#include <vector>

namespace libgp_parser {
    struct Song;
    struct Track;
} // namespace libgp_parser

/**
 * @brief Builds a rough monospaced ASCII tablature from a parsed Guitar Pro song.
 *
 * Voice 0 only. Tied notes render as continuation dashes. Percussion tracks
 * return an explanatory message instead of tab lines.
 */
class AsciiTabRenderer {
  public:
    struct TrackPreview {
        QString name{};
        QString tuningDisplay{};
        bool percussion{false};
        QString tabText{};
    };

    struct SongPreview {
        QString title{};
        QString artist{};
        int bpm{};
        /** Number of master bars / measures in the score (0 if unknown). */
        int barCount{};
        std::vector<TrackPreview> tracks{};
    };

    [[nodiscard]] static std::optional<SongPreview> loadFromFile(const QString &filePath);
    [[nodiscard]] static QString renderTrack(const libgp_parser::Song &song, int trackIndex);
    [[nodiscard]] static TrackPreview previewForTrack(const libgp_parser::Song &song,
                                                      int trackIndex);

  private:
    [[nodiscard]] static QString stringLabel(const libgp_parser::Track &track, int stringNumber);
    [[nodiscard]] static QString renderNonPercussionTrack(const libgp_parser::Song &song,
                                                          const libgp_parser::Track &track);
};

#endif // ASCII_TAB_RENDERER_H
