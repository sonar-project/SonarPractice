#ifndef TST_MEDIASTREAMCLASSIFICATIONTEST_H
#define TST_MEDIASTREAMCLASSIFICATIONTEST_H

#include <QObject>

class TestMediaStreamClassification : public QObject {
    Q_OBJECT

  private slots:
    void testClassifyAudioOnlyContainer();
    void testClassifyVideoContainer();
    void testClassifyVideoOnlyContainer();
    void testClassifyNeitherStreamUsesDefault();
    void testIsContainerExtension();
};

#endif // TST_MEDIASTREAMCLASSIFICATIONTEST_H
