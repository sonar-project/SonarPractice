#include "AudioAnalyzer.h"

#include "AudioAnalyzerCommon.h"
#include "AudioAnalyzerHybrid.h"
#include "AudioAnalyzerMelodic.h"
#include "AudioAnalyzerRhythm.h"
#include "AudioPlaybackEngine.h"

#include <aubio.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

    /**
     * @brief Picks the detection backend for the given mode.
     */
    [[nodiscard]] QVector<AudioAnalyzer::Note>
    detectNotesForMode(const std::vector<float> &monoSamples, uint_t sampleRateHz,
                       double timeOffsetSec, AudioAnalyzer::DetectionMode mode,
                       QString &errorMessage) {
        if (monoSamples.empty() || sampleRateHz == 0) {
            errorMessage = AudioAnalyzer::tr("No audio data to analyze.");
            return {};
        }

        switch (mode) {
        case AudioAnalyzer::DetectionMode::Melodic:
            return AudioAnalyzerInternal::detectNotesMelodic(monoSamples, sampleRateHz,
                                                             timeOffsetSec, errorMessage);
        case AudioAnalyzer::DetectionMode::Hybrid:
            return AudioAnalyzerInternal::detectNotesHybrid(monoSamples, sampleRateHz, timeOffsetSec,
                                                            errorMessage);
        case AudioAnalyzer::DetectionMode::Rhythmic:
        default:
            return AudioAnalyzerInternal::detectNotesRhythmic(monoSamples, sampleRateHz,
                                                            timeOffsetSec, errorMessage);
        }
    }

} // namespace

/**
 * @brief Returns the JSON sidecar path for an audio file.
 */
QString AudioAnalyzer::jsonOutputPathFor(const QString &audioFilePath) {
    const QFileInfo fileInfo(audioFilePath);
    return QDir(fileInfo.absolutePath())
        .filePath(fileInfo.completeBaseName() + QStringLiteral(".json"));
}

/**
 * @brief Loads detected notes from a pitch JSON file.
 */
QVector<AudioAnalyzer::Note> AudioAnalyzer::loadNotesFromJson(const QString &jsonPath,
                                                              QString &errorMessage) {
    QVector<Note> notes;

    QFile jsonFile(jsonPath);
    if (!jsonFile.open(QIODevice::ReadOnly)) {
        errorMessage = tr("Could not read JSON file: %1").arg(jsonPath);
        return notes;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(jsonFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        errorMessage = tr("Invalid pitch JSON: %1").arg(parseError.errorString());
        return notes;
    }

    const QJsonArray notesArray = document.object().value(QStringLiteral("notes")).toArray();
    notes.reserve(notesArray.size());
    for (const QJsonValue &value : notesArray) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject object = value.toObject();
        Note note;
        note.startSec = object.value(QStringLiteral("start")).toDouble();
        note.endSec = object.value(QStringLiteral("end")).toDouble();
        note.frequencyHz = object.value(QStringLiteral("freq")).toDouble();
        if (note.endSec > note.startSec && note.frequencyHz > 0.0) {
            notes.push_back(note);
        }
    }
    return notes;
}

/**
 * @brief Writes detected notes to a pitch JSON file.
 */
bool AudioAnalyzer::saveNotesToJson(const QString &jsonPath, const QVector<Note> &notes,
                                    QString &errorMessage) {
    QJsonArray notesArray;
    for (const Note &note : notes) {
        QJsonObject noteObject;
        noteObject.insert(QStringLiteral("start"), note.startSec);
        noteObject.insert(QStringLiteral("end"), note.endSec);
        noteObject.insert(QStringLiteral("freq"), note.frequencyHz);
        notesArray.append(noteObject);
    }

    QJsonObject rootObject;
    rootObject.insert(QStringLiteral("notes"), notesArray);

    QFile jsonFile(jsonPath);
    if (!jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        errorMessage = tr("Could not write JSON file: %1").arg(jsonPath);
        return false;
    }

    jsonFile.write(QJsonDocument(rootObject).toJson(QJsonDocument::Indented));
    jsonFile.flush();
    return true;
}

/**
 * @brief Converts a frequency in Hz to the nearest MIDI note number.
 */
int AudioAnalyzer::midiNoteFromFrequency(double frequencyHz) {
    if (frequencyHz <= 0.0) {
        return 0;
    }
    return static_cast<int>(std::lround(aubio_freqtomidi(static_cast<smpl_t>(frequencyHz))));
}

/**
 * @brief Converts a frequency in Hz to a note name with octave (e.g. "A4").
 */
QString AudioAnalyzer::noteLabelFromFrequency(double frequencyHz) {
    if (frequencyHz <= 0.0) {
        return QStringLiteral("—");
    }

    const int midi = midiNoteFromFrequency(frequencyHz);
    static const QStringList pitchNames = {QStringLiteral("C"),  QStringLiteral("C#"),
                                           QStringLiteral("D"),  QStringLiteral("D#"),
                                           QStringLiteral("E"),  QStringLiteral("F"),
                                           QStringLiteral("F#"), QStringLiteral("G"),
                                           QStringLiteral("G#"), QStringLiteral("A"),
                                           QStringLiteral("A#"), QStringLiteral("B")};
    const int pitchClass = ((midi % 12) + 12) % 12;
    const int octave = midi / 12 - 1;
    return pitchNames.at(pitchClass) + QString::number(octave);
}

/**
 * @brief Analyzes the full audio file with default options.
 */
AudioAnalyzer::AnalysisResult AudioAnalyzer::analyze(const QString &filePath) const {
    const AnalysisOptions defaultOptions;
    return analyze(filePath, defaultOptions);
}

/**
 * @brief Decodes audio, runs the selected detection mode, and returns notes.
 */
AudioAnalyzer::AnalysisResult AudioAnalyzer::analyze(const QString &filePath,
                                                     const AnalysisOptions &options) const {
    AnalysisResult result;

    SampleRequest decodeRequest;
    QString decodeError;
    const std::vector<float> decoded =
        AudioPlaybackEngine::decodeFileToFloat(filePath, decodeRequest, decodeError);
    if (!decodeError.isEmpty() || decoded.empty()) {
        result.errorMessage =
            decodeError.isEmpty() ? tr("Decoder returned no audio data.") : decodeError;
        return result;
    }

    const auto channelCount = static_cast<std::size_t>(decodeRequest.channelCount);
    if (channelCount == 0) {
        result.errorMessage = tr("Invalid channel count in decoded audio.");
        return result;
    }

    const auto sampleRateHz = static_cast<uint_t>(decodeRequest.sampleRateHz);
    if (sampleRateHz == 0) {
        result.errorMessage = tr("Invalid sample rate in decoded audio.");
        return result;
    }

    const std::vector<float> mono =
        AudioAnalyzerInternal::downmixToMono(decoded, channelCount);
    double timeOffsetSec = 0.0;
    const std::vector<float> analysisMono =
        AudioAnalyzerInternal::extractRegionMono(mono, sampleRateHz, options, timeOffsetSec);
    if (analysisMono.empty()) {
        result.errorMessage = tr("Selected region is empty.");
        return result;
    }

    result.notes = detectNotesForMode(analysisMono, sampleRateHz, timeOffsetSec, options.mode,
                                    result.errorMessage);
    if (!result.errorMessage.isEmpty()) {
        return result;
    }

    result.success = true;
    return result;
}

/**
 * @brief Writes notes to JSON using saveNotesToJson().
 */
bool AudioAnalyzer::writeJson(const QString &jsonPath, const QVector<Note> &notes,
                              QString &errorMessage) const {
    return saveNotesToJson(jsonPath, notes, errorMessage);
}

/**
 * @brief Analyzes the full file and saves notes to a sidecar JSON.
 */
AudioAnalyzer::SaveResult AudioAnalyzer::analyzeAndSave(const QString &filePath) const {
    const AnalysisOptions defaultOptions;
    return analyzeAndSave(filePath, defaultOptions);
}

/**
 * @brief Analyzes with options and saves notes to a sidecar JSON.
 */
AudioAnalyzer::SaveResult AudioAnalyzer::analyzeAndSave(const QString &filePath,
                                                        const AnalysisOptions &options) const {
    SaveResult saveResult;
    const AnalysisResult result = analyze(filePath, options);
    if (!result.success) {
        saveResult.errorMessage = result.errorMessage;
        return saveResult;
    }

    saveResult.jsonPath = jsonOutputPathFor(filePath);
    if (!writeJson(saveResult.jsonPath, result.notes, saveResult.errorMessage)) {
        return saveResult;
    }

    saveResult.noteCount = result.notes.size();
    saveResult.success = true;
    return saveResult;
}
