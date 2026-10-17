#ifndef AUDIOTESTHELPERS_H
#define AUDIOTESTHELPERS_H

#include <QAudioBuffer>
#include <QAudioFormat>
#include <QByteArray>

#include <vector>

namespace AudioTestHelpers {

inline QAudioFormat makeFormat(int sampleRate, int channelCount,
                               QAudioFormat::SampleFormat sampleFormat) {
    QAudioFormat format;
    format.setSampleRate(sampleRate);
    format.setChannelCount(channelCount);
    format.setSampleFormat(sampleFormat);
    format.setChannelConfig(QAudioFormat::defaultChannelConfigForChannelCount(channelCount));
    return format;
}

inline QAudioBuffer makeFloatBuffer(const std::vector<float> &interleaved, int channelCount,
                                    int sampleRate = 44100) {
    const auto format = makeFormat(sampleRate, channelCount, QAudioFormat::Float);
    QByteArray data(reinterpret_cast<const char *>(interleaved.data()),
                    static_cast<int>(interleaved.size() * sizeof(float)));
    return QAudioBuffer(data, format);
}

inline QAudioBuffer makeInt16Buffer(const std::vector<qint16> &interleaved, int channelCount,
                                    int sampleRate = 44100) {
    const auto format = makeFormat(sampleRate, channelCount, QAudioFormat::Int16);
    QByteArray data(reinterpret_cast<const char *>(interleaved.data()),
                    static_cast<int>(interleaved.size() * sizeof(qint16)));
    return QAudioBuffer(data, format);
}

} // namespace AudioTestHelpers

#endif // AUDIOTESTHELPERS_H
