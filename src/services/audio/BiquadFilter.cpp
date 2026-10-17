#include "BiquadFilter.h"

#include <QtMath>
#include <cmath>

namespace {

    constexpr double kPiMultiplier = 2.0;

    // Clamps a frequency within a valid range based on the sample rate.
    double clampFrequency(double sampleRateHz, double frequencyToClamp) {
        const double nyquist = sampleRateHz * 0.49;
        return qBound(20.0, frequencyToClamp, nyquist);
    }

} // namespace

/**
 * @brief Configures the biquad filter coefficients based on the desired type and parameters.
 * @param filterType The type of filter (LowShelf, HighShelf, or Peaking).
 * @param filter A structure containing frequency, gain, Q factor, and sample rate settings.
 */
void BiquadFilter::configure(Type filterType, Filter filter) {
    m_type = filterType;
    m_sampleRateHz = filter.sampleRateHz;
    m_frequencyHz = clampFrequency(filter.sampleRateHz, filter.frequencyHz);
    m_gainDb = filter.gainDb;
    m_qFactor = qMax(0.1, filter.qFactor);

    const double angularFrequency = kPiMultiplier * m_frequencyHz / m_sampleRateHz;
    const double sinOmega = std::sin(angularFrequency);
    const double cosOmega = std::cos(angularFrequency);
    const double alpha = sinOmega / (kPiMultiplier * m_qFactor);
    const double linearGain = std::pow(10.0, m_gainDb / 20.0);
    const double amplitude = std::sqrt(linearGain);

    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a0 = 1.0;
    double a1 = 0.0;
    double a2 = 0.0;

    switch (m_type) {
    case Type::LowShelf: {
        const double twoSqrtAalpha = kPiMultiplier * amplitude * alpha;
        const double ap1 = amplitude + 1.0;
        const double am1 = amplitude - 1.0;
        b0 = amplitude * (ap1 - am1 * cosOmega + twoSqrtAalpha);
        b1 = kPiMultiplier * amplitude * (am1 - ap1 * cosOmega);
        b2 = amplitude * (ap1 - am1 * cosOmega - twoSqrtAalpha);
        a0 = ap1 + am1 * cosOmega + twoSqrtAalpha;
        a1 = -kPiMultiplier * (am1 + ap1 * cosOmega);
        a2 = ap1 + am1 * cosOmega - twoSqrtAalpha;
        break;
    }
    case Type::HighShelf: {
        const double twoSqrtAalpha = kPiMultiplier * amplitude * alpha;
        const double ap1 = amplitude + 1.0;
        const double am1 = amplitude - 1.0;
        b0 = amplitude * (ap1 + am1 * cosOmega + twoSqrtAalpha);
        b1 = -kPiMultiplier * amplitude * (am1 + ap1 * cosOmega);
        b2 = amplitude * (ap1 + am1 * cosOmega - twoSqrtAalpha);
        a0 = ap1 - am1 * cosOmega + twoSqrtAalpha;
        a1 = kPiMultiplier * (am1 - ap1 * cosOmega);
        a2 = ap1 - am1 * cosOmega - twoSqrtAalpha;
        break;
    }
    case Type::Peaking: {
        b0 = 1.0 + alpha * amplitude;
        b1 = -kPiMultiplier * cosOmega;
        b2 = 1.0 - alpha * amplitude;
        a0 = 1.0 + alpha / amplitude;
        a1 = -kPiMultiplier * cosOmega;
        a2 = 1.0 - alpha / amplitude;
        break;
    }
    }

    m_b0 = b0 / a0;
    m_b1 = b1 / a0;
    m_b2 = b2 / a0;
    m_a1 = a1 / a0;
    m_a2 = a2 / a0;
    reset();
}

/**
 * @brief Resets the filter's internal state (delay lines) to zero.
 */
void BiquadFilter::reset() {
    m_z1Left = 0.0;
    m_z2Left = 0.0;
    m_z1Right = 0.0;
    m_z2Right = 0.0;
}

/**
 * @brief Processes a single channel of audio samples.
 * @param channelSamples Pointer to the array of samples to process.
 * @param sampleCount The number of samples to process.
 */
void BiquadFilter::processChannel(float *channelSamples, size_t sampleCount) {
    double z1 = m_z1Left;
    double z2 = m_z2Left;

    for (std::size_t index = 0; index < sampleCount; ++index) {
        const double input = static_cast<double>(channelSamples[index]);
        const double output = m_b0 * input + z1;
        z1 = m_b1 * input - m_a1 * output + z2;
        z2 = m_b2 * input - m_a2 * output;
        channelSamples[index] = static_cast<float>(output);
    }

    m_z1Left = z1;
    m_z2Left = z2;
}

/**
 * @brief Processes interleaved stereo or mono audio samples through the filter.
 * @param interleavedSamples Vector of audio samples.
 * @param channelCount The number of channels (1 for mono, 2 for stereo).
 */
void BiquadFilter::process(std::vector<float> &interleavedSamples, size_t channelCount) {
    if (channelCount <= 0 || interleavedSamples.empty()) {
        return;
    }

    const size_t frameCount = static_cast<size_t>(interleavedSamples.size()) / channelCount;
    if (channelCount == 1) {
        processChannel(interleavedSamples.data(), frameCount);
        return;
    }

    std::vector<float> leftChannel(static_cast<size_t>(frameCount));
    std::vector<float> rightChannel(static_cast<size_t>(frameCount));
    for (std::size_t frame = 0; frame < frameCount; ++frame) {
        leftChannel[frame] = interleavedSamples[frame * channelCount];
        rightChannel[frame] = interleavedSamples[frame * channelCount + 1];
    }

    const double savedZ1Right = m_z1Right;
    const double savedZ2Right = m_z2Right;
    processChannel(leftChannel.data(), frameCount);
    m_z1Right = savedZ1Right;
    m_z2Right = savedZ2Right;
    processChannel(rightChannel.data(), frameCount);

    for (std::size_t frame = 0; frame < frameCount; ++frame) {
        interleavedSamples[frame * channelCount] = leftChannel[frame];
        interleavedSamples[frame * channelCount + 1] = rightChannel[frame];
    }
}
