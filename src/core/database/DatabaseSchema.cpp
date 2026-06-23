#include "QSqlDatabase"
#include "QSqlError"
#include "QSqlQuery"

#include "DatabaseSchema.h"

/**
 * @brief Creates all tables, performance indexes, and legacy schema upgrades.
 *
 * Table order respects foreign-key dependencies. See DatabaseSchema.h for the
 * full list. Also calls migratePerformanceIndexes() and
 * migrateRemindersMediaFileToPracticeAsset().
 *
 * @return True if every step succeeded, false otherwise.
 */
bool DatabaseSchema::createAllTables() {
    // Order matters: parent tables before dependents (foreign keys).
    return createTable(SqlStatement("CREATE TABLE IF NOT EXISTS users ("
                                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                    "name TEXT UNIQUE, "
                                    "role TEXT, " // 'admin, teacher' or 'student'
                                    "created_at DATETIME DEFAULT CURRENT_TIMESTAMP)",
                                    "users")) &&
           // artists
           createTable(SqlStatement("CREATE TABLE IF NOT EXISTS artists ("
                                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                    "name TEXT UNIQUE NOT NULL)",
                                    "artists")) &&
           // songs (references artists, tunings)
           createTable(SqlStatement("CREATE TABLE IF NOT EXISTS songs ("
                                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                    "user_id INTEGER NOT NULL DEFAULT 1,"
                                    "title TEXT, artist_id INTEGER, tuning_id INTEGER,"
                                    "base_bpm INTEGER,"
                                    "total_bars INTEGER,"
                                    "current_bpm INTEGER DEFAULT 0,"
                                    "is_favorite INTEGER DEFAULT 0,"
                                    "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
                                    "FOREIGN KEY(artist_id) REFERENCES artists(id),"
                                    "FOREIGN KEY(tuning_id) REFERENCES tunings(id))",
                                    "songs")) &&
           // media_files (references songs)
           createTable(SqlStatement("CREATE TABLE IF NOT EXISTS media_files ("
                                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                    "song_id INTEGER NOT NULL, "
                                    "file_path TEXT NOT NULL UNIQUE, "
                                    "file_type TEXT, "
                                    "media_kind TEXT, "
                                    "file_size INTEGER, "
                                    "file_hash TEXT, "
                                    "source_type TEXT NOT NULL DEFAULT 'LOCAL', "
                                    "is_managed INTEGER NOT NULL DEFAULT 0, "
                                    "can_be_practiced INTEGER NOT NULL DEFAULT 0, "
                                    "has_audio INTEGER NOT NULL DEFAULT 0, "
                                    "has_video INTEGER NOT NULL DEFAULT 0, "
                                    "import_root TEXT, "
                                    "source_relative_path TEXT, "
                                    "FOREIGN KEY(song_id) REFERENCES songs(id) ON DELETE CASCADE)",
                                    "media_files")) &&
           // tunings
           createTable(SqlStatement(
               "CREATE TABLE IF NOT EXISTS tunings ( id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "name TEXT UNIQUE NOT NULL)",
               "tunings")) &&
           // file_relations (pairwise media file links)
           createTable(SqlStatement("CREATE TABLE IF NOT EXISTS file_relations ("
                                    "file_id_a INTEGER NOT NULL,"
                                    "file_id_b INTEGER NOT NULL,"
                                    "PRIMARY KEY (file_id_a, file_id_b),"
                                    "UNIQUE (file_id_b))",
                                    "file_relations")) &&
           // link_groups (primary song/media pairs)
           createTable(SqlStatement(
               "CREATE TABLE IF NOT EXISTS link_groups ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "title TEXT NOT NULL, "
               "primary_song_id INTEGER NOT NULL, "
               "primary_media_id INTEGER NOT NULL UNIQUE, "
               "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
               "FOREIGN KEY(primary_song_id) REFERENCES songs(id) ON DELETE CASCADE, "
               "FOREIGN KEY(primary_media_id) REFERENCES media_files(id) ON DELETE CASCADE)",
               "link_groups")) &&
           // sections (bar ranges within a song)
           createTable(SqlStatement("CREATE TABLE IF NOT EXISTS sections ("
                                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                    "song_id INTEGER NOT NULL,"
                                    "name TEXT,"
                                    "start_bar INTEGER NOT NULL,"
                                    "end_bar INTEGER NOT NULL,"
                                    "FOREIGN KEY(song_id) REFERENCES songs(id) ON DELETE CASCADE)",
                                    "sections")) &&
           // practice_notices (dated notes for a song)
           createTable(SqlStatement("CREATE TABLE IF NOT EXISTS practice_notices ("
                                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                    "song_id INTEGER NOT NULL,"
                                    "note_date DATE NOT NULL,"
                                    "content TEXT NOT NULL,"
                                    "FOREIGN KEY(song_id) REFERENCES songs(id) ON DELETE CASCADE)",
                                    "practice_notices")) &&
           // practice_assets (composite song + optional media slots)
           createTable(SqlStatement(
               "CREATE TABLE IF NOT EXISTS practice_assets ("
               "id              INTEGER PRIMARY KEY AUTOINCREMENT,"
               "song_id         INTEGER,"
               "guitar_pro_id   INTEGER,"
               "audio_id        INTEGER,"
               "video_id        INTEGER,"
               "image_id        INTEGER,"
               "document_id     INTEGER,"
               "created_at      DATETIME DEFAULT CURRENT_TIMESTAMP,"
               "FOREIGN KEY(song_id)       REFERENCES songs(id)       ON DELETE CASCADE,"
               "FOREIGN KEY(guitar_pro_id) REFERENCES media_files(id) ON DELETE SET NULL,"
               "FOREIGN KEY(audio_id)      REFERENCES media_files(id) ON DELETE SET NULL,"
               "FOREIGN KEY(video_id)      REFERENCES media_files(id) ON DELETE SET NULL,"
               "FOREIGN KEY(image_id)      REFERENCES media_files(id) ON DELETE SET NULL,"
               "FOREIGN KEY(document_id)   REFERENCES media_files(id) ON DELETE SET NULL"
               ")",
               "practice_assets")) &&
           // practice_journal (session log, references practice_assets)
           createTable(SqlStatement(
               "CREATE TABLE IF NOT EXISTS practice_journal ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "user_id INTEGER NOT NULL DEFAULT 1, "
               "asset_id INTEGER NOT NULL, "
               "practice_date DATETIME DEFAULT CURRENT_TIMESTAMP, "
               "start_bar INTEGER, "
               "end_bar INTEGER, "
               "practiced_bpm INTEGER, "
               "total_reps INTEGER, "
               "successful_streaks INTEGER, "
               "duration_seconds INTEGER DEFAULT 0,"
               "rating INTEGER,"
               "notice_id INTEGER,"
               "FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE, "
               "FOREIGN KEY(asset_id) REFERENCES practice_assets(id) ON DELETE CASCADE, "
               "FOREIGN KEY(notice_id) REFERENCES practice_notices(id) ON DELETE SET NULL)",
               "practice_journal")) &&
           // reminders (scheduled practice reminders)
           createTable(SqlStatement(
               "CREATE TABLE IF NOT EXISTS reminders ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "user_id INTEGER NOT NULL DEFAULT 1,"
               "song_id INTEGER NOT NULL,"
               "title TEXT,"                      // Reminder title (optional)
               "reminder_date DATE,"              // One-off reminder date
               "interval_days INTEGER DEFAULT 0," // X Day after
               "weekday INTEGER,"                 // Day of the week (0-6, 0=Sunday)
               "is_daily INTEGER DEFAULT 0,"      // Daily reminder
               "is_monthly INTEGER DEFAULT 0,"    // Monthly reminder
               "is_weekly INTEGER DEFAULT 0,"     // Weekly reminder
               "is_active INTEGER DEFAULT 1,"     // Active/Inactive
               "practice_asset_id INTEGER,"
               "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
               "FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE,"
               "FOREIGN KEY(song_id) REFERENCES songs(id) ON DELETE CASCADE,"
               "FOREIGN KEY(practice_asset_id) REFERENCES practice_assets(id) ON DELETE SET NULL)",
               "reminders")) &&
           // reminder_conditions (optional bar/BPM/duration rules)
           createTable(
               SqlStatement("CREATE TABLE IF NOT EXISTS reminder_conditions ("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "reminder_id INTEGER NOT NULL, "
                            "start_bar INTEGER, "
                            "end_bar INTEGER, "
                            "min_bpm INTEGER, "
                            "min_minutes INTEGER, "
                            "FOREIGN KEY(reminder_id) REFERENCES reminders(id) ON DELETE CASCADE)",
                            "reminder_conditions")) &&
           createTable(SqlStatement(
               "CREATE TABLE IF NOT EXISTS reminder_completion_overrides ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "reminder_id INTEGER NOT NULL, "
               "completion_date DATE NOT NULL, "
               "accepted INTEGER NOT NULL DEFAULT 1, "
               "FOREIGN KEY(reminder_id) REFERENCES reminders(id) ON DELETE CASCADE, "
               "UNIQUE(reminder_id, completion_date))",
               "reminder_completion_overrides")) &&
           // audio_config_presets (saved playback settings per media file)
           createTable(SqlStatement(
               "CREATE TABLE IF NOT EXISTS audio_config_presets ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "media_file_id INTEGER NOT NULL, "
               "name TEXT NOT NULL, "
               "tempo_percent INTEGER NOT NULL DEFAULT 100, "
               "eq_preset_id TEXT NOT NULL DEFAULT 'flat', "
               "region_start_ms INTEGER NOT NULL DEFAULT 0, "
               "region_end_ms INTEGER NOT NULL DEFAULT 0, "
               "loop_enabled INTEGER NOT NULL DEFAULT 0, "
               "FOREIGN KEY(media_file_id) REFERENCES media_files(id) ON DELETE CASCADE)",
               "audio_config_presets")) &&
           migratePerformanceIndexes() && migrateRemindersMediaFileToPracticeAsset();
}

/**
 * @brief Creates performance indexes for frequently queried columns.
 *
 * Uses CREATE INDEX IF NOT EXISTS (and one unique index on practice_assets).
 * Idempotent on new and existing databases.
 *
 * @return True if every index statement succeeded, false otherwise.
 */
bool DatabaseSchema::migratePerformanceIndexes() {
    return createIndex(SqlStatement("CREATE INDEX IF NOT EXISTS idx_media_files_song_id "
                                    "ON media_files(song_id)",
                                    QStringLiteral("idx_media_files_song_id"))) &&
           createIndex(SqlStatement("CREATE INDEX IF NOT EXISTS idx_link_groups_primary_song "
                                    "ON link_groups(primary_song_id)",
                                    QStringLiteral("idx_link_groups_primary_song"))) &&
           createIndex(SqlStatement("CREATE INDEX IF NOT EXISTS idx_reminders_song_id "
                                    "ON reminders(song_id)",
                                    QStringLiteral("idx_reminders_song_id"))) &&
           createIndex(SqlStatement("CREATE INDEX IF NOT EXISTS idx_reminders_practice_asset_id "
                                    "ON reminders(practice_asset_id)",
                                    QStringLiteral("idx_reminders_practice_asset_id"))) &&
           createIndex(
               SqlStatement("CREATE INDEX IF NOT EXISTS idx_reminder_conditions_reminder_id "
                            "ON reminder_conditions(reminder_id)",
                            QStringLiteral("idx_reminder_conditions_reminder_id"))) &&
           createIndex(SqlStatement("CREATE INDEX IF NOT EXISTS idx_media_files_file_hash "
                                    "ON media_files(file_hash)",
                                    QStringLiteral("idx_media_files_file_hash"))) &&
           createIndex(SqlStatement("CREATE INDEX IF NOT EXISTS idx_practice_journal_asset_date "
                                    "ON practice_journal(asset_id, practice_date)",
                                    QStringLiteral("idx_practice_journal_asset_date"))) &&
           createIndex(SqlStatement("CREATE INDEX IF NOT EXISTS idx_practice_notices_song_date "
                                    "ON practice_notices(song_id, note_date)",
                                    QStringLiteral("idx_practice_notices_song_date"))) &&
           createIndex(SqlStatement(
               "CREATE INDEX IF NOT EXISTS idx_reminder_completion_overrides_date "
               "ON reminder_completion_overrides(completion_date)",
               QStringLiteral("idx_reminder_completion_overrides_date"))) &&
           createIndex(SqlStatement(
               "CREATE UNIQUE INDEX IF NOT EXISTS idx_practice_assets_composition "
               "ON practice_assets(song_id, guitar_pro_id, audio_id, video_id, image_id)",
               QStringLiteral("idx_practice_assets_composition")));
}

/**
 * @brief Upgrades legacy reminders tables that still have media_file_id.
 *
 * Adds practice_asset_id and its index when the old column is present.
 * Does not copy existing media_file_id values into practice_asset_id.
 *
 * @return True if the upgrade succeeded or was not necessary, false on error.
 */
bool DatabaseSchema::migrateRemindersMediaFileToPracticeAsset() {
    QSqlDatabase db = QSqlDatabase::database(m_connection.connectionName());
    QSqlQuery query(db);

    // PRAGMA table_info returns one row per column; check whether media_file_id still exists.
    bool hasOldColumn = false;
    if (!query.exec(QStringLiteral("PRAGMA table_info(reminders)"))) {
        qCritical() << "[DatabaseSchema] PRAGMA table_info(reminders) failed:"
                    << query.lastError().text();
        return false;
    }
    while (query.next()) {
        // Column index 1 is the column name.
        if (query.value(1).toString() == QLatin1String("media_file_id")) {
            hasOldColumn = true;
            break;
        }
    }

    if (!hasOldColumn) {
        return true; // Already migrated or fresh database
    }

    qInfo() << "[DatabaseSchema] Migrating reminders.media_file_id -> practice_asset_id";

    // SQLite before 3.25 has no RENAME COLUMN — add the new column instead.
    // The old media_file_id column may remain unused (no DROP COLUMN before 3.35).
    QSqlQuery add(db);
    if (!add.exec(QStringLiteral("ALTER TABLE reminders ADD COLUMN practice_asset_id INTEGER "
                                 "REFERENCES practice_assets(id) ON DELETE SET NULL"))) {
        // Column already exists — migration was partially applied before
        const QString err = add.lastError().text();
        if (!err.contains(QLatin1String("duplicate column"), Qt::CaseInsensitive)) {
            qCritical() << "[DatabaseSchema] ALTER TABLE reminders ADD COLUMN failed:" << err;
            return false;
        }
    }

    // Index on the new column (idempotent via IF NOT EXISTS).
    QSqlQuery idx(db);
    if (!idx.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_reminders_practice_asset_id "
                                 "ON reminders(practice_asset_id)"))) {
        qWarning() << "[DatabaseSchema] index idx_reminders_practice_asset_id failed:"
                   << idx.lastError().text();
        // Non-fatal — the index is optional
    }

    qInfo() << "[DatabaseSchema] reminders migration complete.";
    return true;
}

/**
 * @brief Executes a SQL command to create an index in the database.
 * @param statement The SqlStatement object containing the query and name.
 * @return True if the index was created, false if there was an error.
 */
bool DatabaseSchema::createIndex(const SqlStatement &statement) {
    QSqlDatabase db = QSqlDatabase::database(m_connection.connectionName());
    QSqlQuery query(db);

    if (!query.exec(statement.sql)) {
        qCritical() << "[DatabaseSchema] create index" << statement.name
                    << "failed:" << query.lastError().text();
        return false;
    }
    return true;
}

/**
 * @brief Executes a SQL command to create a table in the database.
 * @param statement The SqlStatement object containing the query and name.
 * @return True if the table was created, false if there was an error.
 */
bool DatabaseSchema::createTable(const SqlStatement &statement) {
    QSqlDatabase db = QSqlDatabase::database(m_connection.connectionName());
    QSqlQuery q(db);

    if (!q.exec(statement.sql)) {
        qCritical() << "[DatabaseSchema] create table" << statement.name
                    << "failed:" << q.lastError().text();
        qDebug() << "[DatabaseSchema] create table" << statement.name
                 << "failed, query:" << q.executedQuery();
        return false;
    }
    return true;
}
