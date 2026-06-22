#include "GuitarTabLayout.h"

#include "AudioAnalyzer.h"

#include <aubio.h>

#include <QRegularExpression>

#include <cmath>
#include <algorithm>

#include <QHash>

namespace {

    constexpr int kMaxFret = 24;
    /** Prefer thick strings up to this fret for rhythm-guitar mapping. */
    constexpr int kRhythmGuitarPreferMaxFret = 8;

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
        static const QStringList flatNames = {QStringLiteral("C"),  QStringLiteral("Db"),
                                              QStringLiteral("D"),  QStringLiteral("Eb"),
                                              QStringLiteral("E"),  QStringLiteral("F"),
                                              QStringLiteral("Gb"), QStringLiteral("G"),
                                              QStringLiteral("Ab"), QStringLiteral("A"),
                                              QStringLiteral("Bb"), QStringLiteral("B")};

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
            for (int index = 0; index < flatNames.size(); ++index) {
                if (normalized.compare(flatNames.at(index), Qt::CaseInsensitive) == 0) {
                    pitchClass = index;
                    break;
                }
            }
        }

        if (pitchClass < 0) {
            return fallbackMidi;
        }

        if (!octaveMatch.hasMatch()) {
            if (fallbackMidi > 0) {
                const int fallbackOctave = fallbackMidi / 12 - 1;
                return (fallbackOctave + 1) * 12 + pitchClass;
            }
            octave = 4;
        }

        return (octave + 1) * 12 + pitchClass;
    }

    [[nodiscard]] bool isNoteToken(const QString &token) {
        static const QRegularExpression noteTokenRegex(
            QStringLiteral("^[A-Ga-g](?:[#b♯♭]|b)?\\d*$"));
        return noteTokenRegex.match(token.trimmed()).hasMatch();
    }

    [[nodiscard]] QString normalizeTuningKey(const QString &tuningName) {
        QString key = tuningName.trimmed().toLower();
        key.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
        return key;
    }

    [[nodiscard]] QString aliasNoteTextForTuningName(const QString &tuningName) {
        static const QHash<QString, QString> aliases = {
            {QStringLiteral("standard"), QStringLiteral("E A D G B E")},
            {QStringLiteral("e standard"), QStringLiteral("E A D G B E")},
            {QStringLiteral("standard e"), QStringLiteral("E A D G B E")},
            {QStringLiteral("eadgbe"), QStringLiteral("E A D G B E")},
            {QStringLiteral("drop d"), QStringLiteral("D A D G B E")},
            {QStringLiteral("dropped d"), QStringLiteral("D A D G B E")},
            {QStringLiteral("dadgbe"), QStringLiteral("D A D G B E")},
            {QStringLiteral("d standard"), QStringLiteral("D G C F A D")},
            {QStringLiteral("dgcfad"), QStringLiteral("D G C F A D")},
            {QStringLiteral("whole step down"), QStringLiteral("D G C F A D")},
            {QStringLiteral("c standard"), QStringLiteral("C F Bb Eb G C")},
            {QStringLiteral("dadgad"), QStringLiteral("D A D G A D")},
            {QStringLiteral("open g"), QStringLiteral("D G D G B D")},
            {QStringLiteral("open d"), QStringLiteral("D A D F# A D")},
            {QStringLiteral("drop c"), QStringLiteral("C G C F A D")},
            {QStringLiteral("drop b"), QStringLiteral("B F# B E G# C#")},
            {QStringLiteral("standard bass"), QStringLiteral("G D A E")},
            {QStringLiteral("e a d g"), QStringLiteral("G D A E")},
        };

        const QString key = normalizeTuningKey(tuningName);
        const auto aliasIt = aliases.constFind(key);
        if (aliasIt != aliases.constEnd()) {
            return aliasIt.value();
        }
        return {};
    }

} // namespace

/** @brief Returns the number of tab strings (4 bass or 6 guitar). */
int GuitarTabLayout::stringCount() const { return m_openStringMidi.size(); }

/** @brief Returns open-string MIDI numbers, thin string first. */
const QVector<int> &GuitarTabLayout::openStringMidi() const { return m_openStringMidi; }

/** @brief Returns display labels for each string line. */
const QStringList &GuitarTabLayout::stringLabels() const { return m_stringLabels; }

/** @brief Builds standard EADGBE guitar layout. */
GuitarTabLayout GuitarTabLayout::standardGuitar() {
    GuitarTabLayout layout;
    layout.m_instrument = Instrument::Guitar;
    layout.m_openStringMidi = standardGuitarOpenMidi();
    layout.m_stringLabels = standardGuitarLabels();
    return layout;
}

/** @brief Builds standard GDAD bass layout. */
GuitarTabLayout GuitarTabLayout::standardBass() {
    GuitarTabLayout layout;
    layout.m_instrument = Instrument::Bass;
    layout.m_openStringMidi = standardBassOpenMidi();
    layout.m_stringLabels = standardBassLabels();
    return layout;
}

/** @brief Parses a note name such as "E" or "F#3" to a MIDI number. */
int GuitarTabLayout::noteNameToMidi(const QString &noteName) {
    return parseNoteNameToMidi(noteName, 0);
}

/** @brief Builds a layout from a space-separated list of note names. */
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

/** @brief Converts a tuning name or note list to text for fromTuningText(). */
QString GuitarTabLayout::resolveTuningNoteText(const QString &tuningName) {
    const QString trimmed = tuningName.trimmed();
    if (trimmed.isEmpty()) {
        return trimmed;
    }

    const QString aliasText = aliasNoteTextForTuningName(trimmed);
    if (!aliasText.isEmpty()) {
        return aliasText;
    }

    const QStringList tokens =
        trimmed.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (tokens.size() >= 3) {
        bool allNotes = true;
        for (const QString &token : tokens) {
            if (!isNoteToken(token)) {
                allNotes = false;
                break;
            }
        }
        if (allNotes) {
            return tokens.join(QLatin1Char(' '));
        }
    }

    return trimmed;
}

/** @brief Resolves a DB tuning name (e.g. "Drop D") to a GuitarTabLayout. */
GuitarTabLayout GuitarTabLayout::fromTuningName(const QString &tuningName, Instrument instrument) {
    const QString noteText = resolveTuningNoteText(tuningName);
    if (noteText.isEmpty()) {
        return instrument == Instrument::Bass ? standardBass() : standardGuitar();
    }
    return fromTuningText(noteText, instrument);
}

/**
 * @brief Maps a frequency to the best string and fret on the tab staff.
 *
 * For guitar, prefers thick strings with frets up to 8 for rhythm parts.
 */
TabPosition GuitarTabLayout::positionForFrequency(double frequencyHz) const {
    TabPosition best;
    if (frequencyHz <= 0.0 || m_openStringMidi.isEmpty()) {
        return best;
    }

    const int midi = AudioAnalyzer::midiNoteFromFrequency(frequencyHz);
    int bestFret = kMaxFret + 1;
    int rhythmicStringIndex = -1;
    int rhythmicFret = kMaxFret + 1;

    for (int stringIndex = 0; stringIndex < m_openStringMidi.size(); ++stringIndex) {
        const int fret = midi - m_openStringMidi.at(stringIndex);
        if (fret < 0 || fret > kMaxFret) {
            continue;
        }

        if (m_instrument == Instrument::Guitar && fret <= kRhythmGuitarPreferMaxFret) {
            if (stringIndex > rhythmicStringIndex ||
                (stringIndex == rhythmicStringIndex && fret < rhythmicFret)) {
                rhythmicStringIndex = stringIndex;
                rhythmicFret = fret;
            }
        }

        if (fret < bestFret || (fret == bestFret && stringIndex > best.stringIndex)) {
            bestFret = fret;
            best.stringIndex = stringIndex;
            best.fret = fret;
        }
    }

    if (rhythmicStringIndex >= 0) {
        best.stringIndex = rhythmicStringIndex;
        best.fret = rhythmicFret;
    }

    return best;
}

/** @brief Returns the frequency in Hz for a string and fret position. */
double GuitarTabLayout::frequencyForPosition(int stringIndex, int fret) const {
    if (stringIndex < 0 || stringIndex >= m_openStringMidi.size() || fret < 0) {
        return 0.0;
    }
    const int midi = m_openStringMidi.at(stringIndex) + fret;
    return static_cast<double>(aubio_miditofreq(static_cast<smpl_t>(midi)));
}

/** @brief Returns the fret number as text for display on the tab. */
QString GuitarTabLayout::fretText(const TabPosition &position) {
    if (!position.isValid()) {
        return QStringLiteral("?");
    }
    return QString::number(position.fret);
}
