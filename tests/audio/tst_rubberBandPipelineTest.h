#ifndef TST_RUBBERBANDPIPELINETEST_H
#define TST_RUBBERBANDPIPELINETEST_H

#include <QObject>

class TestRubberBandPipeline : public QObject {
    Q_OBJECT

  private slots:
    void stretchRejectsInvalidInput();
    void stretchChangesFrameCount();
    void unityTempoRatioPassesThrough();
    void stretchSupportsCancellation();
};

#endif // TST_RUBBERBANDPIPELINETEST_H
