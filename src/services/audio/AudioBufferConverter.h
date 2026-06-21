#ifndef AUDIOBUFFERCONVERTER_H
#define AUDIOBUFFERCONVERTER_H

#include <QAudioBuffer>

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

/**
 * Owns deinterleaved float PCM suitable for RubberBand (float* const* API).
 *
 * Storage layout: one contiguous std::vector<float> per channel.
 * channelData() returns a stable pointer array valid for the object's lifetime.
 */
class DeinterleavedFloatBuffer {
  public:
    struct ConvertParameter{
        std::size_t frameCount;
        std::size_t channelCount;
    };

    [[nodiscard]] static std::optional<DeinterleavedFloatBuffer>
    fromQAudioBuffer(const QAudioBuffer &buffer);

    [[nodiscard]] static DeinterleavedFloatBuffer
    fromInterleaved(std::span<const float> interleaved, std::size_t channelCount);

    [[nodiscard]] static DeinterleavedFloatBuffer createEmpty(ConvertParameter param);

    [[nodiscard]] std::size_t frameCount() const noexcept { return m_frameCount; }
    [[nodiscard]] std::size_t channelCount() const noexcept { return m_channelPointers.size(); }

    [[nodiscard]] float *const *writableChannelData() noexcept { return m_channelPointers.data(); }

    [[nodiscard]] const float *const *channelData() const noexcept {
        return reinterpret_cast<const float *const *>(m_channelPointers.data());
    }

    [[nodiscard]] std::span<float> channel(std::size_t index);
    [[nodiscard]] std::span<const float> channel(std::size_t index) const;

    [[nodiscard]] std::vector<float> toInterleaved() const;

    static void interleaveInto(std::span<const std::span<const float>> channels,
                               std::vector<float> &outInterleaved);

    static void interleaveInto(const DeinterleavedFloatBuffer &source,
                               std::vector<float> &outInterleaved);

  private:
    void allocate(DeinterleavedFloatBuffer::ConvertParameter param);

    std::vector<std::vector<float>> m_storage{};
    std::vector<float *> m_channelPointers{};
    std::size_t m_frameCount{};
};

#endif // AUDIOBUFFERCONVERTER_H
