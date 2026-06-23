#ifndef REPOSITORYFACTORY_H
#define REPOSITORYFACTORY_H

#include "SqliteArtistRepository.h"
#include "SqliteAudioConfigPresetRepository.h"
#include "SqliteFileRelationRepository.h"
#include "SqliteLinkGroupRepository.h"
#include "SqliteMediaFileRepository.h"
#include "SqlitePracticeAssetRepository.h"
#include "SqlitePracticeJournalRepository.h"
#include "SqlitePracticeNoticeRepository.h"
#include "SqliteReminderConditionRepository.h"
#include "SqliteReminderCompletionRepository.h"
#include "SqliteReminderRepository.h"
#include "SqliteSongRepository.h"
#include "SqliteTuningRepository.h"
#include "SqliteUserRepository.h"

struct RepositoryFactory {
    struct Dependencies {
        IDatabaseConnection &connection;
    };

    struct Repositories {
        std::unique_ptr<SqliteArtistRepository> artist;
        std::unique_ptr<SqliteTuningRepository> tuning;
        std::unique_ptr<SqliteSongRepository> song;
        std::unique_ptr<SqliteMediaFileRepository> mediaFile;
        std::unique_ptr<SqliteLinkGroupRepository> linkGroup;
        std::unique_ptr<SqliteFileRelationRepository> fileRelation;
        std::unique_ptr<SqlitePracticeJournalRepository> journal;
        std::unique_ptr<SqlitePracticeAssetRepository> practiceAsset;
        std::unique_ptr<SqlitePracticeNoticeRepository> notice;
        std::unique_ptr<SqliteReminderRepository> reminder;
        std::unique_ptr<SqliteReminderConditionRepository> reminderCondition;
        std::unique_ptr<SqliteReminderCompletionRepository> reminderCompletion;
        std::unique_ptr<SqliteAudioConfigPresetRepository> audioPreset;
        std::unique_ptr<SqliteUserRepository> user;
    };

    static Repositories create(const Dependencies &deps) {
        IDatabaseConnection &conn = deps.connection;
        Repositories repos;
        repos.artist = std::make_unique<SqliteArtistRepository>(conn);
        repos.tuning = std::make_unique<SqliteTuningRepository>(conn);
        repos.song = std::make_unique<SqliteSongRepository>(conn);
        repos.mediaFile = std::make_unique<SqliteMediaFileRepository>(conn);
        repos.linkGroup = std::make_unique<SqliteLinkGroupRepository>(conn);
        repos.fileRelation = std::make_unique<SqliteFileRelationRepository>(conn);
        repos.journal = std::make_unique<SqlitePracticeJournalRepository>(conn);
        repos.practiceAsset = std::make_unique<SqlitePracticeAssetRepository>(conn);
        repos.notice = std::make_unique<SqlitePracticeNoticeRepository>(conn);
        repos.reminder = std::make_unique<SqliteReminderRepository>(conn);
        repos.reminderCondition = std::make_unique<SqliteReminderConditionRepository>(conn);
        repos.reminderCompletion = std::make_unique<SqliteReminderCompletionRepository>(conn);
        repos.audioPreset = std::make_unique<SqliteAudioConfigPresetRepository>(conn);
        repos.user = std::make_unique<SqliteUserRepository>(conn);
        return repos;
    }
};

#endif // REPOSITORYFACTORY_H