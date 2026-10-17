#include "tst_audioBufferConverterTest.h"

#include "AudioBufferConverter.h"
#include "AudioTestHelpers.h"

#include <QTest>

#include <limits>

void TestAudioBufferConverter::createEmptyRejectsZeroDimensions() {
    DeinterleavedFloatBuffer::ConvertParameter param{.frameCount = 100, .channelCount = 0};
    const auto noChannels = DeinterleavedFloatBuffer::createEmpty(param);
    QCOMPARE(noChannels.channelCount(), 0u);
    QCOMPARE(noChannels.frameCount(), 0u);

    const auto noFrames = DeinterleavedFloatBuffer::createEmpty(DeinterleavedFloatBuffer::ConvertParameter{2, 0});
    QCOMPARE(noFrames.channelCount(), 0u);
    QCOMPARE(noFrames.frameCount(), 0u);
}

void TestAudioBufferConverter::fromInterleavedRejectsEmptyInput() {
    const std::vector<float> empty;
    const auto result = DeinterleavedFloatBuffer::fromInterleaved(empty, 2);
    QCOMPARE(result.channelCount(), 0u);
    QCOMPARE(result.frameCount(), 0u);

    const std::vector<float> mono{0.5F};
    const auto noChannels = DeinterleavedFloatBuffer::fromInterleaved(mono, 0);
    QCOMPARE(noChannels.channelCount(), 0u);
}

void TestAudioBufferConverter::fromInterleavedRoundTripPreservesSamples() {
    const std::vector<float> source = {0.1F, 0.2F, 0.3F, 0.4F, 0.5F, 0.6F};
    const auto deinterleaved = DeinterleavedFloatBuffer::fromInterleaved(source, 2);
    QCOMPARE(deinterleaved.channelCount(), 2u);
    QCOMPARE(deinterleaved.frameCount(), 3u);
    QCOMPARE(deinterleaved.channel(0)[0], 0.1F);
    QCOMPARE(deinterleaved.channel(1)[2], 0.6F);

    const std::vector<float> roundTrip = deinterleaved.toInterleaved();
    QCOMPARE(roundTrip, source);
}

void TestAudioBufferConverter::fromQAudioBufferRejectsInvalidBuffer() {
    const QAudioBuffer invalid;
    const auto result = DeinterleavedFloatBuffer::fromQAudioBuffer(invalid);
    QVERIFY(!result.has_value());
}

void TestAudioBufferConverter::fromQAudioBufferConvertsFloatStereo() {
    const std::vector<float> source = {1.0F, -1.0F, 0.5F, -0.5F};
    const QAudioBuffer buffer = AudioTestHelpers::makeFloatBuffer(source, 2);
    const auto result = DeinterleavedFloatBuffer::fromQAudioBuffer(buffer);
    QVERIFY(result.has_value());
    QCOMPARE(result->channelCount(), 2u);
    QCOMPARE(result->frameCount(), 2u);
    QCOMPARE(result->channel(0)[0], 1.0F);
    QCOMPARE(result->channel(1)[1], -0.5F);
}

void TestAudioBufferConverter::fromQAudioBufferConvertsInt16Mono() {
    const std::vector<qint16> source = {16384, -16384};
    const QAudioBuffer buffer = AudioTestHelpers::makeInt16Buffer(source, 1);
    const auto result = DeinterleavedFloatBuffer::fromQAudioBuffer(buffer);
    QVERIFY(result.has_value());
    QCOMPARE(result->channelCount(), 1u);
    QCOMPARE(result->frameCount(), 2u);
    QVERIFY(qAbs(result->channel(0)[0] - 0.5F) < 0.01F);
    QVERIFY(qAbs(result->channel(0)[1] + 0.5F) < 0.01F);
}

void TestAudioBufferConverter::interleaveIntoRejectsUnsafeDimensions() {
    std::vector<float> out;
    const std::vector<std::span<const float>> emptyChannels;
    DeinterleavedFloatBuffer::interleaveInto(emptyChannels, out);
    QVERIFY(out.empty());

    const std::vector<float> left(1, 0.5F);
    const std::vector<std::span<const float>> channels = {left};
    DeinterleavedFloatBuffer::interleaveInto(channels, out);
    QCOMPARE(out.size(), 1u);

    const std::vector<float> channelData(1, 0.0F);
    const auto unsafeFrameCount = std::numeric_limits<std::size_t>::max() / 2 + 1;
    const std::span<const float> leftUnsafe(channelData.data(), unsafeFrameCount);
    const std::span<const float> rightUnsafe(channelData.data(), unsafeFrameCount);
    const std::vector<std::span<const float>> unsafeChannels = {leftUnsafe, rightUnsafe};
    DeinterleavedFloatBuffer::interleaveInto(unsafeChannels, out);
    QVERIFY(out.empty());
}

void TestAudioBufferConverter::channelAccessOutOfRangeReturnsEmptySpan() {
    const std::vector<float> source = {0.1F, 0.2F};
    const auto buffer = DeinterleavedFloatBuffer::fromInterleaved(source, 2);
    QVERIFY(buffer.channel(99).empty());
}

QTEST_MAIN(TestAudioBufferConverter)
