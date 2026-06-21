#ifndef SONG_H
#define SONG_H

#include "interfaces/IDatabaseConnection.h"
#include <QtGlobal>

class QString;

struct Song {
    qlonglong id{};
    QString title{};
    int baseBpm{};
    qlonglong artistId{};
    qlonglong tuningId{};
    QString tuningName{};
};

#endif // SONG_H
