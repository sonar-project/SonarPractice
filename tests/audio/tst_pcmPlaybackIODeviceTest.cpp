#include "tst_pcmPlaybackIODeviceTest.h"

#include "PcmPlaybackIODevice.h"

#include <QTest>

#include <cstring>

void TestPcmPlaybackIODevice::readDataNeverExceedsBufferBounds() {
    PcmPlaybackIODevice device;
    QByteArray pcm(100, '\0');
    for (int i = 0; i < pcm.size(); ++i) {
        pcm[i] = static_cast<char>(i);
    }
    device.setPcmData(pcm);

    char output[256];
    std::memset(output, 0xCC, sizeof(output));

    const qint64 read = device.readData(output, 256);
    QCOMPARE(read, 100);
    QCOMPARE(static_cast<unsigned char>(output[0]), 0u);
    QCOMPARE(static_cast<unsigned char>(output[99]), 99u);

    for (std::size_t i = 100; i < sizeof(output); ++i) {
        QCOMPARE(static_cast<unsigned char>(output[i]), 0xCCu);
    }
}

void TestPcmPlaybackIODevice::readDataReturnsZeroForEmptyBuffer() {
    PcmPlaybackIODevice device;
    char output[16];
    QCOMPARE(device.readData(output, 16), 0);
    QCOMPARE(device.readData(output, 0), 0);
}

void TestPcmPlaybackIODevice::setPlaybackPositionClampsToBufferSize() {
    PcmPlaybackIODevice device;
    device.setPcmData(QByteArray(50, '\0'));

    device.setPlaybackPositionBytes(999);
    QCOMPARE(device.playbackPositionBytes(), 50);

    device.setPlaybackPositionBytes(-10);
    QCOMPARE(device.playbackPositionBytes(), 0);
}

void TestPcmPlaybackIODevice::loopRegionEndClampsToBufferSize() {
    PcmPlaybackIODevice device;
    device.setPcmData(QByteArray(64, 'x'));
    device.setLoopRegion(10, 200, false);
    device.setPlaybackPositionBytes(10);

    char buffer[64];
    const qint64 read = device.readData(buffer, 64);
    QCOMPARE(read, 54);
}

void TestPcmPlaybackIODevice::loopingRewindsAtRegionEnd() {
    PcmPlaybackIODevice device;
    device.setPcmData(QByteArray("ABCD", 4));
    device.setLoopRegion(0, 4, true);
    device.setPlaybackPositionBytes(3);

    char oneByte[1];
    QCOMPARE(device.readData(oneByte, 1), 1);
    QCOMPARE(oneByte[0], 'D');
    QCOMPARE(device.playbackPositionBytes(), 0);

    char fourBytes[4];
    QCOMPARE(device.readData(fourBytes, 4), 4);
    QCOMPARE(QByteArray(fourBytes, 4), QByteArray("ABCD"));
    QCOMPARE(device.playbackPositionBytes(), 0);
}

void TestPcmPlaybackIODevice::nonLoopingStopsAtRegionEnd() {
    PcmPlaybackIODevice device;
    device.setPcmData(QByteArray(20, 'z'));
    device.setLoopRegion(0, 8, false);
    device.setPlaybackPositionBytes(6);

    char buffer[8];
    QCOMPARE(device.readData(buffer, 8), 2);
    QCOMPARE(device.readData(buffer, 8), 0);
    QVERIFY(device.atEnd());
}

QTEST_MAIN(TestPcmPlaybackIODevice)
