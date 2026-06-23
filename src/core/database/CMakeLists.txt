# src/core/database/CMakeLists.txt

find_package(Qt6 REQUIRED COMPONENTS Core Sql)

add_library(DatabaseMangerLib STATIC
    interfaces/IDatabaseConnection.h
    interfaces/IUserRepository.h
    interfaces/IArtistRepository.h
    interfaces/ITuningRepository.h
    interfaces/ISongRepository.h
    interfaces/IMediaFileRepository.h
    interfaces/ILinkGroupRepository.h
    interfaces/IFileRelationRepository.h
    interfaces/IPracticeJournalRepository.h
    interfaces/IPracticeNoticeRepository.h
    interfaces/IPracticeAssetRepository.h
    interfaces/IReminderRepository.h
    interfaces/IReminderConditionRepository.h
    interfaces/IReminderCompletionRepository.h
    interfaces/IAudioConfigPresetRepository.h
    User.h
    Artist.h
    Tuning.h
    Song.h
    MediaFile.h
    LinkGroup.h
    MediaFileMapping.h
    JournalEntry.h
    JournalDayEntry.h
    PracticeAsset.h
    PracticeNotice.h
    Reminder.h Reminder.cpp
    ReminderConstants.h
    ReminderCondition.h
    ReminderCompletionOverride.h
    AudioConfigPreset.h
    SqlQueryColumns.h
    RepositoryUtils.h
    SqliteConnection.h SqliteConnection.cpp
    DatabaseSchema.h DatabaseSchema.cpp
    SqliteUserRepository.h SqliteUserRepository.cpp
    SqliteArtistRepository.h SqliteArtistRepository.cpp
    SqliteTuningRepository.h SqliteTuningRepository.cpp
    SqliteSongRepository.h SqliteSongRepository.cpp
    SqliteMediaFileRepository.h SqliteMediaFileRepository.cpp
    SqliteLinkGroupRepository.h SqliteLinkGroupRepository.cpp
    SqliteFileRelationRepository.h SqliteFileRelationRepository.cpp
    SqlitePracticeJournalRepository.h SqlitePracticeJournalRepository.cpp
    SqlitePracticeAssetRepository.h SqlitePracticeAssetRepository.cpp
    SqlitePracticeNoticeRepository.h SqlitePracticeNoticeRepository.cpp
    SqliteReminderRepository.h SqliteReminderRepository.cpp
    SqliteReminderConditionRepository.h SqliteReminderConditionRepository.cpp
    SqliteReminderCompletionRepository.h SqliteReminderCompletionRepository.cpp
    SqliteAudioConfigPresetRepository.h SqliteAudioConfigPresetRepository.cpp
)

target_link_libraries(DatabaseMangerLib PUBLIC Qt6::Core Qt6::Sql)

target_include_directories(DatabaseMangerLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
