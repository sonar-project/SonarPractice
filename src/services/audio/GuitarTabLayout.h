#ifndef GUITARTABLAYOUT_H
#define GUITARTABLAYOUT_H

#include <QString>
#include <QStringList>
#include <QVector>

/** Fret position on a single tab staff line (0 = top / thin string). */
struct TabPosition {
    int stringIndex{-1};
    int fret{-1};

    [[nodiscard]] bool isValid() const { return stringIndex >= 0 && fret >= 0; }
};

/**
 * @brief Maps frequencies to guitar/bass tab positions (Guitar Pro style).
 *
 * Strings are ordered top-to-bottom: thin/high string first, thick/low string last.
 */
class GuitarTabLayout {
  public:
    enum class Instrument { Guitar, Bass };

    [[nodiscard]] int stringCount() const;
    [[nodiscard]] const QVector<int> &openStringMidi() const;
    [[nodiscard]] const QStringList &stringLabels() const;

    [[nodiscard]] static GuitarTabLayout standardGuitar();
    [[nodiscard]] static GuitarTabLayout standardBass();
    /** Parses tuning text such as "E A D G B E" or note names with octaves. */
    [[nodiscard]] static GuitarTabLayout fromTuningText(const QString &tuningText,
                                                      Instrument instrument);
    /** Resolves a DB tuning name or note list (e.g. "Drop D", "D G C F A D"). */
    [[nodiscard]] static GuitarTabLayout fromTuningName(const QString &tuningName,
                                                        Instrument instrument);
    /** Expands common tuning names to a low-to-high note string for fromTuningText(). */
    [[nodiscard]] static QString resolveTuningNoteText(const QString &tuningName);

    [[nodiscard]] TabPosition positionForFrequency(double frequencyHz) const;
    [[nodiscard]] double frequencyForPosition(int stringIndex, int fret) const;
    [[nodiscard]] static QString fretText(const TabPosition &position);
    [[nodiscard]] static int noteNameToMidi(const QString &noteName);

  private:
    Instrument m_instrument{Instrument::Guitar};
    QVector<int> m_openStringMidi{};
    QStringList m_stringLabels{};
};

#endif // GUITARTABLAYOUT_H
