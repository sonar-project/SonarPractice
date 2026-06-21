#include "SqliteAudioConfigPresetRepository.h"

#include "RepositoryUtils.h"
#include "SqlQueryColumns.h"

#include <QLoggingCategory>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

    Q_LOGGING_CATEGORY(lcAudioPresetRepo, "sonarp.audio.preset")

    constexpr auto kPresetSelectColumns = "id, media_file_id, name, tempo_percent, eq_preset_id, "
                                          "region_start_ms, region_end_ms, loop_enabled";

} // namespace

/**
 * @brief Constructs the repository with a database connection.
 * @param connection The database connection object to use.
 */
SqliteAudioConfigPresetRepository::SqliteAudioConfigPresetRepository(
    IDatabaseConnection &connection)
    : m_connection(connection) {}

/**
 * @brief Converts the current row of a database query into an AudioConfigPreset object.
 * @param query The active query object positioned at the desired record.
 * @return An AudioConfigPreset object populated with data from the current row.
 */
AudioConfigPreset SqliteAudioConfigPresetRepository::presetFromQuery(const QSqlQuery &query) {
    AudioConfigPreset preset;
    preset.id = query.value(SqlQueryColumns::AudioConfigPreset::Id).toLongLong();
    preset.mediaFileId = query.value(SqlQueryColumns::AudioConfigPreset::MediaFileId).toLongLong();
    preset.name = query.value(SqlQueryColumns::AudioConfigPreset::Name).toString();
    preset.tempoPercent = query.value(SqlQueryColumns::AudioConfigPreset::TempoPercent).toInt();
    preset.eqPresetId = query.value(SqlQueryColumns::AudioConfigPreset::EqPresetId).toString();
    preset.regionStartMs =
        query.value(SqlQueryColumns::AudioConfigPreset::RegionStartMs).toLongLong();
    preset.regionEndMs = query.value(SqlQueryColumns::AudioConfigPreset::RegionEndMs).toLongLong();
    preset.loopEnabled = query.value(SqlQueryColumns::AudioConfigPreset::LoopEnabled).toInt() != 0;
    return preset;
}

/**
 * @brief Retrieves a specific audio preset from the database by its ID.
 * @param presetId The unique ID of the preset.
 * @return The AudioConfigPreset if found, or nullopt if it does not exist.
 */
std::optional<AudioConfigPreset> SqliteAudioConfigPresetRepository::getPreset(qlonglong presetId) {
    if (presetId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(QStringLiteral("SELECT %1 FROM audio_config_presets WHERE id = :id")
                      .arg(QString::fromLatin1(kPresetSelectColumns)));
    query.bindValue(QStringLiteral(":id"), presetId);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    return presetFromQuery(query);
}

/**
 * @brief Gets a list of all presets associated with a specific media file.
 * @param mediaFileId The ID of the media file to look up.
 * @return A vector of audio presets linked to the given media file.
 */
std::vector<AudioConfigPreset>
SqliteAudioConfigPresetRepository::listPresetsForMedia(qlonglong mediaFileId) {
    std::vector<AudioConfigPreset> presets;
    if (mediaFileId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return presets;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(
        QStringLiteral(
            "SELECT %1 FROM audio_config_presets WHERE media_file_id = :media_id ORDER BY name")
            .arg(QString::fromLatin1(kPresetSelectColumns)));
    query.bindValue(QStringLiteral(":media_id"), mediaFileId);

    if (!query.exec()) {
        qCritical(lcAudioPresetRepo) << "listPresetsForMedia failed:" << query.lastError().text();
        return presets;
    }

    while (query.next()) {
        presets.push_back(presetFromQuery(query));
    }
    return presets;
}

/**
 * @brief Saves a new audio configuration preset to the database.
 * @param preset The preset object to store.
 * @return The ID of the newly created preset, or nullopt if saving failed.
 */
std::optional<qlonglong>
SqliteAudioConfigPresetRepository::createPreset(const AudioConfigPreset &preset) {
    if (preset.mediaFileId <= 0 || preset.name.trimmed().isEmpty() ||
        !RepositoryUtils::ensureOpen(m_connection)) {
        return std::nullopt;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(QStringLiteral(
        "INSERT INTO audio_config_presets "
        "(media_file_id, name, tempo_percent, eq_preset_id, region_start_ms, region_end_ms, "
        "loop_enabled) "
        "VALUES (:media_id, :name, :tempo, :eq, :start_ms, :end_ms, :loop)"));
    query.bindValue(QStringLiteral(":media_id"), preset.mediaFileId);
    query.bindValue(QStringLiteral(":name"), preset.name);
    query.bindValue(QStringLiteral(":tempo"), preset.tempoPercent);
    query.bindValue(QStringLiteral(":eq"), preset.eqPresetId);
    query.bindValue(QStringLiteral(":start_ms"), preset.regionStartMs);
    query.bindValue(QStringLiteral(":end_ms"), preset.regionEndMs);
    query.bindValue(QStringLiteral(":loop"), preset.loopEnabled ? 1 : 0);

    if (!query.exec()) {
        qCritical(lcAudioPresetRepo) << "createPreset failed:" << query.lastError().text();
        return std::nullopt;
    }

    const qlonglong newId = query.lastInsertId().toLongLong();
    return newId > 0 ? std::optional<qlonglong>(newId) : std::nullopt;
}

/**
 * @brief Updates the settings of an existing audio configuration preset.
 * @param preset The preset object containing the updated values.
 * @return True if the update was successful, false otherwise.
 */
bool SqliteAudioConfigPresetRepository::updatePreset(const AudioConfigPreset &preset) {
    if (preset.id <= 0 || preset.mediaFileId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(
        QStringLiteral("UPDATE audio_config_presets SET "
                       "name = :name, tempo_percent = :tempo, eq_preset_id = :eq, "
                       "region_start_ms = :start_ms, region_end_ms = :end_ms, loop_enabled = :loop "
                       "WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), preset.id);
    query.bindValue(QStringLiteral(":name"), preset.name);
    query.bindValue(QStringLiteral(":tempo"), preset.tempoPercent);
    query.bindValue(QStringLiteral(":eq"), preset.eqPresetId);
    query.bindValue(QStringLiteral(":start_ms"), preset.regionStartMs);
    query.bindValue(QStringLiteral(":end_ms"), preset.regionEndMs);
    query.bindValue(QStringLiteral(":loop"), preset.loopEnabled ? 1 : 0);

    if (!query.exec()) {
        qCritical(lcAudioPresetRepo) << "updatePreset failed:" << query.lastError().text();
        return false;
    }

    return true;
}

/**
 * @brief Removes an audio configuration preset from the database by its ID.
 * @param presetId The unique ID of the preset to delete.
 * @return True if the deletion was successful, false otherwise.
 */
bool SqliteAudioConfigPresetRepository::deletePreset(qlonglong presetId) {
    if (presetId <= 0 || !RepositoryUtils::ensureOpen(m_connection)) {
        return false;
    }

    QSqlQuery query(RepositoryUtils::database(m_connection));
    query.prepare(QStringLiteral("DELETE FROM audio_config_presets WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), presetId);

    if (!query.exec()) {
        qCritical(lcAudioPresetRepo) << "deletePreset failed:" << query.lastError().text();
        return false;
    }

    return true;
}
