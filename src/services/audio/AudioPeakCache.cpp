#include "AudioPeakCache.h"

#include "AudioConstants.h"

#include <QDataStream>
#include <QFile>

namespace {

    constexpr quint32 kPeakCacheMagic = 0x4B505053; // "SPPK" little-endian
    constexpr quint32 kPeakCacheVersion = 1;

} // namespace

QString AudioPeakCache::cachePathFor(const QString &sourceFilePath) {
    return sourceFilePath + QStringLiteral(".peak");
}

bool AudioPeakCache::isSourceValid(const QFileInfo &sourceInfo, qint64 storedSize,
                                   qint64 storedMtimeSecs) {
    if (!sourceInfo.exists() || !sourceInfo.isFile()) {
        return false;
    }
    return sourceInfo.size() == storedSize &&
           sourceInfo.lastModified().toSecsSinceEpoch() == storedMtimeSecs;
}

bool AudioPeakCache::read(const QString &sourceFilePath, AudioPeakCacheEntry &entry) {
    const QFileInfo sourceInfo(sourceFilePath);
    if (!sourceInfo.exists() || !sourceInfo.isFile()) {
        return false;
    }

    QFile cacheFile(cachePathFor(sourceFilePath));
    if (!cacheFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    QDataStream stream(&cacheFile);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    quint32 magic = 0;
    quint32 version = 0;
    qint64 sourceSize = 0;
    qint64 sourceMtimeSecs = 0;
    quint32 bucketCount = 0;

    stream >> magic >> version >> sourceSize >> sourceMtimeSecs >> bucketCount;
    stream >> entry.durationMs >> entry.sampleRateHz >> entry.channelCount;

    if (stream.status() != QDataStream::Ok || magic != kPeakCacheMagic ||
        version != kPeakCacheVersion || bucketCount == 0 ||
        bucketCount != static_cast<quint32>(AudioConstants::kPeakBucketCount)) {
        return false;
    }

    if (!isSourceValid(sourceInfo, sourceSize, sourceMtimeSecs)) {
        return false;
    }

    entry.peaks.resize(static_cast<int>(bucketCount));
    for (int bucket = 0; bucket < static_cast<int>(bucketCount); ++bucket) {
        float peak = 0.0F;
        stream >> peak;
        entry.peaks[bucket] = peak;
    }

    return stream.status() == QDataStream::Ok && !entry.peaks.isEmpty();
}

bool AudioPeakCache::write(const QString &sourceFilePath, const AudioPeakCacheEntry &entry) {
    const QFileInfo sourceInfo(sourceFilePath);
    if (!sourceInfo.exists() || !sourceInfo.isFile() || entry.peaks.isEmpty()) {
        return false;
    }

    QFile cacheFile(cachePathFor(sourceFilePath));
    if (!cacheFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QDataStream stream(&cacheFile);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    const quint32 bucketCount = static_cast<quint32>(entry.peaks.size());
    stream << kPeakCacheMagic << kPeakCacheVersion << sourceInfo.size()
           << sourceInfo.lastModified().toSecsSinceEpoch() << bucketCount;
    stream << entry.durationMs << entry.sampleRateHz << entry.channelCount;

    for (float peak : entry.peaks) {
        stream << peak;
    }

    return stream.status() == QDataStream::Ok;
}
