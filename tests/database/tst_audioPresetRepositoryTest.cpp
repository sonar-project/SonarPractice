#include "tst_audioPresetRepositoryTest.h"

#include "AudioConfigPreset.h"
#include "MediaFile.h"
#include "User.h"

#include <QTest>

void TestAudioPresetRepository::init() {
    setUp();

    User admin;
    admin.name = QStringLiteral("admin");
    admin.role = QStringLiteral("admin");
    QVERIFY(m_userRepo.createUser(admin).has_value());
}

void TestAudioPresetRepository::cleanup() { tearDown(); }

std::optional<qlonglong> TestAudioPresetRepository::createMediaFile() {
    const auto artistId = createTestArtist(QStringLiteral("Artist"));
    if (!artistId.has_value()) {
        return std::nullopt;
    }

    const auto tuningId = createTestTuning(QStringLiteral("Standard"));
    if (!tuningId.has_value()) {
        return std::nullopt;
    }

    const auto songId = createTestSong(*artistId, *tuningId, QStringLiteral("Track"));
    if (!songId.has_value()) {
        return std::nullopt;
    }

    MediaFile media;
    media.songId = *songId;
    media.filePath = QStringLiteral("/tmp/test-audio.mp3");
    media.mediaKind = MediaKind::Audio;
    return m_mediaFileRepo.createMediaFile(media);
}

void TestAudioPresetRepository::createListAndDeletePreset() {
    const auto mediaId = createMediaFile();
    QVERIFY(mediaId.has_value());

    AudioConfigPreset preset;
    preset.mediaFileId = *mediaId;
    preset.name = QStringLiteral("Slow practice");
    preset.tempoPercent = 75;
    preset.eqPresetId = QStringLiteral("reduce_low");
    preset.regionStartMs = 1000;
    preset.regionEndMs = 5000;
    preset.loopEnabled = true;

    const auto presetId = m_presetRepo.createPreset(preset);
    QVERIFY(presetId.has_value());

    const std::vector<AudioConfigPreset> presets = m_presetRepo.listPresetsForMedia(*mediaId);
    QCOMPARE(static_cast<int>(presets.size()), 1);
    QCOMPARE(presets.front().name, QStringLiteral("Slow practice"));
    QCOMPARE(presets.front().tempoPercent, 75);

    QVERIFY(m_presetRepo.deletePreset(*presetId));
    QCOMPARE(static_cast<int>(m_presetRepo.listPresetsForMedia(*mediaId).size()), 0);
}

QTEST_MAIN(TestAudioPresetRepository)
