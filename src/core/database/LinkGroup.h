#ifndef LINKGROUP_H
#define LINKGROUP_H

#include <QString>
#include <QtGlobal>

struct LinkGroup {
    qlonglong id{};
    QString title{};
    qlonglong primarySongId{};
    qlonglong primaryMediaId{};
    QString createdAt{};
};

#endif // LINKGROUP_H
