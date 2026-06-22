#ifndef AUDIOANALYZER_H
#define AUDIOANALYZER_H

#include <QCoreApplication>
#include <QtGlobal>
#include <QString>
#include <QVector>

/**
 * @brief Offline pitch detection using aubio.
 *
 * Decodes an audio file, detects notes, and can write results as JSON next to
 * the source file.
 */
class AudioAnalyzer {
    Q_DECLARE_TR_FUNCTIONS(AudioAnalyzer)

  public:
    struct Note {
        double startSec{};
        double endSec{};
        double frequencyHz{};
    };

    struct AnalysisOptions {
        qint64 regionStartMs{};
        qint64 regionEndMs{};
        bool useRegion{};
    };

    struct AnalysisResult {
        bool success{false};
        QString errorMessage;
        QVector<Note> notes;
    };

    struct SaveResult {
        bool success{false};
        QString errorMessage;
        int noteCount{};
        QString jsonPath;
    };

    [[nodiscard]] AnalysisResult analyze(const QString &filePath) const;
    [[nodiscard]] AnalysisResult analyze(const QString &filePath,
                                         const AnalysisOptions &options) const;

    [[nodiscard]] SaveResult analyzeAndSave(const QString &filePath) const;
    [[nodiscard]] SaveResult analyzeAndSave(const QString &filePath,
                                            const AnalysisOptions &options) const;

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
