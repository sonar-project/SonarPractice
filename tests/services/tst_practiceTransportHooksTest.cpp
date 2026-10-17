#include "tst_practiceTransportHooksTest.h"

#include "AnalysisSession.h"
#include "NoOpAnalyserModule.h"
#include "PracticeSessionConfig.h"
#include "PracticeTransport.h"

#include <QtTest>

void TestPracticeTransportHooks::transportRejectsPlayWithoutReference() {
    PracticeTransport transport;
    PracticeSessionConfig config;
    config.kind = PracticeSessionKind::Probe;
    transport.configure(config);

    QVERIFY(!transport.play());
    QCOMPARE(transport.state(), PracticeTransport::State::Idle);
}

void TestPracticeTransportHooks::transportPlayPauseStopWithReference() {
    PracticeTransport transport;
    PracticeSessionConfig config;
    config.guitarProMediaId = 42;
    config.kind = PracticeSessionKind::Practice;
    transport.configure(config);

    QVERIFY(transport.play());
    QCOMPARE(transport.state(), PracticeTransport::State::Playing);

    transport.pause();
    QCOMPARE(transport.state(), PracticeTransport::State::Paused);

    transport.stop();
    QCOMPARE(transport.state(), PracticeTransport::State::Idle);
}

void TestPracticeTransportHooks::sessionConfigDefaultsAndFlags() {
    const PracticeSessionConfig config;
    QCOMPARE(config.kind, PracticeSessionKind::Practice);
    QCOMPARE(config.tempoPercent, PracticeConstants::kDefaultTempoPercent);
    QCOMPARE(config.targetReps, PracticeConstants::kMaxSuccessfulStreak);
    QVERIFY(!config.hasAnyReference());

    PracticeSessionConfig withAudio;
    withAudio.audioMediaId = 7;
    QVERIFY(withAudio.hasAudio());
    QVERIFY(withAudio.hasAnyReference());
    QVERIFY(!withAudio.hasGuitarPro());
}

void TestPracticeTransportHooks::noOpAnalyserReturnsEmptyFindings() {
    NoOpAnalyserModule module;
    QCOMPARE(module.id(), QStringLiteral("noop"));
    QVERIFY(!module.requiresScore());
    QVERIFY(!module.requiresReferenceAudio());

    AnalysisSession session;
    session.config.audioMediaId = 1;
    QCOMPARE(module.analyze(session).size(), 0);
}

QTEST_MAIN(TestPracticeTransportHooks)
