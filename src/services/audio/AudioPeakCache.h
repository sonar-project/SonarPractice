#ifndef AUDIOPEAKCACHE_H
#define AUDIOPEAKCACHE_H

#include <QFileInfo>
#include <QString>
#include <QVector>

struct AudioPeakCacheEntry {
    QVector<float> peaks;
    qint64 durationMs{0};
    int sampleRateHz{0};
    int channelCount{0};
};

class AudioPeakCache {
  public:
    [[nodiscard]] static QString cachePathFor(const QString &sourceFilePath);

    [[nodiscard]] static bool read(const QString &sourceFilePath, AudioPeakCacheEntry &entry);

    [[nodiscard]] static bool write(const QString &sourceFilePath, const AudioPeakCacheEntry &entry);

  private:
    [[nodiscard]] static bool isSourceValid(const QFileInfo &sourceInfo, qint64 storedSize,
                                            qint64 storedMtimeSecs);
};

#endif // AUDIOPEAKCACHE_H
