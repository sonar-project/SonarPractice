#ifndef AUDIOANALYZER_H
#define AUDIOANALYZER_H

#include <QCoreApplication>
#include <QtGlobal>
#include <QString>
#include <QVector>

/**
 * @brief Offline note detection for guitar practice using aubio.
 *
 * Decodes an audio file, runs the selected detection mode, and writes notes to JSON.
 * The UI reads that JSON to show tab positions and a timeline.
 */
class AudioAnalyzer {
    Q_DECLARE_TR_FUNCTIONS(AudioAnalyzer)

  public:
    /** How notes are found in the audio. */
    enum class DetectionMode {
        Rhythmic = 0, /**< Onset-based: good for pick strokes and palm mutes. */
        Melodic = 1,  /**< Note-based: good for clean single-guitar takes. */
        Hybrid = 2,   /**< Rhythmic timing with melodic pitch where they overlap. */
    };

    /** One detected note with start, end, and pitch in Hz. */
    struct Note {
        double startSec{};
        double endSec{};
        double frequencyHz{};
    };

    /** Input for analyze(): optional region and detection mode. */
    struct AnalysisOptions {
        qint64 regionStartMs{};
        qint64 regionEndMs{};
        bool useRegion{};
        DetectionMode mode{DetectionMode::Rhythmic};
    };

    /** Result of analyze() without writing a file. */
    struct AnalysisResult {
        bool success{false};
        QString errorMessage;
        QVector<Note> notes;
    };

    /** Result of analyzeAndSave() including the JSON path. */
    struct SaveResult {
        bool success{false};
        QString errorMessage;
        int noteCount{};
        QString jsonPath;
    };

    /** Analyzes the full file with default options (rhythmic mode). */
    [[nodiscard]] AnalysisResult analyze(const QString &filePath) const;
    /** Analyzes a file or region with the given options. */
    [[nodiscard]] AnalysisResult analyze(const QString &filePath,
                                         const AnalysisOptions &options) const;

    /** Analyzes the full file and writes a sidecar JSON next to the audio. */
    [[nodiscard]] SaveResult analyzeAndSave(const QString &filePath) const;
    /** Analyzes with options and writes a sidecar JSON next to the audio. */
    [[nodiscard]] SaveResult analyzeAndSave(const QString &filePath,
                                            const AnalysisOptions &options) const;

    /** Returns the JSON path for an audio file (same folder, .json extension). */
    [[nodiscard]] static QString jsonOutputPathFor(const QString &audioFilePath);

    /** Reads notes from a pitch JSON file produced by analyzeAndSave(). */
    [[nodiscard]] static QVector<Note> loadNotesFromJson(const QString &jsonPath,
                                                         QString &errorMessage);
    /** Writes notes to JSON using the standard pitch schema. */
    [[nodiscard]] static bool saveNotesToJson(const QString &jsonPath, const QVector<Note> &notes,
                                              QString &errorMessage);
    /** Converts a frequency in Hz to a label such as "A4". */
    [[nodiscard]] static QString noteLabelFromFrequency(double frequencyHz);
    /** Converts a frequency in Hz to the nearest MIDI note number. */
    [[nodiscard]] static int midiNoteFromFrequency(double frequencyHz);

  private:
    [[nodiscard]] bool writeJson(const QString &jsonPath, const QVector<Note> &notes,
                                 QString &errorMessage) const;
};

#endif // AUDIOANALYZER_H
