#ifndef ARTIST_H
#define ARTIST_H

#include <QString>
#include <QtGlobal>

struct Artist {
    qlonglong id{};
    QString name{};
};

#endif // ARTIST_H
