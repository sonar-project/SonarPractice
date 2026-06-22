#ifndef APPINFO_H
#define APPINFO_H

#include <QObject>
#include <QString>
#include <QtGlobal>

class AppInfo : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    Q_PROPERTY(QString compiler READ compiler CONSTANT)
    Q_PROPERTY(QString buildDate READ buildDate CONSTANT)

  public:
    explicit AppInfo(QObject *parent = nullptr) : QObject(parent) {}

    QString version() const { return QStringLiteral(APP_VERSION); }
    QString qtVersion() const { return QStringLiteral(QT_VERSION_STR); }
    QString compiler() const { return QStringLiteral(COMPILER_INFO); }
    QString buildDate() const { return QStringLiteral(BUILD_DATE); }
};

#endif // APPINFO_H
