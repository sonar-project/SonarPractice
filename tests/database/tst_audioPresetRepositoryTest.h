#ifndef TST_AUDIOPRESETREPOSITORYTEST_H
#define TST_AUDIOPRESETREPOSITORYTEST_H

#include "DatabaseTestFixture.h"
#include "SqliteAudioConfigPresetRepository.h"

#include <QObject>

class TestAudioPresetRepository : public QObject, protected DatabaseTestFixture {
    Q_OBJECT

  public:
    TestAudioPresetRepository()
        : DatabaseTestFixture(QStringLiteral("AudioPresetRepositoryTest")) {}

  private slots:
    void init();
    void cleanup();
    void createListAndDeletePreset();

  private:
    [[nodiscard]] std::optional<qlonglong> createMediaFile();

    SqliteAudioConfigPresetRepository m_presetRepo{m_connector};
};

#endif // TST_AUDIOPRESETREPOSITORYTEST_H
