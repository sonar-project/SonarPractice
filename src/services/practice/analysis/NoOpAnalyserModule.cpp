/**
 * @file NoOpAnalyserModule.cpp
 * @brief Empty analyser used until TimingModule lands (Phase 5).
 */

#include "NoOpAnalyserModule.h"

QString NoOpAnalyserModule::id() const { return QStringLiteral("noop"); }

bool NoOpAnalyserModule::requiresScore() const { return false; }

bool NoOpAnalyserModule::requiresReferenceAudio() const { return false; }

QVector<AnalysisFinding> NoOpAnalyserModule::analyze(const AnalysisSession &session) const {
    Q_UNUSED(session);
    return {};
}
