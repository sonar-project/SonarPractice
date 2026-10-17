#ifndef NOOPANALYSERMODULE_H
#define NOOPANALYSERMODULE_H

#include "IAnalyserModule.h"

/**
 * @brief Placeholder module that always returns no findings (Phase 0).
 */
class NoOpAnalyserModule final : public IAnalyserModule {
  public:
    [[nodiscard]] QString id() const override;
    [[nodiscard]] bool requiresScore() const override;
    [[nodiscard]] bool requiresReferenceAudio() const override;
    [[nodiscard]] QVector<AnalysisFinding> analyze(const AnalysisSession &session) const override;
};

#endif // NOOPANALYSERMODULE_H
