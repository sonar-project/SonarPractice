#ifndef SETTINGSCONFIGPROVIDER_H
#define SETTINGSCONFIGPROVIDER_H

#include "AppSettings.h"
#include "interfaces/iConfigProvider.h"

class SettingsConfigProvider : public IConfigProvider {
  public:
    explicit SettingsConfigProvider(const AppSettings &settings);

    [[nodiscard]] QStringList allowedFileTypes() const override;

  private:
    const AppSettings &m_settings;
};

#endif // SETTINGSCONFIGPROVIDER_H
