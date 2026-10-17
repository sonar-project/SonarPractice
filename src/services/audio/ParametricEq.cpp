#include "ParametricEq.h"
#include "BiquadFilter.h"

namespace {

    constexpr double kLowShelfHz = 120.0;
    constexpr double kMidPeakHz = 2500.0;
    constexpr double kHighShelfHz = 6000.0;
    constexpr double kDefaultQ = 0.9;

} // namespace

/**
 * @brief Returns a string identifier for a given EQ preset.
 * @param preset The preset type.
 * @return A unique string ID.
 */
QString ParametricEq::presetId(Preset preset) {
    switch (preset) {
    case Preset::Flat:
        return QStringLiteral("flat");
    case Preset::ReduceLow:
        return QStringLiteral("reduce_low");
    case Preset::ReduceHigh:
        return QStringLiteral("reduce_high");
    case Preset::MidFocus:
        return QStringLiteral("mid_focus");
    }
    return QStringLiteral("flat");
}

/**
 * @brief Maps a string identifier back to its corresponding Preset enum.
 * @param presetId The string ID of the preset.
 * @return The associated Preset, or Flat if not found.
 */
ParametricEq::Preset ParametricEq::presetFromId(const QString &presetId) {
    if (presetId == QStringLiteral("reduce_low")) {
        return Preset::ReduceLow;
    }
    if (presetId == QStringLiteral("reduce_high")) {
        return Preset::ReduceHigh;
    }
    if (presetId == QStringLiteral("mid_focus")) {
        return Preset::MidFocus;
    }
    return Preset::Flat;
}

/**
 * @brief Sets the sample rate and updates filter coefficients.
 * @param sampleRateHz Sample rate in Hz.
 */
void ParametricEq::setSampleRate(double sampleRateHz) {
    m_sampleRateHz = sampleRateHz;
    rebuildFilters();
}

/**
 * @brief Sets the EQ preset and updates filter coefficients.
 * @param preset The chosen Preset.
 */
void ParametricEq::setPreset(Preset preset) {
    m_preset = preset;
    rebuildFilters();
}

/**
 * @brief Sets the EQ preset using a string identifier.
 * @param presetId The string ID of the preset.
 */
void ParametricEq::setPresetById(const QString &presetId) { setPreset(presetFromId(presetId)); }

/**
 * @brief Recalculates filter coefficients for all stages based on the current preset.
 */
void ParametricEq::rebuildFilters() {
    const double sampleRate = static_cast<double>(m_sampleRateHz);

    double lowGain = 0.0;
    double midGain = 0.0;
    double highGain = 0.0;

    switch (m_preset) {
    case Preset::Flat:
        break;
    case Preset::ReduceLow:
        lowGain = -6.0;
        break;
    case Preset::ReduceHigh:
        highGain = -6.0;
        break;
    case Preset::MidFocus:
        lowGain = -3.0;
        highGain = -3.0;
        midGain = 3.0;
        break;
    }

    m_lowShelf.configure(BiquadFilter::Type::LowShelf,
                         BiquadFilter::Filter{sampleRate, kLowShelfHz, lowGain, kDefaultQ});
    m_midPeak.configure(BiquadFilter::Type::Peaking,
                        BiquadFilter::Filter{sampleRate, kMidPeakHz, midGain, kDefaultQ});
    m_highShelf.configure(BiquadFilter::Type::HighShelf,
                          BiquadFilter::Filter{sampleRate, kHighShelfHz, highGain, kDefaultQ});
}

/**
 * @brief Applies the EQ filter chain to the provided audio samples.
 * @param interleavedSamples Vector of audio samples to process.
 * @param channelCount The number of audio channels.
 */
void ParametricEq::process(std::vector<float> &interleavedSamples, int channelCount) {
    if (m_preset == Preset::Flat) {
        return;
    }
    m_lowShelf.process(interleavedSamples, static_cast<std::size_t>(channelCount));
    m_midPeak.process(interleavedSamples, static_cast<std::size_t>(channelCount));
    m_highShelf.process(interleavedSamples, static_cast<std::size_t>(channelCount));
}
