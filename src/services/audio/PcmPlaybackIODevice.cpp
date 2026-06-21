#include "PcmPlaybackIODevice.h"

#include <cstring>

/**
 * @brief Constructs the device and opens it in read-only mode.
 * @param parent Optional QObject parent.
 */
PcmPlaybackIODevice::PcmPlaybackIODevice(QObject *parent) : QIODevice(parent) {
    open(QIODevice::ReadOnly);
}

/**
 * @brief Updates the internal buffer with new PCM data.
 * @param pcmData The raw byte array of audio samples.
 */
void PcmPlaybackIODevice::setPcmData(const QByteArray &pcmData) {
    QMutexLocker locker(&m_mutex);
    m_pcmData = pcmData;
    m_readPosition.store(0);
    if (!m_pcmData.isEmpty()) {
        m_regionEndByte = m_pcmData.size();
    }
}

/**
 * @brief Sets the start and end points for playback and looping.
 * @param regionStartByte Start offset in bytes.
 * @param regionEndByte End offset in bytes.
 * @param loopEnabled Whether playback should restart at regionStartByte when regionEndByte is
 * reached.
 */
void PcmPlaybackIODevice::setLoopRegion(qint64 regionStartByte, qint64 regionEndByte,
                                        bool loopEnabled) {
    QMutexLocker locker(&m_mutex);
    m_regionStartByte = qMax<qint64>(0, regionStartByte);
    m_regionEndByte = qMax(m_regionStartByte, regionEndByte);
    m_loopEnabled = loopEnabled;
}

/**
 * @brief Resets the current playback position to the start of the region.
 */
void PcmPlaybackIODevice::resetReadPosition() { setPlaybackPositionBytes(m_regionStartByte); }

/**
 * @brief Manually sets the playback position in bytes.
 * @param positionBytes The new position offset.
 */
void PcmPlaybackIODevice::setPlaybackPositionBytes(qint64 positionBytes) {
    QMutexLocker locker(&m_mutex);
    const qint64 clamped = qBound<qint64>(0, positionBytes, m_pcmData.size());
    m_readPosition.store(clamped);
}

/**
 * @brief Returns the current read position in the buffer.
 * @return Current position in bytes.
 */
qint64 PcmPlaybackIODevice::playbackPositionBytes() const { return m_readPosition.load(); }

/**
 * @brief Returns the total number of bytes currently available to be read.
 * @return Bytes available.
 */
qint64 PcmPlaybackIODevice::bytesAvailable() const {
    QMutexLocker locker(&m_mutex);
    return availableBytesInRegionLocked();
}

/**
 * @brief Internal helper to calculate available bytes within the loop region under a lock.
 * @return Bytes available.
 */
qint64 PcmPlaybackIODevice::availableBytesInRegionLocked() const {
    if (m_pcmData.isEmpty()) {
        return 0;
    }

    const qint64 regionEnd = qMin(m_regionEndByte, static_cast<qint64>(m_pcmData.size()));
    if (regionEnd <= m_regionStartByte) {
        return 0;
    }

    if (m_loopEnabled) {
        return regionEnd - m_regionStartByte;
    }

    qint64 position = m_readPosition.load();

    if (position < m_regionStartByte) {
        position = m_regionStartByte;
    }

    if (position >= regionEnd) {
        return 0;
    }

    return regionEnd - position;
}

/**
 * @brief Implements the QIODevice read interface to supply audio data.
 * @param data Pointer to the destination buffer.
 * @param maxSize Maximum number of bytes to read.
 * @return Number of bytes actually read.
 */
qint64 PcmPlaybackIODevice::readData(char *data, qint64 maxSize) {
    QMutexLocker locker(&m_mutex);
    if (m_pcmData.isEmpty() || maxSize <= 0) {
        return 0;
    }

    qint64 position = m_readPosition.load();
    if (position < m_regionStartByte) {
        position = m_regionStartByte;
    }

    const qint64 regionEnd = qMin(m_regionEndByte, static_cast<qint64>(m_pcmData.size()));
    if (position >= regionEnd) {
        if (m_loopEnabled && regionEnd > m_regionStartByte) {
            position = m_regionStartByte;
        } else {
            return 0;
        }
    }

    const qint64 available = regionEnd - position;
    const qint64 toRead = qMin(maxSize, available);
    std::memcpy(data, m_pcmData.constData() + position, static_cast<size_t>(toRead));
    position += toRead;

    if (position >= regionEnd) {
        if (m_loopEnabled && regionEnd > m_regionStartByte) {
            position = m_regionStartByte;
        }
    }

    m_readPosition.store(position);
    return toRead;
}

/**
 * @brief Implements the QIODevice write interface to supply audio data.
 * @param data Pointer to the destination buffer.
 * @param maxSize Maximum number of bytes to write.
 * @return Number of bytes actually write. Returns -1 on error.
 */
qint64 PcmPlaybackIODevice::writeData(const char *data, qint64 maxSize) {
    Q_UNUSED(data)
    Q_UNUSED(maxSize)
    return -1;
}

/**
 * @brief Returns the size of the PCM buffer.
 * @return Size in bytes.
 */
qint64 PcmPlaybackIODevice::size() const {
    QMutexLocker locker(&m_mutex);
    return m_pcmData.size();
}

bool PcmPlaybackIODevice::isSequential() const { return true; }

/**
 * @brief Checks if the device is at the end of the data region.
 * @return True if no more data is available and looping is disabled.
 */
bool PcmPlaybackIODevice::atEnd() const {
    QMutexLocker locker(&m_mutex);
    if (m_loopEnabled) {
        return false;
    }
    return availableBytesInRegionLocked() <= 0;
}
