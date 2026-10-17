#include "tst_pathResolverTest.h"

#include "MediaFile.h"
#include "PathResolver.h"

#include <QTest>

void TestPathResolver::testResolveLinkedLocalPath() {
    PathResolver resolver(QStringLiteral("/app/storage/songs"));

    MediaFile file;
    file.filePath = QStringLiteral("/home/user/Music/tab.gp5");
    file.sourceType = MediaSourceType::Local;
    file.isManaged = false;

    QCOMPARE(resolver.resolve(file), file.filePath);
}

void TestPathResolver::testResolveManagedPath() {
    PathResolver resolver(QStringLiteral("/app/storage/songs"));

    MediaFile file;
    file.filePath = QStringLiteral("riff.gp5");
    file.sourceType = MediaSourceType::Local;
    file.isManaged = true;

    QCOMPARE(resolver.resolve(file), QStringLiteral("/app/storage/songs/riff.gp5"));
}

void TestPathResolver::testResolveVirtualUrlUnchanged() {
    PathResolver resolver(QStringLiteral("/app/storage/songs"));

    MediaFile file;
    file.filePath = QStringLiteral("https://example.com/course/legato");
    file.sourceType = MediaSourceType::Url;
    file.isManaged = false;

    QCOMPARE(resolver.resolve(file), file.filePath);
}

QTEST_MAIN(TestPathResolver)
