#include "AudioAnalyzerMelodic.h"

#include "AudioAnalyzerCommon.h"

#include <aubio.h>

#include <unordered_map>

namespace AudioAnalyzerInternal {

    namespace {

        constexpr uint_t kNotesBufferSize = 2048;
        constexpr uint_t kNotesHopSize = 256;
        constexpr smpl_t kNotesSilenceDb = -85.0;

    } // namespace

    /**
     * @brief Tracks note on/off events with aubio_notes and refines pitch with yinfft.
     */
    QVector<AudioAnalyzer::Note>
    detectNotesMelodic(const std::vector<float> &monoSamples, uint_t sampleRateHz,
                       double timeOffsetSec, QString &errorMessage) {
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

        refineNoteFrequencies(monoSamples, sampleRateHz, timeOffsetSec, detectedNotes,
                              PitchAlgorithm::YinFft);
        fillMissingPitchFromNeighbors(detectedNotes);
        removeNotesWithoutPitch(detectedNotes);
        return detectedNotes;
    }

} // namespace AudioAnalyzerInternal
