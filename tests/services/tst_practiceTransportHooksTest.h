#ifndef TST_PRACTICETRANSPORTHOOKSTEST_H
#define TST_PRACTICETRANSPORTHOOKSTEST_H

#include <QObject>

/**
 * @brief Smoke tests for Phase-0 practice transport / analyser hooks.
 */
class TestPracticeTransportHooks : public QObject {
    Q_OBJECT

  private slots:
    void transportRejectsPlayWithoutReference();
    void transportPlayPauseStopWithReference();
    void sessionConfigDefaultsAndFlags();
    void noOpAnalyserReturnsEmptyFindings();
};

#endif // TST_PRACTICETRANSPORTHOOKSTEST_H
