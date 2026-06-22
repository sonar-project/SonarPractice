#include "GuitarTabLayout.h"

#include "AudioAnalyzer.h"

#include <aubio.h>

#include <QRegularExpression>

#include <cmath>
#include <algorithm>

namespace {

    constexpr int kMaxFret = 24;

    QVector<int> standardGuitarOpenMidi() {
        return {64, 59, 55, 50, 45, 40};
    }

    QVector<int> standardBassOpenMidi() { return {43, 38, 33, 28}; }

    QStringList standardGuitarLabels() {
        return {QStringLiteral("e"), QStringLiteral("B"), QStringLiteral("G"),
                QStringLiteral("D"), QStringLiteral("A"), QStringLiteral("E")};
    }

    QStringList standardBassLabels() {
        return {QStringLiteral("G"), QStringLiteral("D"), QStringLiteral("A"),
                QStringLiteral("E")};
    }

    int parseNoteNameToMidi(const QString &noteName, int fallbackMidi);

    QVector<int> defaultOpenMidiForLetters(const QStringList &letters, GuitarTabLayout::Instrument instrument) {
        const QVector<int> defaults = instrument == GuitarTabLayout::Instrument::Bass
                                          ? standardBassOpenMidi()
                                          : standardGuitarOpenMidi();
        QVector<int> result;
        result.reserve(letters.size());
        for (int index = 0; index < letters.size(); ++index) {
            const int fallbackMidi =
                index < defaults.size() ? defaults.at(index) : defaults.constLast();
            result.push_back(parseNoteNameToMidi(letters.at(index), fallbackMidi));
        }
        return result;
    }

    int parseNoteNameToMidi(const QString &noteName, int fallbackMidi) {
        static const QStringList pitchNames = {QStringLiteral("C"),  QStringLiteral("C#"),
                                               QStringLiteral("D"),  QStringLiteral("D#"),
                                               QStringLiteral("E"),  QStringLiteral("F"),
                                               QStringLiteral("F#"), QStringLiteral("G"),
                                               QStringLiteral("G#"), QStringLiteral("A"),
                                               QStringLiteral("A#"), QStringLiteral("B")};

        const QString trimmed = noteName.trimmed();
        if (trimmed.isEmpty()) {
            return fallbackMidi;
        }

        QString pitchPart = trimmed;
        int octave = 4;
        const QRegularExpression octaveRegex(QStringLiteral("(-?\\d+)\\s*$"));
        const QRegularExpressionMatch octaveMatch = octaveRegex.match(pitchPart);
        if (octaveMatch.hasMatch()) {
            octave = octaveMatch.captured(1).toInt();
            pitchPart.chop(octaveMatch.capturedLength());
            pitchPart = pitchPart.trimmed();
        }

        QString normalized = pitchPart;
        normalized.replace(QStringLiteral("♯"), QStringLiteral("#"));
        normalized.replace(QStringLiteral("♭"), QStringLiteral("b"));
        if (normalized.length() >= 2 && normalized.at(1) == QLatin1Char('b')) {
            normalized = normalized.at(0) + QStringLiteral("b");
        }

        int pitchClass = -1;
        for (int index = 0; index < pitchNames.size(); ++index) {
            if (normalized.compare(pitchNames.at(index), Qt::CaseInsensitive) == 0) {
                pitchClass = index;
                break;
            }
        }

        if (pitchClass < 0) {
            return fallbackMidi;
        }

        if (!octaveMatch.hasMatch()) {
            if (fallbackMidi > 0) {
                return fallbackMidi;
            }
            octave = 4;
        }

        return (octave + 1) * 12 + pitchClass;
    }

} // namespace

int GuitarTabLayout::stringCount() const { return m_openStringMidi.size(); }

const QVector<int> &GuitarTabLayout::openStringMidi() const { return m_openStringMidi; }

const QStringList &GuitarTabLayout::stringLabels() const { return m_stringLabels; }

GuitarTabLayout GuitarTabLayout::standardGuitar() {
    GuitarTabLayout layout;
    layout.m_instrument = Instrument::Guitar;
    layout.m_openStringMidi = standardGuitarOpenMidi();
    layout.m_stringLabels = standardGuitarLabels();
    return layout;
}

GuitarTabLayout GuitarTabLayout::standardBass() {
    GuitarTabLayout layout;
    layout.m_instrument = Instrument::Bass;
    layout.m_openStringMidi = standardBassOpenMidi();
    layout.m_stringLabels = standardBassLabels();
    return layout;
}

int GuitarTabLayout::noteNameToMidi(const QString &noteName) {
    return parseNoteNameToMidi(noteName, 0);
}

GuitarTabLayout GuitarTabLayout::fromTuningText(const QString &tuningText,
                                                Instrument instrument) {
    const QStringList tokens =
        tuningText.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);

    if (tokens.isEmpty()) {
        return instrument == Instrument::Bass ? standardBass() : standardGuitar();
    }

    QStringList orderedTokens = tokens;
    if (orderedTokens.size() >= 4) {
        std::reverse(orderedTokens.begin(), orderedTokens.end());
    }

    GuitarTabLayout layout;
    layout.m_instrument = instrument;
    layout.m_openStringMidi = defaultOpenMidiForLetters(orderedTokens, instrument);
    layout.m_stringLabels.reserve(orderedTokens.size());
    for (const QString &token : orderedTokens) {
        QString label = token;
        label.remove(QRegularExpression(QStringLiteral("\\d+")));
        if (label.isEmpty()) {
            label = token.left(1);
        }
        if (!label.isEmpty()) {
            label = label.at(0).toUpper() + label.mid(1);
        }
        layout.m_stringLabels.append(label);
    }

    if (layout.m_stringLabels.size() == 4) {
        layout.m_instrument = Instrument::Bass;
    } else if (layout.m_stringLabels.size() >= 6) {
        layout.m_instrument = Instrument::Guitar;
        layout.m_openStringMidi = layout.m_openStringMidi.mid(0, 6);
        layout.m_stringLabels = layout.m_stringLabels.mid(0, 6);
    }

    return layout;
}

TabPosition GuitarTabLayout::positionForFrequency(double frequencyHz) const {
    TabPosition best;
    if (frequencyHz <= 0.0 || m_openStringMidi.isEmpty()) {
        return best;
    }

    const int midi = AudioAnalyzer::midiNoteFromFrequency(frequencyHz);
    int bestFret = kMaxFret + 1;

    for (int stringIndex = 0; stringIndex < m_openStringMidi.size(); ++stringIndex) {
        const int fret = midi - m_openStringMidi.at(stringIndex);
        if (fret < 0 || fret > kMaxFret) {
            continue;
        }
        if (fret < bestFret || (fret == bestFret && stringIndex > best.stringIndex)) {
            bestFret = fret;
            best.stringIndex = stringIndex;
            best.fret = fret;
        }
    }

    return best;
}

double GuitarTabLayout::frequencyForPosition(int stringIndex, int fret) const {
    if (stringIndex < 0 || stringIndex >= m_openStringMidi.size() || fret < 0) {
        return 0.0;
    }
    const int midi = m_openStringMidi.at(stringIndex) + fret;
    return static_cast<double>(aubio_miditofreq(static_cast<smpl_t>(midi)));
}

QString GuitarTabLayout::fretText(const TabPosition &position) {
    if (!position.isValid()) {
        return QStringLiteral("?");
    }
    return QString::number(position.fret);
}
