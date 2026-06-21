#include "SettingsConfigProvider.h"

SettingsConfigProvider::SettingsConfigProvider(const AppSettings &settings)
    : m_settings(settings) {}

QStringList SettingsConfigProvider::allowedFileTypes() const {
    return m_settings.allowedExtensions();
}
