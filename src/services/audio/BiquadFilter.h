#ifndef BIQUADFILTER_H
#define BIQUADFILTER_H

#include <cstdint>
#include <vector>

class BiquadFilter {
  public:
    enum class Type : std::uint8_t { LowShelf, HighShelf, Peaking };
    struct Filter {
        double sampleRateHz{44100.0};
        double frequencyHz{441}; // Cutoff
        double gainDb{2};        // Gain in decib
        double qFactor{0.7};     // Quality factor
    };

    void configure(Type type, Filter filter);
    void reset();
    void process(std::vector<float> &interleavedSamples, size_t channelCount);

  private:
    void processChannel(float *channelSamples, size_t sampleCount);

    Type m_type{Type::Peaking};
    double m_sampleRateHz{44100.0};
    double m_frequencyHz{1000.0};
    double m_gainDb{};
    double m_qFactor{0.707};
    double m_b0{1.0};
    double m_b1{};
    double m_b2{};
    double m_a1{};
    double m_a2{};
    double m_z1Left{};
    double m_z2Left{};
    double m_z1Right{};
    double m_z2Right{};
};

#endif // BIQUADFILTER_H
