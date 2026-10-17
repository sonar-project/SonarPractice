#ifndef IANALYSERMODULE_H
#define IANALYSERMODULE_H

#include "AnalysisFinding.h"
#include "AnalysisSession.h"

#include <QString>
#include <QVector>

/**
 * @brief Pluggable analyser: session in, findings out (no UI / reminder logic).
 *
 * Modules declare requirements; callers skip modules when references are missing.
 */
class IAnalyserModule {
  public:
    virtual ~IAnalyserModule() = default;

    [[nodiscard]] virtual QString id() const = 0;
    [[nodiscard]] virtual bool requiresScore() const = 0;
    [[nodiscard]] virtual bool requiresReferenceAudio() const = 0;
    [[nodiscard]] virtual QVector<AnalysisFinding> analyze(const AnalysisSession &session) const = 0;
};

#endif // IANALYSERMODULE_H
