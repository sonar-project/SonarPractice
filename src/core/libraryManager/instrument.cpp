#include "instrument.h"

namespace {

    constexpr int kMidiAcousticGrandPiano = 0;
    constexpr int kMidiAcousticGuitarNylon = 24;
    constexpr int kMidiAcousticGuitarSteel = 25;
    constexpr int kMidiElectricGuitarJazz = 26;
    constexpr int kMidiElectricGuitarClean = 27;
    constexpr int kMidiElectricGuitarMuted = 28;
    constexpr int kMidiOverdrivenGuitar = 29;
    constexpr int kMidiDistortionGuitar = 30;
    constexpr int kMidiElectricBassFinger = 33;
    constexpr int kMidiElectricBassPick = 34;

} // namespace

namespace instrument {
    QString midi_program_to_instrument_name(int program) {
        switch (program) {
        case kMidiAcousticGrandPiano:
            return "Acoustic Grand Piano";
        case kMidiAcousticGuitarNylon:
            return "Acoustic Guitar (Nylon)";
        case kMidiAcousticGuitarSteel:
            return "Acoustic Guitar (Steel)";
        case kMidiElectricGuitarJazz:
            return "Electric Guitar (Jazz)";
        case kMidiElectricGuitarClean:
            return "Electric Guitar (Clean)";
        case kMidiElectricGuitarMuted:
            return "Electric Guitar (Muted)";
        case kMidiOverdrivenGuitar:
            return "Overdriven Guitar";
        case kMidiDistortionGuitar:
            return "Distortion Guitar";
        case kMidiElectricBassFinger:
            return "Electric Bass (Finger)";
        case kMidiElectricBassPick:
            return "Electric Bass (Pick)";
        // Add more mappings as needed over time.
        default:
            return "Unknown Instrument (MIDI " + QString::number(program) + ")";
        }
    }
} // namespace instrument
