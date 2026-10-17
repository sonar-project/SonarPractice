#ifndef TST_IMPORTORGANIZERSESSIONTEST_H
#define TST_IMPORTORGANIZERSESSIONTEST_H

#include <QObject>

class TestImportOrganizerSession : public QObject {
    Q_OBJECT

  private slots:
    void testScanAndMapSingleFileToFolder();
    void testMapFolderPreservesStructure();
    void testMapContentsOnlyStripsWrapper();
    void testBatchDuplicateChecksOnlyOne();
    void testAlreadyInDatabaseHiddenFromAllFilter();
    void testUnmapRestoresSource();
    void testUsesOrganizerFalseForLink();
    void testImportEntriesUsesPlannedRelativePath();
};

#endif // TST_IMPORTORGANIZERSESSIONTEST_H
