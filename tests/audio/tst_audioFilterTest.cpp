#include "tst_audioFilterTest.h"

#include "BiquadFilter.h"
#include "ParametricEq.h"

#include <QTest>

#include <cmath>

void TestAudioFilter::biquadRejectsInvalidChannelCount() {
    BiquadFilter filter;
    filter.configure(BiquadFilter::Type::Peaking, BiquadFilter::Filter{44100.0, 1000.0, 3.0, 0.7});

    std::vector<float> samples = {0.5F, -0.5F};
    const auto before = samples;
    filter.process(samples, 0);
    QCOMPARE(samples, before);

    filter.process(samples, 3);
    QCOMPARE(samples, before);
}

void TestAudioFilter::biquadProcessesMonoWithoutCrash() {
    BiquadFilter filter;
    filter.configure(BiquadFilter::Type::LowShelf, BiquadFilter::Filter{44100.0, 200.0, 6.0, 0.7});

    std::vector<float> samples(128, 0.25F);
    filter.process(samples, 1);
    QVERIFY(std::isfinite(samples.front()));
}

void TestAudioFilter::biquadProcessesStereoWithoutCrash() {
    BiquadFilter filter;
    filter.configure(BiquadFilter::Type::HighShelf, BiquadFilter::Filter{44100.0, 4000.0, -3.0, 0.7});

    std::vector<float> samples(256);
    for (std::size_t frame = 0; frame < 128; ++frame) {
        samples[frame * 2] = 0.3F;
        samples[frame * 2 + 1] = -0.3F;
    }
    filter.process(samples, 2);
    QVERIFY(std::isfinite(samples[1]));
}

void TestAudioFilter::parametricEqPresetIdRoundTrip() {
    QCOMPARE(ParametricEq::presetId(ParametricEq::Preset::Flat), QStringLiteral("flat"));
    QCOMPARE(ParametricEq::presetId(ParametricEq::Preset::ReduceLow), QStringLiteral("reduce_low"));
    QCOMPARE(ParametricEq::presetFromId(QStringLiteral("reduce_high")),
             ParametricEq::Preset::ReduceHigh);
    QCOMPARE(ParametricEq::presetFromId(QStringLiteral("unknown")), ParametricEq::Preset::Flat);
}

void TestAudioFilter::flatPresetLeavesSamplesUnchanged() {
    ParametricEq eq;
    eq.setSampleRate(44100);
    eq.setPreset(ParametricEq::Preset::Flat);

    std::vector<float> samples = {0.1F, 0.2F, 0.3F, 0.4F};
    const auto expected = samples;
    eq.process(samples, 2);
    QCOMPARE(samples, expected);
}

void TestAudioFilter::reduceLowPresetModifiesSamples() {
    ParametricEq eq;
    eq.setSampleRate(44100);
    eq.setPreset(ParametricEq::Preset::ReduceLow);

    std::vector<float> samples(512, 0.5F);
    const auto expected = samples;
    eq.process(samples, 2);
    QVERIFY(samples != expected);
}

QTEST_MAIN(TestAudioFilter)
