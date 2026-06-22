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

    // aubio_notes segmentation + per-note YIN-FFT refinement for accurate Hz values.
    constexpr uint_t kNotesBufferSize = 2048;
    constexpr uint_t kNotesHopSize = 256;
    constexpr uint_t kPitchBufferSize = 8192;
    constexpr uint_t kPitchHopSize = 256;
    constexpr smpl_t kNotesSilenceDb = -85.0;
    constexpr smpl_t kPitchTolerance = 0.92;
    constexpr double kMergeGapMs = 55.0;
    constexpr double kMergeCents = 35.0;

    [[nodiscard]] double medianOf(std::vector<double> values) {
        if (values.empty()) {
            return 0.0;
        }
        const auto mid = values.size() / 2;
        std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(mid),
                         values.end());
        if (values.size() % 2 == 1) {
            return values[mid];
        }
        const double lowerMax = *std::max_element(values.begin(),
                                                  values.begin() + static_cast<std::ptrdiff_t>(mid));
        return (lowerMax + values[mid]) / 2.0;
    }

    [[nodiscard]] double centsBetween(double frequencyAHz, double frequencyBHz) {
        if (frequencyAHz <= 0.0 || frequencyBHz <= 0.0) {
            return 0.0;
        }
        return 1200.0 * std::log2(frequencyBHz / frequencyAHz);
    }

    [[nodiscard]] double robustMedianFrequency(std::vector<double> samples) {
        if (samples.empty()) {
            return 0.0;
        }
        if (samples.size() < 3) {
            return medianOf(std::move(samples));
        }

        const double roughMedian = medianOf(samples);
        std::vector<double> inliers;
        inliers.reserve(samples.size());
        for (const double frequencyHz : samples) {
            if (std::abs(centsBetween(roughMedian, frequencyHz)) <= 50.0) {
                inliers.push_back(frequencyHz);
            }
        }
        if (inliers.empty()) {
            return roughMedian;
        }
        return medianOf(std::move(inliers));
    }

    void refineNoteFrequencies(const std::vector<float> &monoSamples, uint_t sampleRateHz,
                               double timeOffsetSec, QVector<AudioAnalyzer::Note> &notes) {
        if (notes.isEmpty() || monoSamples.empty() || sampleRateHz == 0) {
            return;
        }

        aubio_pitch_t *pitchDetector =
            new_aubio_pitch("yinfft", kPitchBufferSize, kPitchHopSize, sampleRateHz);
        if (pitchDetector == nullptr) {
            return;
        }

        aubio_pitch_set_silence(pitchDetector, -90.0);
        aubio_pitch_set_tolerance(pitchDetector, kPitchTolerance);
        aubio_pitch_set_unit(pitchDetector, "Hz");

        fvec_t *input = new_fvec(kPitchHopSize);
        fvec_t *pitchOutput = new_fvec(1);
        if (input == nullptr || pitchOutput == nullptr) {
            del_fvec(input);
            del_fvec(pitchOutput);
            del_aubio_pitch(pitchDetector);
            return;
        }

        for (AudioAnalyzer::Note &note : notes) {
            const double relativeStartSec = note.startSec - timeOffsetSec;
            const double relativeEndSec = note.endSec - timeOffsetSec;
            if (relativeEndSec <= relativeStartSec) {
                continue;
            }

            const auto startFrame = static_cast<std::size_t>(
                std::max(0.0, relativeStartSec) * static_cast<double>(sampleRateHz));
            const auto endFrame = static_cast<std::size_t>(
                std::min(relativeEndSec * static_cast<double>(sampleRateHz),
                         static_cast<double>(monoSamples.size())));

            std::vector<double> pitchSamples;
            for (std::size_t offset = startFrame; offset + kPitchHopSize <= endFrame;
                 offset += kPitchHopSize) {
                for (uint_t index = 0; index < kPitchHopSize; ++index) {
                    input->data[index] = monoSamples[offset + index];
                }

                aubio_pitch_do(pitchDetector, input, pitchOutput);
                const double pitchHz = pitchOutput->data[0];
                if (pitchHz > 0.0) {
                    pitchSamples.push_back(pitchHz);
                }
            }

            if (!pitchSamples.empty()) {
                note.frequencyHz = robustMedianFrequency(pitchSamples);
            }
        }

        del_fvec(input);
        del_fvec(pitchOutput);
        del_aubio_pitch(pitchDetector);
    }

    void mergeAdjacentSimilarNotes(QVector<AudioAnalyzer::Note> &detectedNotes) {
        if (detectedNotes.size() < 2) {
            return;
        }

        QVector<AudioAnalyzer::Note> merged;
        merged.reserve(detectedNotes.size());
        merged.push_back(detectedNotes.first());

        for (int index = 1; index < detectedNotes.size(); ++index) {
            const AudioAnalyzer::Note &current = detectedNotes.at(index);
            AudioAnalyzer::Note &previous = merged.last();

            const double gapMs = (current.startSec - previous.endSec) * 1000.0;
            const double pitchDeltaCents =
                std::abs(centsBetween(previous.frequencyHz, current.frequencyHz));
            if (gapMs >= 0.0 && gapMs <= kMergeGapMs && pitchDeltaCents <= kMergeCents) {
                const double previousDuration = previous.endSec - previous.startSec;
                const double currentDuration = current.endSec - current.startSec;
                const double totalDuration = previousDuration + currentDuration;
                if (totalDuration > 0.0) {
                    previous.frequencyHz = (previous.frequencyHz * previousDuration +
                                            current.frequencyHz * currentDuration) /
                                           totalDuration;
                }
                previous.endSec = current.endSec;
                continue;
            }
            merged.push_back(current);
        }

        detectedNotes = std::move(merged);
    }

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
            new_aubio_notes("default", kNotesBufferSize, kNotesHopSize, sampleRateHz);
        if (notesDetector == nullptr) {
            errorMessage = AudioAnalyzer::tr("Could not initialize aubio note detector.");
            return detectedNotes;
        }

        aubio_notes_set_silence(notesDetector, kNotesSilenceDb);
        aubio_notes_set_minioi_ms(notesDetector, 25.0);
        aubio_notes_set_release_drop(notesDetector, 8.0);

        fvec_t *input = new_fvec(kNotesHopSize);
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
            static_cast<double>(kNotesHopSize) / static_cast<double>(sampleRateHz);
        const auto totalFrames = monoSamples.size();
        std::unordered_map<int, double> activeNoteStartSec;

        for (std::size_t offset = 0; offset + kNotesHopSize <= totalFrames;
             offset += kNotesHopSize) {
            for (uint_t index = 0; index < kNotesHopSize; ++index) {
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

        const double tailTimeSec =
            static_cast<double>(totalFrames) / static_cast<double>(sampleRateHz);
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

        refineNoteFrequencies(monoSamples, sampleRateHz, timeOffsetSec, detectedNotes);
        mergeAdjacentSimilarNotes(detectedNotes);
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
    jsonFile.flush();
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
