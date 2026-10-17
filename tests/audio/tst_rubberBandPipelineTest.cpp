#include "tst_rubberBandPipelineTest.h"

#include "AudioConstants.h"
#include "RubberBandPipeline.h"

#include <QTest>

void TestRubberBandPipeline::stretchRejectsInvalidInput() {
    std::vector<float> source;
    std::vector<float> output;
    QString errorMessage;
    RubberBandPipeline pipeline;

    QVERIFY(!pipeline.stretch(source, 0, 0, 1.0, output, errorMessage));
    QVERIFY(!errorMessage.isEmpty());

    source.assign(4, 0.1F);
    QVERIFY(!pipeline.stretch(source, 2, 44100, 0.0, output, errorMessage));
    QVERIFY(!errorMessage.isEmpty());
}

void TestRubberBandPipeline::stretchChangesFrameCount() {
    std::vector<float> source(44100 * 2, 0.1F);
    std::vector<float> output;
    QString errorMessage;
    RubberBandPipeline pipeline;

    const double timeRatio = AudioConstants::rubberBandTimeRatioFromTempoPercent(80);
    QVERIFY(pipeline.stretch(source, 2, 44100, timeRatio, output, errorMessage));
    QVERIFY(errorMessage.isEmpty());
    QVERIFY(static_cast<int>(output.size()) > static_cast<int>(source.size()));
}

void TestRubberBandPipeline::unityTempoRatioPassesThrough() {
    const std::vector<float> source = {0.1F, 0.2F, 0.3F, 0.4F};
    std::vector<float> output;
    QString errorMessage;
    RubberBandPipeline pipeline;

    QVERIFY(pipeline.stretch(source, 2, 44100, 1.0, output, errorMessage));
    QCOMPARE(output, source);
}

void TestRubberBandPipeline::stretchSupportsCancellation() {
    std::vector<float> source(44100 * 2, 0.1F);
    std::vector<float> output;
    QString errorMessage;
    RubberBandPipeline pipeline;

    bool cancelled = false;
    const auto isCancelled = [&]() { return cancelled; };

    cancelled = true;
    QVERIFY(!pipeline.stretch(source, 2, 44100, 1.5, output, errorMessage, {}, isCancelled));
    QCOMPARE(errorMessage, QStringLiteral("Cancelled."));
}

QTEST_MAIN(TestRubberBandPipeline)
