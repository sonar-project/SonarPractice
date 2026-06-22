#include "AudioAnalyzer.h"

#include "AudioPlaybackEngine.h"

#include <aubio.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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

    [[nodiscard]] QVector<AudioAnalyzer::Note>
    detectNotes(const std::vector<float> &monoSamples, uint_t sampleRateHz, QString &errorMessage) {
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
                    note.startSec = activeIt->second;
                    note.endSec = frameTimeSec + secondsPerHop;
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
            note.startSec = startSec;
            note.endSec = tailTimeSec;
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

AudioAnalyzer::AnalysisResult AudioAnalyzer::analyze(const QString &filePath) const {
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
    result.notes = detectNotes(mono, sampleRateHz, result.errorMessage);
    if (!result.errorMessage.isEmpty()) {
        return result;
    }

    result.success = true;
    return result;
}

bool AudioAnalyzer::analyzeAndSave(const QString &filePath, QString &errorMessage) const {
    const AnalysisResult result = analyze(filePath);
    if (!result.success) {
        errorMessage = result.errorMessage;
        return false;
    }

    const QString jsonPath = jsonOutputPathFor(filePath);
    QJsonArray notesArray;
    for (const Note &note : result.notes) {
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
