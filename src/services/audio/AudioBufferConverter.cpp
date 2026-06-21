#include "AudioBufferConverter.h"

#include <QAudioFormat>
#include <cstring>
#include <limits>
#include <optional>

namespace {

    constexpr float kInt16ToFloat = 1.0F / 32768.0F;
    constexpr float kInt32ToFloat = 1.0F / 2147483648.0F;
    constexpr float kUInt8ToFloat = 1.0F / 128.0F;
    constexpr float kUInt8Bias = 128.0F;

    /**
    * @brief Checks whether multiplying frame and channel counts is safe.
    * @param[in] frameCount  Number of frames.
    * @param[in] channelCount Number of channels.
    * @return true if multiplication is safe, false otherwise.
    */
    [[nodiscard]] bool isSafeFrameChannelProduct(std::size_t frameCount, std::size_t channelCount) {
        if (frameCount == 0 || channelCount == 0) {
            return false;
        }
        return frameCount <= std::numeric_limits<std::size_t>::max() / channelCount;
    }

    /**
    * @brief Deinterleaves float source into separate channel buffers.
    * @param[in] source  Pointer to interleaved float data.
    * @param[in] param   Conversion parameters (frame & channel count).
    * @param[out] storage 2‑D vector to receive deinterleaved samples.
    */
    void deinterleaveFloatSource(const float *source,
                                 DeinterleavedFloatBuffer::ConvertParameter param,
                                 std::vector<std::vector<float>> &storage) {
        if (param.channelCount == 1) {
            std::memcpy(storage[0].data(), source, param.frameCount * sizeof(float));
            return;
        }

        for (std::size_t channel = 0; channel < param.channelCount; ++channel) {
            float *const destination = storage[channel].data();
            for (std::size_t frame = 0; frame < param.frameCount; ++frame) {
                destination[frame] = source[(frame * param.channelCount) + channel];
            }
        }
    }

    /**
    * @brief Deinterleaves int16 source into separate channel buffers.
    * @param[in] source  Pointer to interleaved int16 data.
    * @param[in] param   Conversion parameters (frame & channel count).
    * @param[out] storage 2‑D vector to receive deinterleaved samples.
    */
    void deinterleaveInt16Source(const qint16 *source,
                                 DeinterleavedFloatBuffer::ConvertParameter param,
                                 std::vector<std::vector<float>> &storage) {
        for (std::size_t channel = 0; channel < param.channelCount; ++channel) {
            float *const destination = storage[channel].data();
            for (std::size_t frame = 0; frame < param.frameCount; ++frame) {
                destination[frame] =
                    static_cast<float>(source[(frame * param.channelCount) + channel]) *
                    kInt16ToFloat;
            }
        }
    }

    /**
    * @brief Deinterleaves int32 source into separate channel buffers.
    * @param[in] source  Pointer to interleaved int32 data.
    * @param[in] param   Conversion parameters (frame & channel count).
    * @param[out] storage 2‑D vector to receive deinterleaved samples.
    */
    void deinterleaveInt32Source(const qint32 *source,
                                 DeinterleavedFloatBuffer::ConvertParameter param,
                                 std::vector<std::vector<float>> &storage) {
        for (std::size_t channel = 0; channel < param.channelCount; ++channel) {
            float *const destination = storage[channel].data();
            for (std::size_t frame = 0; frame < param.frameCount; ++frame) {
                destination[frame] =
                    static_cast<float>(source[(frame * param.channelCount) + channel]) *
                    kInt32ToFloat;
            }
        }
    }

    /**
    * @brief Deinterleaves uint8 source into separate channel buffers.
    * @param[in] source  Pointer to interleaved uint8 data.
    * @param[in] param   Conversion parameters (frame & channel count).
    * @param[out] storage 2‑D vector to receive deinterleaved samples.
    */
    void deinterleaveUInt8Source(const uchar *source,
                                 DeinterleavedFloatBuffer::ConvertParameter param,
                                 std::vector<std::vector<float>> &storage) {
        for (std::size_t channel = 0; channel < param.channelCount; ++channel) {
            float *const destination = storage[channel].data();
            for (std::size_t frame = 0; frame < param.frameCount; ++frame) {
                destination[frame] =
                    (static_cast<float>(source[(frame * param.channelCount) + channel]) -
                     kUInt8Bias) *
                    kUInt8ToFloat;
            }
        }
    }

} // namespace

/**
* @brief Allocates internal storage for the buffer.
* @param[in] param Conversion parameters (frame & channel count).
*/
void DeinterleavedFloatBuffer::allocate(DeinterleavedFloatBuffer::ConvertParameter param) {
    if (param.frameCount == 0 || param.channelCount == 0) {
        return;
    }
    m_frameCount = param.frameCount;
    m_storage.resize(param.channelCount);

    m_channelPointers.resize(param.channelCount);

    for (std::size_t channel = 0; channel < param.channelCount; ++channel) {
        m_storage[channel].resize(param.frameCount);
        m_channelPointers[channel] = m_storage[channel].data();
    }
}

/**
* @brief Creates an empty deinterleaved float buffer.
* @param[in] param Conversion parameters (frame & channel count).
* @return A new, allocated buffer if parameters are valid; otherwise an empty buffer.
*/
DeinterleavedFloatBuffer DeinterleavedFloatBuffer::createEmpty(ConvertParameter param) {
    DeinterleavedFloatBuffer result;
    if (param.channelCount > 0 && param.frameCount > 0 &&
        isSafeFrameChannelProduct(param.frameCount, param.channelCount)) {
        result.allocate(param);
    }
    return result;
}

/**
* @brief Creates a deinterleaved float buffer from a QAudioBuffer.
* @param[in] buffer Source audio buffer.
* @return Optional buffer; std::nullopt if source invalid or unsupported format.
*/
std::optional<DeinterleavedFloatBuffer>
DeinterleavedFloatBuffer::fromQAudioBuffer(const QAudioBuffer &buffer) {
    if (!buffer.isValid()) {
        return std::nullopt;
    }

    const QAudioFormat format = buffer.format();
    const int rawChannelCount = format.channelCount();
    const qsizetype rawFrameCount = buffer.frameCount();
    if (rawChannelCount <= 0 || rawFrameCount <= 0) {
        return std::nullopt;
    }

    const auto channelCount = static_cast<std::size_t>(rawChannelCount);
    const auto frameCount = static_cast<std::size_t>(rawFrameCount);
    if (!isSafeFrameChannelProduct(frameCount, channelCount)) {
        return std::nullopt;
    }

    const auto *const byteData = buffer.constData<char>();
    if (byteData == nullptr) {
        return std::nullopt;
    }

    DeinterleavedFloatBuffer result;

    DeinterleavedFloatBuffer::ConvertParameter param{.frameCount = frameCount,
                                                     .channelCount = channelCount};

    result.allocate(param);

    switch (format.sampleFormat()) {
    case QAudioFormat::Float: {
        const auto *source = reinterpret_cast<const float *>(byteData);
        deinterleaveFloatSource(source, param, result.m_storage);
        break;
    }
    case QAudioFormat::Int16: {
        const auto *source = reinterpret_cast<const qint16 *>(byteData);
        deinterleaveInt16Source(source, param, result.m_storage);
        break;
    }
    case QAudioFormat::Int32: {
        const auto *source = reinterpret_cast<const qint32 *>(byteData);
        deinterleaveInt32Source(source, param, result.m_storage);
        break;
    }
    case QAudioFormat::UInt8: {
        const auto *source = reinterpret_cast<const uchar *>(byteData);
        deinterleaveUInt8Source(source, param, result.m_storage);
        break;
    }
    default: {
        for (std::size_t frame = 0; frame < frameCount; ++frame) {
            const auto frameOffset =
                static_cast<size_t>(frame) * static_cast<size_t>(format.bytesPerFrame());
            const auto *frameData = byteData + frameOffset;
            for (std::size_t channel = 0; channel < channelCount; ++channel) {
                const auto channelByteOffset =
                    static_cast<size_t>(channel) * static_cast<size_t>(format.bytesPerSample());
                result.m_storage[channel][frame] =
                    static_cast<float>(format.normalizedSampleValue(frameData + channelByteOffset));
            }
        }
        break;
    }
    }

    return result;
}

/**
* @brief Converts interleaved float data into a deinterleaved buffer.
* @param[in] interleaved  Span of interleaved samples.
* @param[in] channelCount Number of channels.
* @return The resulting deinterleaved buffer.
*/
DeinterleavedFloatBuffer
DeinterleavedFloatBuffer::fromInterleaved(std::span<const float> interleaved,
                                          std::size_t channelCount) {
    DeinterleavedFloatBuffer result;
    if (interleaved.empty() || channelCount == 0) {
        return result;
    }

    const std::size_t frameCount = interleaved.size() / channelCount;
    if (frameCount == 0) {
        return result;
    }

    DeinterleavedFloatBuffer::ConvertParameter param{.frameCount = frameCount,
                                                     .channelCount = channelCount};

    result.allocate(param);

    deinterleaveFloatSource(interleaved.data(), param, result.m_storage);
    return result;
}

/**
* @brief Returns a mutable view of a channel.
* @param[in] index Channel index.
* @return Span of samples; empty if index out of range.
*/
std::span<float> DeinterleavedFloatBuffer::channel(const std::size_t index) {
    if (index >= m_storage.size()) {
        return {};
    }
    return {m_storage[index].data(), m_frameCount};
}

/**
* @brief Returns a const view of a channel.
* @param[in] index Channel index.
* @return Span of samples; empty if index out of range.
*/
std::span<const float> DeinterleavedFloatBuffer::channel(const std::size_t index) const {
    if (index >= m_storage.size()) {
        return {};
    }
    return {m_storage[index].data(), m_frameCount};
}

/**
* @brief Returns a mutable view of a channel.
* @param[in] index Channel index.
* @return Span of samples; empty if index out of range.
*/
void DeinterleavedFloatBuffer::interleaveInto(std::span<const std::span<const float>> channels,
                                              std::vector<float> &outInterleaved) {
    if (channels.empty()) {
        outInterleaved.clear();
        return;
    }

    const std::size_t channelCount = channels.size();
    const std::size_t frameCount = channels[0].size();
    if (frameCount == 0 || !isSafeFrameChannelProduct(frameCount, channelCount)) {
        outInterleaved.clear();
        return;
    }

    const std::size_t totalSamples = frameCount * channelCount;
    outInterleaved.resize(totalSamples);

    if (channelCount == 1) {
        std::memcpy(outInterleaved.data(), channels[0].data(), frameCount * sizeof(float));
        return;
    }

    for (std::size_t frame = 0; frame < frameCount; ++frame) {
        for (std::size_t channel = 0; channel < channelCount; ++channel) {
            outInterleaved[(frame * channelCount) + channel] = channels[channel][frame];
        }
    }
}

/**
* @brief Interleaves data from another buffer into a vector.
* @param[in] source Source buffer.
* @param[out] outInterleaved  Vector receiving interleaved samples.
*/
void DeinterleavedFloatBuffer::interleaveInto(const DeinterleavedFloatBuffer &source,
                                              std::vector<float> &outInterleaved) {
    std::vector<std::span<const float>> channelViews;
    channelViews.reserve(source.channelCount());
    for (std::size_t channel = 0; channel < source.channelCount(); ++channel) {
        channelViews.emplace_back(source.channel(channel));
    }
    interleaveInto(channelViews, outInterleaved);
}

/**
* @brief Returns a vector containing interleaved samples of the buffer.
* @return Interleaved sample vector.
*/
std::vector<float> DeinterleavedFloatBuffer::toInterleaved() const {
    std::vector<float> interleaved;
    interleaveInto(*this, interleaved);
    return interleaved;
}
