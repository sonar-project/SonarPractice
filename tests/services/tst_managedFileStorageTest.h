#ifndef TST_MANAGEDFILESTORAGETEST_H
#define TST_MANAGEDFILESTORAGETEST_H

#include <QObject>
#include <QTest>

class TestManagedFileStorage : public QObject {
    Q_OBJECT

  private slots:
    void testLinkKeepsOriginalPath();
    void testCopyCreatesManagedFile();
    void testCopyUsesIncrementedNameWhenFileExists();
    void testMoveRemovesSourceFile();
};

#endif // TST_MANAGEDFILESTORAGETEST_H
