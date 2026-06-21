#ifndef IAUDIOCONFIGPRESETREPOSITORY_H
#define IAUDIOCONFIGPRESETREPOSITORY_H

#include "AudioConfigPreset.h"

#include <optional>
#include <vector>

class IAudioConfigPresetRepository {
  public:
    virtual ~IAudioConfigPresetRepository() = default;

    [[nodiscard]] virtual std::optional<AudioConfigPreset> getPreset(qlonglong presetId) = 0;
    [[nodiscard]] virtual std::vector<AudioConfigPreset>
    listPresetsForMedia(qlonglong mediaFileId) = 0;
    [[nodiscard]] virtual std::optional<qlonglong>
    createPreset(const AudioConfigPreset &preset) = 0;
    [[nodiscard]] virtual bool updatePreset(const AudioConfigPreset &preset) = 0;
    [[nodiscard]] virtual bool deletePreset(qlonglong presetId) = 0;
};

#endif // IAUDIOCONFIGPRESETREPOSITORY_H
