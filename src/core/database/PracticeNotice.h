#ifndef PRACTICENOTICE_H
#define PRACTICENOTICE_H

#include <QDate>
#include <QtGlobal>

class QString;

struct PracticeNotice {
    qlonglong id{};
    qlonglong songId{};
    QDate noteDate{};
    QString content{};
};

#endif // PRACTICENOTICE_H
