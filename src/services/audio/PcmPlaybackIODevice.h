#ifndef PCMPLAYBACKIODEVICE_H
#define PCMPLAYBACKIODEVICE_H

#include <QIODevice>
#include <QMutex>
#include <atomic>

class PcmPlaybackIODevice : public QIODevice {
    Q_OBJECT

  public:
    explicit PcmPlaybackIODevice(QObject *parent = nullptr);

    void setPcmData(const QByteArray &pcmData);
    void setLoopRegion(qint64 regionStartByte, qint64 regionEndByte, bool loopEnabled);
    void resetReadPosition();
    void setPlaybackPositionBytes(qint64 positionBytes);

    [[nodiscard]] qint64 playbackPositionBytes() const;
    [[nodiscard]] qint64 bytesAvailable() const override;
    [[nodiscard]] qint64 readData(char *data, qint64 maxSize) override;
    [[nodiscard]] qint64 writeData(const char *data, qint64 maxSize) override;
    [[nodiscard]] qint64 size() const override;
    [[nodiscard]] bool isSequential() const override;
    [[nodiscard]] bool atEnd() const override;

  private:
    [[nodiscard]] qint64 availableBytesInRegionLocked() const;

    QByteArray m_pcmData;
    std::atomic<qint64> m_readPosition{};
    qint64 m_regionStartByte{};
    qint64 m_regionEndByte{};
    bool m_loopEnabled{false};
    mutable QMutex m_mutex{};
};

#endif // PCMPLAYBACKIODEVICE_H
