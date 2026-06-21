/**
 * @file LibraryManager.cpp
 * @brief Guitar Pro parsing and track classification (guitar vs bass).
 */

#include "LibraryManager.h"
#include "instrument.h"

#include <QDebug>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <libgp_parser/load_song.hpp>
#include <libgp_parser/song.hpp>

namespace {

    constexpr int kMidiAcousticGuitarNylon = 24;
    constexpr int kMidiAcousticGuitarSteel = 31;
    constexpr int kMidiElectricBassFinger = 32;
    constexpr int kMidiElectricBassPick = 39;
    constexpr int kMidiProgramUnset = 0;
    constexpr int kGuitarMinStringCount = 6;
    constexpr int kBassMinStringCount = 4;
    constexpr int kBassMaxStringCount = 5;

} // namespace

/**
 * @brief Parses a Guitar Pro file and extracts song metadata and track information.
 * @param filePath The absolute path to the Guitar Pro file.
 * @return A SongMetadata object containing song details, or nullopt if parsing failed.
 */
std::optional<SongMetadata> LibraryManager::parseGuitarProFile(const QString &filePath) {
    std::string nativePath = filePath.toStdString();
    const auto result = libgp_parser::load_song(nativePath);

    if (!result) {
        return std::nullopt;
    }

    const libgp_parser::Song &song = result.value();

    SongMetadata meta;
    meta.title = QString::fromStdString(song.name());
    meta.artist = QString::fromStdString(song.artist());
    meta.album = QString::fromStdString(song.album());
    meta.bpm = song.tempo_bpm;

    // Use indices, not pointers — QList may reallocate on append() and invalidate pointers.
    int firstGuitarTrackIndex = -1;
    int firstBassTrackIndex = -1;

    for (const libgp_parser::Track &track : song.tracks) {
        TrackMetadata trackMeta;
        trackMeta.number = track.number;
        trackMeta.name = QString::fromStdString(track.name);
        trackMeta.instrumentName = instrument::midi_program_to_instrument_name(track.gm_program);

        // GTP often stores strings high→low; GPX/GP7 low→high — detect via pitch order.
        const bool reverseTuningOrder = track.tuning_pitches.size() >= 2 &&
                                        track.tuning_pitches.front() > track.tuning_pitches.back();

        const auto appendTuningNote = [&](const std::string &noteStr) {
            const QString note = QString::fromStdString(noteStr);
            trackMeta.tuningNotes.append(note);

            QString pureNote = note;
            static const QRegularExpression digitRegex("[0-9]");
            pureNote.remove(digitRegex);

            if (!trackMeta.tuningDisplay.isEmpty()) {
                trackMeta.tuningDisplay += " ";
            }
            trackMeta.tuningDisplay += pureNote;
        };

        if (reverseTuningOrder) {
            for (auto it = track.tuning_notes.rbegin(); it != track.tuning_notes.rend(); ++it) {
                appendTuningNote(*it);
            }
        } else {
            for (const std::string &note : track.tuning_notes) {
                appendTuningNote(note);
            }
        }

        const int stringCount = static_cast<int>(track.tuning_notes.size());

        // GPX provides gm_program; GTP (.gp3–.gp5) often does not — fall back to string count.
        trackMeta.isGuitar = isGuitarTrack(track.gm_program, stringCount);
        trackMeta.isBass = isBassTrack(track.gm_program, stringCount);

        meta.tracks.append(trackMeta);

        const int trackIndex = static_cast<int>(meta.tracks.size()) - 1;

        if (trackMeta.isGuitar && firstGuitarTrackIndex < 0) {
            firstGuitarTrackIndex = trackIndex;
        }
        if (trackMeta.isBass && firstBassTrackIndex < 0) {
            firstBassTrackIndex = trackIndex;
        }
    }

    // Pick the default UI track: guitar, bass, or the first track with tuning data.
    if (!meta.tracks.isEmpty()) {
        int selectedTrackIndex = 0;

        if (firstGuitarTrackIndex >= 0) {
            selectedTrackIndex = firstGuitarTrackIndex;
        } else if (firstBassTrackIndex >= 0) {
            selectedTrackIndex = firstBassTrackIndex;
        } else {
            for (int i = 0; i < meta.tracks.size(); ++i) {
                if (!meta.tracks.at(i).tuningDisplay.isEmpty()) {
                    selectedTrackIndex = i;
                    break;
                }
            }
        }

        const TrackMetadata &selectedTrack = meta.tracks.at(selectedTrackIndex);
        meta.defaultUiTuning = selectedTrack.tuningDisplay;
        meta.defaultUiTrackName = selectedTrack.name;
    }

    return meta;
}

/**
 * @brief Determines if a track is a guitar based on MIDI program and string count.
 * @param gmProgram The MIDI program number (instrument ID).
 * @param stringCount The number of strings on the instrument.
 * @return True if the track is classified as a guitar, false otherwise.
 */
bool LibraryManager::isGuitarTrack(int gmProgram, int stringCount) {
    if (gmProgram >= kMidiAcousticGuitarNylon && gmProgram <= kMidiAcousticGuitarSteel) {
        return true;
    }

    // GTP files omit gm_program; six or more strings suggests a guitar.
    return gmProgram == kMidiProgramUnset && stringCount >= kGuitarMinStringCount;
}

/**
 * @brief Determines if a track is a bass based on MIDI program and string count.
 * @param gmProgram The MIDI program number (instrument ID).
 * @param stringCount The number of strings on the instrument.
 * @return True if the track is classified as a bass, false otherwise.
 */
bool LibraryManager::isBassTrack(int gmProgram, int stringCount) {
    if (gmProgram >= kMidiElectricBassFinger && gmProgram <= kMidiElectricBassPick) {
        return true;
    }

    // GTP fallback: typical bass tunings use four or five strings.
    return gmProgram == kMidiProgramUnset && stringCount >= kBassMinStringCount &&
           stringCount <= kBassMaxStringCount;
}
