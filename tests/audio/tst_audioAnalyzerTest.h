#ifndef TST_AUDIOANALYZERTEST_H
#define TST_AUDIOANALYZERTEST_H

#include <QObject>

class TestAudioAnalyzer : public QObject {
    Q_OBJECT

  private slots:
    void jsonOutputPathReplacesExtension();
    void analyzeRejectsMissingFile();
    void analyzeAndSaveRejectsMissingFile();
};

#endif // TST_AUDIOANALYZERTEST_H
