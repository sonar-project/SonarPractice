#ifndef TST_AUDIOFILTERTEST_H
#define TST_AUDIOFILTERTEST_H

#include <QObject>

class TestAudioFilter : public QObject {
    Q_OBJECT

  private slots:
    void biquadRejectsInvalidChannelCount();
    void biquadProcessesMonoWithoutCrash();
    void biquadProcessesStereoWithoutCrash();
    void parametricEqPresetIdRoundTrip();
    void flatPresetLeavesSamplesUnchanged();
    void reduceLowPresetModifiesSamples();
};

#endif // TST_AUDIOFILTERTEST_H
