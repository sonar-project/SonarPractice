#ifndef AUDIOCONFIGPRESET_H
#define AUDIOCONFIGPRESET_H

#include <QString>

struct AudioConfigPreset {
    qlonglong id{};
    qlonglong mediaFileId{};
    QString name{};
    int tempoPercent{100};
    QString eqPresetId = QStringLiteral("flat");
    qint64 regionStartMs{};
    qint64 regionEndMs{};
    bool loopEnabled{false};
};

#endif // AUDIOCONFIGPRESET_H
