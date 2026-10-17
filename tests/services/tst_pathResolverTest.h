#ifndef TST_PATHRESOLVERTEST_H
#define TST_PATHRESOLVERTEST_H

#include <QObject>

class TestPathResolver : public QObject {
    Q_OBJECT

  private slots:
    void testResolveLinkedLocalPath();
    void testResolveManagedPath();
    void testResolveVirtualUrlUnchanged();
};

#endif // TST_PATHRESOLVERTEST_H
