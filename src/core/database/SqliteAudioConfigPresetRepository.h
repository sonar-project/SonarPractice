#ifndef SQLITEAUDIOCONFIGPRESETREPOSITORY_H
#define SQLITEAUDIOCONFIGPRESETREPOSITORY_H

#include "interfaces/IAudioConfigPresetRepository.h"
#include "interfaces/IDatabaseConnection.h"

#include <QSqlQuery>

class SqliteAudioConfigPresetRepository : public IAudioConfigPresetRepository {
  public:
    explicit SqliteAudioConfigPresetRepository(IDatabaseConnection &connection);

    [[nodiscard]] std::optional<AudioConfigPreset> getPreset(qlonglong presetId) override;
    [[nodiscard]] std::vector<AudioConfigPreset>

    listPresetsForMedia(qlonglong mediaFileId) override;
    [[nodiscard]] std::optional<qlonglong> createPreset(const AudioConfigPreset &preset) override;

    [[nodiscard]] bool updatePreset(const AudioConfigPreset &preset) override;
    [[nodiscard]] bool deletePreset(qlonglong presetId) override;

  private:
    [[nodiscard]] static AudioConfigPreset presetFromQuery(const QSqlQuery &query);

    IDatabaseConnection &m_connection;
};

#endif // SQLITEAUDIOCONFIGPRESETREPOSITORY_H
