#ifndef PARAMETRICEQ_H
#define PARAMETRICEQ_H

#include "BiquadFilter.h"

#include <QString>
#include <cstdint>
#include <vector>

class ParametricEq {
  public:
    enum class Preset : std::uint8_t {
        Flat,
        ReduceLow,
        ReduceHigh,
        MidFocus,
    };

    void setSampleRate(double sampleRateHz);
    void setPreset(Preset preset);
    void setPresetById(const QString &presetId);
    void process(std::vector<float> &interleavedSamples, int channelCount);

    [[nodiscard]] static QString presetId(Preset preset);
    [[nodiscard]] static Preset presetFromId(const QString &presetId);

  private:
    void rebuildFilters();

    double m_sampleRateHz{44100};
    Preset m_preset{Preset::Flat};
    BiquadFilter m_lowShelf{};
    BiquadFilter m_midPeak{};
    BiquadFilter m_highShelf{};
};

#endif // PARAMETRICEQ_H
