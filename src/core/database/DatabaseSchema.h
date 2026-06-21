#ifndef DATABASESCHEMA_H
#define DATABASESCHEMA_H

#include "interfaces/IDatabaseConnection.h"

/**
 * \brief Manages the creation and migration of database schema.
 *
 * This class is responsible for creating all necessary tables in the database
 * and applying idempotent schema upgrades so existing databases remain compatible
 * with new application versions.
 */
class DatabaseSchema {
  public:
    /**
     * \brief Constructs a DatabaseSchema object.
     *
     * \param connection The IDatabaseConnection instance used to interact with
     * the database.
     */
    explicit DatabaseSchema(IDatabaseConnection &connection) : m_connection(connection) {}

    /**
     * \brief Creates all tables, performance indexes, and legacy schema upgrades.
     *
     * Creates every application table in dependency order. All statements use
     * \c CREATE TABLE IF NOT EXISTS and are safe to run repeatedly on an
     * existing database.
     *
     * Tables created (in order):
     * - \b users — application users and roles
     * - \b artists — song artists
     * - \b songs — catalog songs (references artists and tunings)
     * - \b media_files — files attached to a song (references songs)
     * - \b tunings — instrument tuning names
     * - \b file_relations — pairwise links between media files
     * - \b link_groups — grouped primary song/media pairs
     * - \b sections — bar ranges within a song
     * - \b practice_notices — dated notes attached to a song
     * - \b practice_assets — composite practice unit linking a song to optional
     *   Guitar Pro, audio, video, image, and document media files
     * - \b practice_journal — practice session log (references practice_assets)
     * - \b reminders — scheduled practice reminders (references practice_assets)
     * - \b reminder_conditions — optional bar/BPM/duration rules for a reminder
     * - \b audio_config_presets — saved playback settings per media file
     *
     * After table creation, calls migratePerformanceIndexes() and
     * migrateRemindersMediaFileToPracticeAsset().
     *
     * \return True if all steps succeeded, false otherwise.
     */
    bool createAllTables();

  private:
    struct SqlStatement {
        QString sql;
        QString name;
    };

    /**
     * \brief Executes a SQL query to create a table.
     *
     * \param statement SQL text and a short label used in error logs.
     * \return True if the statement executed successfully, false otherwise.
     */
    bool createTable(const SqlStatement &statement);

    /**
     * \brief Executes a SQL query to create an index.
     *
     * \param statement SQL text and a short label used in error logs.
     * \return True if the statement executed successfully, false otherwise.
     */
    bool createIndex(const SqlStatement &statement);

    /**
     * \brief Creates performance indexes for frequently queried columns.
     *
     * Uses \c CREATE INDEX IF NOT EXISTS (and one unique index on
     * practice_assets) so the call is idempotent on new and existing databases.
     *
     * \return True if every index statement succeeded, false otherwise.
     */
    bool migratePerformanceIndexes();

    /**
     * \brief Upgrades legacy reminders tables that still have media_file_id.
     *
     * Safe to call on new and already-upgraded databases: uses
     * \c PRAGMA table_info to check whether the old column still exists before
     * issuing \c ALTER TABLE. When needed, adds \c practice_asset_id and its
     * index. Existing row data in \c media_file_id is not copied automatically;
     * the legacy column may remain unused on upgraded databases.
     *
     * \return True if the upgrade succeeded or was not necessary, false on error.
     */
    bool migrateRemindersMediaFileToPracticeAsset();

    IDatabaseConnection &m_connection;
};

#endif // DATABASESCHEMA_H
