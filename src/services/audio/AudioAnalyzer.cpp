#include "AudioAnalyzer.h"

#include "AudioPlaybackEngine.h"

#include <aubio.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace {

    constexpr uint_t kAubioBufferSize = 2048;
    constexpr uint_t kAubioHopSize = 512;

    [[nodiscard]] std::vector<float> downmixToMono(const std::vector<float> &interleaved,
                                                   std::size_t channelCount) {
        if (channelCount <= 1) {
            return interleaved;
        }

        const auto frameCount = interleaved.size() / channelCount;
        std::vector<float> mono(frameCount, 0.0F);
        for (std::size_t frame = 0; frame < frameCount; ++frame) {
            float sum = 0.0F;
            for (std::size_t channel = 0; channel < channelCount; ++channel) {
                sum += interleaved[frame * channelCount + channel];
            }
            mono[frame] = sum / static_cast<float>(channelCount);
        }
        return mono;
    }

    [[nodiscard]] std::vector<float> extractRegionMono(const std::vector<float> &monoSamples,
                                                       uint_t sampleRateHz,
                                                       const AudioAnalyzer::AnalysisOptions &options,
                                                       double &timeOffsetSec) {
        if (!options.useRegion) {
            timeOffsetSec = 0.0;
            return monoSamples;
        }

        const auto totalFrames = monoSamples.size();
        const auto startFrame = static_cast<std::size_t>(
            (std::max<qint64>(0, options.regionStartMs) * static_cast<qint64>(sampleRateHz)) /
            1000LL);
        std::size_t endFrame = totalFrames;
        if (options.regionEndMs > 0) {
            endFrame = static_cast<std::size_t>(
                (options.regionEndMs * static_cast<qint64>(sampleRateHz)) / 1000LL);
        }
        endFrame = std::min(endFrame, totalFrames);
        if (endFrame <= startFrame) {
            return {};
        }

        timeOffsetSec = static_cast<double>(startFrame) / static_cast<double>(sampleRateHz);
        return std::vector<float>(monoSamples.begin() + static_cast<std::ptrdiff_t>(startFrame),
                                  monoSamples.begin() + static_cast<std::ptrdiff_t>(endFrame));
    }

    [[nodiscard]] QVector<AudioAnalyzer::Note>
    detectNotes(const std::vector<float> &monoSamples, uint_t sampleRateHz, double timeOffsetSec,
                QString &errorMessage) {
        QVector<AudioAnalyzer::Note> detectedNotes;

        if (monoSamples.empty() || sampleRateHz == 0) {
            errorMessage = AudioAnalyzer::tr("No audio data to analyze.");
            return detectedNotes;
        }

        aubio_notes_t *notesDetector =
            new_aubio_notes("default", kAubioBufferSize, kAubioHopSize, sampleRateHz);
        if (notesDetector == nullptr) {
            errorMessage = AudioAnalyzer::tr("Could not initialize aubio note detector.");
            return detectedNotes;
        }

        fvec_t *input = new_fvec(kAubioHopSize);
        fvec_t *output = new_fvec(3);
        if (input == nullptr || output == nullptr) {
            del_aubio_notes(notesDetector);
            if (input != nullptr) {
                del_fvec(input);
            }
            if (output != nullptr) {
                del_fvec(output);
            }
            errorMessage = AudioAnalyzer::tr("Could not allocate aubio buffers.");
            return detectedNotes;
        }

        const double secondsPerHop =
            static_cast<double>(kAubioHopSize) / static_cast<double>(sampleRateHz);
        const auto totalFrames = monoSamples.size();
        std::unordered_map<int, double> activeNoteStartSec;

        for (std::size_t offset = 0; offset + kAubioHopSize <= totalFrames;
             offset += kAubioHopSize) {
            for (uint_t index = 0; index < kAubioHopSize; ++index) {
                input->data[index] = monoSamples[offset + index];
            }

            aubio_notes_do(notesDetector, input, output);

            const double frameTimeSec =
                static_cast<double>(offset) / static_cast<double>(sampleRateHz);
            const int noteOn = static_cast<int>(std::lround(output->data[0]));
            const int noteOff = static_cast<int>(std::lround(output->data[2]));

            if (noteOff > 0) {
                const auto activeIt = activeNoteStartSec.find(noteOff);
                if (activeIt != activeNoteStartSec.end()) {
                    AudioAnalyzer::Note note;
                    note.startSec = activeIt->second + timeOffsetSec;
                    note.endSec = frameTimeSec + secondsPerHop + timeOffsetSec;
                    note.frequencyHz = aubio_miditofreq(static_cast<smpl_t>(noteOff));
                    detectedNotes.push_back(note);
                    activeNoteStartSec.erase(activeIt);
                }
            }

            if (noteOn > 0) {
                activeNoteStartSec[noteOn] = frameTimeSec;
            }
        }

        const double tailTimeSec = static_cast<double>(totalFrames) / static_cast<double>(sampleRateHz);
        for (const auto &[midiNote, startSec] : activeNoteStartSec) {
            AudioAnalyzer::Note note;
            note.startSec = startSec + timeOffsetSec;
            note.endSec = tailTimeSec + timeOffsetSec;
            note.frequencyHz = aubio_miditofreq(static_cast<smpl_t>(midiNote));
            detectedNotes.push_back(note);
        }

        del_fvec(input);
        del_fvec(output);
        del_aubio_notes(notesDetector);
        return detectedNotes;
    }

} // namespace

QString AudioAnalyzer::jsonOutputPathFor(const QString &audioFilePath) {
    const QFileInfo fileInfo(audioFilePath);
    return QDir(fileInfo.absolutePath())
        .filePath(fileInfo.completeBaseName() + QStringLiteral(".json"));
}

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
    return true;
}

int AudioAnalyzer::midiNoteFromFrequency(double frequencyHz) {
    if (frequencyHz <= 0.0) {
        return 0;
    }
    return static_cast<int>(std::lround(aubio_freqtomidi(static_cast<smpl_t>(frequencyHz))));
}

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

AudioAnalyzer::AnalysisResult AudioAnalyzer::analyze(const QString &filePath) const {
    const AnalysisOptions defaultOptions;
    return analyze(filePath, defaultOptions);
}

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

    const std::vector<float> mono = downmixToMono(decoded, channelCount);
    double timeOffsetSec = 0.0;
    const std::vector<float> analysisMono = extractRegionMono(mono, sampleRateHz, options, timeOffsetSec);
    if (analysisMono.empty()) {
        result.errorMessage = tr("Selected region is empty.");
        return result;
    }

    result.notes = detectNotes(analysisMono, sampleRateHz, timeOffsetSec, result.errorMessage);
    if (!result.errorMessage.isEmpty()) {
        return result;
    }

    result.success = true;
    return result;
}

bool AudioAnalyzer::writeJson(const QString &jsonPath, const QVector<Note> &notes,
                              QString &errorMessage) const {
    return saveNotesToJson(jsonPath, notes, errorMessage);
}

AudioAnalyzer::SaveResult AudioAnalyzer::analyzeAndSave(const QString &filePath) const {
    const AnalysisOptions defaultOptions;
    return analyzeAndSave(filePath, defaultOptions);
}

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
