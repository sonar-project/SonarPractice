#ifndef SQLQUERYCOLUMNS_H
#define SQLQUERYCOLUMNS_H

namespace SqlQueryColumns {

    namespace Artist {
        constexpr int Id = 0;
        constexpr int Name = 1;
    } // namespace Artist

    namespace Tuning {
        constexpr int Id = 0;
        constexpr int Name = 1;
    } // namespace Tuning

    namespace User {
        constexpr int Id = 0;
        constexpr int Name = 1;
        constexpr int Role = 2;
    } // namespace User

    namespace Song {
        constexpr int Id = 0;
        constexpr int Title = 1;
        constexpr int BaseBpm = 2;
        constexpr int ArtistId = 3;
        constexpr int TuningId = 4;
        constexpr int TuningName = 5;
    } // namespace Song

    namespace MediaFile {
        constexpr int Id = 0;
        constexpr int SongId = 1;
        constexpr int FilePath = 2;
        constexpr int FileType = 3;
        constexpr int MediaKind = 4;
        constexpr int FileSize = 5;
        constexpr int FileHash = 6;
        constexpr int SourceType = 7;
        constexpr int IsManaged = 8;
        constexpr int CanBePracticed = 9;
        constexpr int ImportRoot = 10;
        constexpr int SourceRelativePath = 11;
        constexpr int HasVideo = 12;
        constexpr int HasAudio = 13;
    } // namespace MediaFile

    namespace LinkGroup {
        constexpr int Id = 0;
        constexpr int Title = 1;
        constexpr int PrimarySongId = 2;
        constexpr int PrimaryMediaId = 3;
        constexpr int CreatedAt = 4;
    } // namespace LinkGroup

    namespace PracticeJournal {
        constexpr int Id = 0;
        constexpr int UserId = 1;
        constexpr int AssetId = 2;
        constexpr int PracticeDate = 3;
        constexpr int StartBar = 4;
        constexpr int EndBar = 5;
        constexpr int PracticedBpm = 6;
        constexpr int TotalReps = 7;
        constexpr int SuccessfulStreaks = 8;
        constexpr int DurationSeconds = 9;
        constexpr int NoticeId = 10;
    } // namespace PracticeJournal

    namespace PracticeNotice {
        constexpr int Id = 0;
        constexpr int SongId = 1;
        constexpr int NoteDate = 2;
        constexpr int Content = 3;
    } // namespace PracticeNotice

    namespace Reminder {
        constexpr int Id = 0;
        constexpr int UserId = 1;
        constexpr int SongId = 2;
        constexpr int Title = 3;
        constexpr int ReminderDate = 4;
        constexpr int IntervalDays = 5;
        constexpr int Weekday = 6;
        constexpr int IsDaily = 7;
        constexpr int IsMonthly = 8;
        constexpr int IsWeekly = 9;
        constexpr int IsActive = 10;
        constexpr int PracticeAssetId = 11;
    } // namespace Reminder

    namespace ReminderCondition {
        constexpr int Id = 0;
        constexpr int ReminderId = 1;
        constexpr int StartBar = 2;
        constexpr int EndBar = 3;
        constexpr int MinBpm = 4;
        constexpr int MinMinutes = 5;
    } // namespace ReminderCondition

    namespace FileRelation {
        constexpr int PrimaryMediaId = 0;
    } // namespace FileRelation

    namespace AudioConfigPreset {
        constexpr int Id = 0;
        constexpr int MediaFileId = 1;
        constexpr int Name = 2;
        constexpr int TempoPercent = 3;
        constexpr int EqPresetId = 4;
        constexpr int RegionStartMs = 5;
        constexpr int RegionEndMs = 6;
        constexpr int LoopEnabled = 7;
    } // namespace AudioConfigPreset

    namespace PracticeAsset {
        constexpr int Id = 0;
        constexpr int SongId = 1;
        constexpr int GuitarProId = 2;
        constexpr int AudioId = 3;
        constexpr int VideoId = 4;
        constexpr int ImageId = 5;
        constexpr int DocumentId = 6;
    } // namespace PracticeAsset

} // namespace SqlQueryColumns

#endif // SQLQUERYCOLUMNS_H
