#include "tst_managedFileStorageTest.h"

#include "ManagedFileStorage.h"
#include "fnv1a.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

void TestManagedFileStorage::testLinkKeepsOriginalPath() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString sourcePath = tempDir.filePath(QStringLiteral("song.gp3"));
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write("gp");
    source.close();

    StorageParameters params{.fileHash = QStringLiteral("abc123"),
                             .extension = QStringLiteral("gp3"),
                             .sourcePath = sourcePath,
                             .relativeSubPath = {},
                             .managedRoot = tempDir.filePath(QStringLiteral("managed"))

    };

    const ManagedFileResult result = ManagedFileStorage::storeFile(StorageStrategy::Link, params);

    QVERIFY(result.success);
    QCOMPARE(result.storedPath, sourcePath);
    QCOMPARE(result.isManaged, false);
    QVERIFY(QFileInfo::exists(sourcePath));
}

void TestManagedFileStorage::testCopyCreatesManagedFile() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString sourcePath = tempDir.filePath(QStringLiteral("song.gp3"));
    const QString managedRoot = tempDir.filePath(QStringLiteral("managed"));

    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write("gp");
    source.close();

    StorageParameters params{.fileHash = QStringLiteral("abc123"),
                             .extension = QStringLiteral("gp3"),
                             .sourcePath = sourcePath,
                             .relativeSubPath = {},
                             .managedRoot = managedRoot

    };

    const ManagedFileResult result = ManagedFileStorage::storeFile(StorageStrategy::Copy, params);

    QVERIFY(result.success);
    QVERIFY(result.isManaged);
    QCOMPARE(result.relativePath, QStringLiteral("song.gp3"));
    QVERIFY(QFileInfo::exists(result.storedPath));
    QVERIFY(QFileInfo::exists(sourcePath));
}

void TestManagedFileStorage::testCopyUsesIncrementedNameWhenFileExists() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString managedRoot = tempDir.filePath(QStringLiteral("managed"));
    QDir().mkpath(managedRoot);

    const QString existingPath = QDir(managedRoot).filePath(QStringLiteral("song.gp3"));
    QFile existing(existingPath);
    QVERIFY(existing.open(QIODevice::WriteOnly));
    existing.write("existing");
    existing.close();

    const QString sourcePath = tempDir.filePath(QStringLiteral("song.gp3"));
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write("new");
    source.close();

    StorageParameters params{.fileHash = QStringLiteral("abc123"),
                             .extension = QStringLiteral("gp3"),
                             .sourcePath = sourcePath,
                             .relativeSubPath = {},
                             .managedRoot = managedRoot

    };

    const ManagedFileResult result = ManagedFileStorage::storeFile(StorageStrategy::Copy, params);

    QVERIFY(result.success);
    QCOMPARE(result.relativePath, QStringLiteral("song_ver1.gp3"));
    QVERIFY(QFileInfo::exists(result.storedPath));
}

void TestManagedFileStorage::testCopyReusesExistingFileWhenHashMatches() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString managedRoot = tempDir.filePath(QStringLiteral("managed"));
    QDir().mkpath(managedRoot);

    const QString sourcePath = tempDir.filePath(QStringLiteral("song.gp3"));
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write("same content");
    source.close();

    const QString fileHash = FNV1a::calculate(sourcePath);
    QVERIFY(!fileHash.isEmpty());

    const QString existingPath = QDir(managedRoot).filePath(QStringLiteral("song.gp3"));
    QVERIFY(QFile::copy(sourcePath, existingPath));

    StorageParameters params{.fileHash = fileHash,
                             .extension = QStringLiteral("gp3"),
                             .sourcePath = sourcePath,
                             .relativeSubPath = {},
                             .managedRoot = managedRoot};

    const ManagedFileResult result = ManagedFileStorage::storeFile(StorageStrategy::Copy, params);

    QVERIFY(result.success);
    QVERIFY(result.duplicateContent);
    QCOMPARE(result.relativePath, QStringLiteral("song.gp3"));
    QCOMPARE(result.storedPath, existingPath);
    QCOMPARE(QDir(managedRoot).entryList(QDir::Files).size(), 1);
}

void TestManagedFileStorage::testMoveRemovesSourceFile() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString sourcePath = tempDir.filePath(QStringLiteral("song.gp3"));
    const QString managedRoot = tempDir.filePath(QStringLiteral("managed"));

    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write("gp");
    source.close();

    StorageParameters params{.fileHash = QStringLiteral("def456"),
                             .extension = QStringLiteral("gp3"),
                             .sourcePath = sourcePath,
                             .relativeSubPath = {},
                             .managedRoot = managedRoot

    };

    const ManagedFileResult result = ManagedFileStorage::storeFile(StorageStrategy::Move, params);

    QVERIFY(result.success);
    QVERIFY(result.isManaged);
    QVERIFY(QFileInfo::exists(result.storedPath));
    QVERIFY(!QFileInfo::exists(sourcePath));
}

QTEST_MAIN(TestManagedFileStorage)
