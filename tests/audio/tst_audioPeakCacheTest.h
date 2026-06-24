#ifndef TST_AUDIOPEAKCACHETEST_H
#define TST_AUDIOPEAKCACHETEST_H

#include <QObject>

class TestAudioPeakCache : public QObject {
    Q_OBJECT

  private slots:
    void roundTripWritesAndReadsPeaks();
    void rejectsStaleCacheWhenSourceChanges();
};

#endif // TST_AUDIOPEAKCACHETEST_H
