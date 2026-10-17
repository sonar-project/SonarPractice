#ifndef LIBRARY_MANAGER_H
#define LIBRARY_MANAGER_H

#include <QList>
#include <QString>
#include <QStringList>
#include <optional>

struct TrackMetadata {
    int number{};
    QString name{};
    QString instrumentName{};
    QStringList tuningNotes{}; // Explicitly as a list, e.g. ("E4", "B3", "G3"...)
    QString tuningDisplay{};   // Nicely formatted, e.g., "E A D G B E"
    bool isGuitar{false};
    bool isBass{false};
};

struct SongMetadata {
    QString title{};
    QString artist{};
    QString album{};
    int bpm{};
    QList<TrackMetadata> tracks{};
    QString defaultUiTuning{};
    QString defaultUiTrackName{};
};

/**
 * @brief Parses Guitar Pro files and extracts song/track metadata for imports.
 */
class LibraryManager {
  public:
    LibraryManager() = default;
    ~LibraryManager() = default;

    LibraryManager(const LibraryManager &) = delete;
    LibraryManager &operator=(const LibraryManager &) = delete;
    LibraryManager(LibraryManager &&) = delete;
    LibraryManager &operator=(LibraryManager &&) = delete;

    std::optional<SongMetadata> parseGuitarProFile(const QString &filePath);

  private:
    static bool isGuitarTrack(int gmProgram, int stringCount);
    static bool isBassTrack(int gmProgram, int stringCount);
};

#endif // LIBRARY_MANAGER_H
