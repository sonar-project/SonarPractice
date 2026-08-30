/**
 * @file AsciiTabRenderer.cpp
 * @brief Rough ASCII tablature export from libgp_parser songs.
 */

#include "AsciiTabRenderer.h"

#include <libgp_parser/beat.hpp>
#include <libgp_parser/duration.hpp>
#include <libgp_parser/load_song.hpp>
#include <libgp_parser/measure.hpp>
#include <libgp_parser/song.hpp>
#include <libgp_parser/track.hpp>

#include <QRegularExpression>
#include <algorithm>

namespace {

    constexpr long kSixteenthTicks = libgp_parser::kQuarterTime / 4;

    [[nodiscard]] int columnsForTicks(long ticks) {
        if (ticks <= 0) {
            return 1;
        }
        const long cols = (ticks + kSixteenthTicks - 1) / kSixteenthTicks;
        return static_cast<int>(std::max<long>(1, cols));
    }

    [[nodiscard]] QString dashes(int count) { return QString(count, QLatin1Char('-')); }

    [[nodiscard]] QString fretToken(const libgp_parser::Note &note) {
        if (note.tied_note) {
            return QStringLiteral("-");
        }
        return QString::number(note.value);
    }

    [[nodiscard]] const libgp_parser::Note *noteOnString(const libgp_parser::Voice &voice,
                                                         int stringNumber) {
        for (const libgp_parser::Note &note : voice.notes) {
            if (note.string == stringNumber) {
                return &note;
            }
        }
        return nullptr;
    }

    [[nodiscard]] QString tuningDisplayForTrack(const libgp_parser::Track &track) {
        if (!track.tuning_display.empty()) {
            return QString::fromStdString(track.tuning_display);
        }

        QStringList parts;
        parts.reserve(static_cast<int>(track.tuning_notes.size()));
        static const QRegularExpression digitRegex(QStringLiteral("[0-9]"));
        for (const std::string &note : track.tuning_notes) {
            QString pure = QString::fromStdString(note);
            pure.remove(digitRegex);
            if (!pure.isEmpty()) {
                parts.append(pure);
            }
        }
        return parts.join(QLatin1Char(' '));
    }

} // namespace

std::optional<AsciiTabRenderer::SongPreview> AsciiTabRenderer::loadFromFile(const QString &filePath) {
    const auto result = libgp_parser::load_song(filePath.toStdString());
    if (!result) {
        return std::nullopt;
    }

    const libgp_parser::Song &song = result.value();
    SongPreview preview;
    preview.title = QString::fromStdString(song.name());
    preview.artist = QString::fromStdString(song.artist());
    preview.bpm = song.tempo_bpm;
    preview.tracks.reserve(song.tracks.size());

    for (int i = 0; i < static_cast<int>(song.tracks.size()); ++i) {
        preview.tracks.push_back(previewForTrack(song, i));
    }
    return preview;
}

AsciiTabRenderer::TrackPreview AsciiTabRenderer::previewForTrack(const libgp_parser::Song &song,
                                                                 int trackIndex) {
    TrackPreview preview;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(song.tracks.size())) {
        preview.tabText = QStringLiteral("(invalid track)");
        return preview;
    }

    const libgp_parser::Track &track = song.tracks.at(static_cast<std::size_t>(trackIndex));
    preview.name = QString::fromStdString(track.name);
    preview.tuningDisplay = tuningDisplayForTrack(track);
    preview.percussion = track.is_percussion();
    preview.tabText = renderTrack(song, trackIndex);
    return preview;
}

QString AsciiTabRenderer::renderTrack(const libgp_parser::Song &song, int trackIndex) {
    if (trackIndex < 0 || trackIndex >= static_cast<int>(song.tracks.size())) {
        return QStringLiteral("(invalid track)");
    }

    const libgp_parser::Track &track = song.tracks.at(static_cast<std::size_t>(trackIndex));
    if (track.is_percussion()) {
        return QStringLiteral("(percussion track — no tablature)");
    }
    if (track.string_count() <= 0) {
        return QStringLiteral("(no string tuning on this track)");
    }
    return renderNonPercussionTrack(song, track);
}

QString AsciiTabRenderer::stringLabel(const libgp_parser::Track &track, int stringNumber) {
    const int index = stringNumber - 1;
    if (index < 0 || index >= static_cast<int>(track.tuning_notes.size())) {
        return QStringLiteral("?");
    }

    static const QRegularExpression digitRegex(QStringLiteral("[0-9]"));
    QString label = QString::fromStdString(track.tuning_notes.at(static_cast<std::size_t>(index)));
    label.remove(digitRegex);
    if (label.isEmpty()) {
        label = QLatin1Char('?');
    }
    return label.leftJustified(2, QLatin1Char(' '));
}

QString AsciiTabRenderer::renderNonPercussionTrack(const libgp_parser::Song &song,
                                                   const libgp_parser::Track &track) {
    const int stringCount = track.string_count();
    // GP string 1 = highest (top of tab) … string N = lowest (bottom).
    std::vector<QString> lines(static_cast<std::size_t>(stringCount));
    for (int s = 1; s <= stringCount; ++s) {
        lines[static_cast<std::size_t>(s - 1)] = stringLabel(track, s) + QLatin1Char('|');
    }

    for (const libgp_parser::Measure &measure : track.measures) {
        if (measure.header_index < 0 ||
            measure.header_index >= static_cast<int>(song.measure_headers.size())) {
            continue;
        }

        const libgp_parser::MeasureHeader &header = song.header_for(measure);
        const long measureStart = header.start;
        const long measureEnd = measureStart + header.length();

        std::vector<libgp_parser::Beat> beats = measure.beats;
        std::ranges::sort(beats, [](const libgp_parser::Beat &a, const libgp_parser::Beat &b) {
            return a.start < b.start;
        });

        long cursor = measureStart;
        for (const libgp_parser::Beat &beat : beats) {
            if (beat.start < measureStart || beat.start >= measureEnd) {
                continue;
            }

            const libgp_parser::Voice &voice = beat.voice(0);
            const long gapTicks = beat.start - cursor;
            if (gapTicks > 0) {
                const int gapCols = columnsForTicks(gapTicks);
                for (QString &line : lines) {
                    line += dashes(gapCols);
                }
                cursor = beat.start;
            }

            long durationTicks = voice.duration.time();
            if (durationTicks <= 0) {
                durationTicks = kSixteenthTicks;
            }
            // Do not spill past the bar line for rough preview spacing.
            durationTicks = std::min(durationTicks, measureEnd - beat.start);
            if (durationTicks <= 0) {
                durationTicks = kSixteenthTicks;
            }

            std::vector<QString> tokens(static_cast<std::size_t>(stringCount));
            int totalCols = columnsForTicks(durationTicks);
            for (int s = 1; s <= stringCount; ++s) {
                QString token = QStringLiteral("-");
                if (const libgp_parser::Note *note = noteOnString(voice, s)) {
                    token = fretToken(*note);
                }
                tokens[static_cast<std::size_t>(s - 1)] = token;
                totalCols = std::max(totalCols, static_cast<int>(token.size()));
            }
            for (int s = 1; s <= stringCount; ++s) {
                const QString &token = tokens[static_cast<std::size_t>(s - 1)];
                QString &line = lines[static_cast<std::size_t>(s - 1)];
                line += token;
                line += dashes(totalCols - static_cast<int>(token.size()));
            }
            cursor = beat.start + durationTicks;
        }

        if (cursor < measureEnd) {
            const int trailCols = columnsForTicks(measureEnd - cursor);
            for (QString &line : lines) {
                line += dashes(trailCols);
            }
        }

        for (QString &line : lines) {
            line += QLatin1Char('|');
        }
    }

    return QStringList(lines.begin(), lines.end()).join(QLatin1Char('\n'));
}
