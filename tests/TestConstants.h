#ifndef TESTCONSTANTS_H
#define TESTCONSTANTS_H

namespace TestBpm {
constexpr int kDefaultSong = 120;
constexpr int kSecondary = 100;
constexpr int kPractice = 90;
constexpr int kSlow = 80;
constexpr int kUpdated = 95;
constexpr int kConditionMin = 88;
constexpr int kSession = 72;
constexpr int kBase = 60;
constexpr int kHigh = 160;
constexpr int kLastEntry = 102;
constexpr int kOlderEntry = 70;
} // namespace TestBpm

namespace TestBars {
constexpr int kStart = 5;
constexpr int kEnd = 8;
constexpr int kEndExtended = 12;
constexpr int kEndSection = 16;
constexpr int kStartLater = 9;
constexpr int kEndLater = 24;
constexpr int kEndStub = 10;
constexpr int kStartStub = 8;
} // namespace TestBars

namespace TestReps {
constexpr int kTotal = 7;
constexpr int kSuccessful = 5;
constexpr int kPartialSuccess = 3;
constexpr int kFullSuccess = 7;
constexpr int kNearTotal = 9;
constexpr int kTenTotal = 10;
} // namespace TestReps

namespace TestDurations {
constexpr int kShortSeconds = 30;
constexpr int kPracticeSeconds = 90;
constexpr int kLongSeconds = 120;
constexpr int kFiveMinutes = 300;
constexpr int kNineMinutes = 540;
constexpr int kOneMinute = 60;
constexpr int kTwoMinutes = 2;
constexpr int kOneMinuteRounded = 1;
} // namespace TestDurations

namespace TestDates {
constexpr int kYear = 2026;
constexpr int kJanuary = 1;
constexpr int kFebruary = 2;
constexpr int kMarch = 3;
constexpr int kMay = 5;
constexpr int kJune = 6;
constexpr int kDay1 = 1;
constexpr int kDay10 = 10;
constexpr int kDay20 = 20;
constexpr int kDay28 = 28;
constexpr int kHour9 = 9;
constexpr int kHour10 = 10;
constexpr int kMinute0 = 0;
constexpr int kMinute30 = 30;
} // namespace TestDates

namespace TestFileSizes {
constexpr int kSmall = 512;
constexpr int kMedium = 1024;
} // namespace TestFileSizes

namespace TestReminder {
constexpr int kIntervalDays = 15;
constexpr int kConditionEndBar = 8;
constexpr int kConditionMinBpm = 80;
} // namespace TestReminder

namespace TestCounts {
constexpr int kThree = 3;
constexpr int kTwo = 2;
constexpr int kOne = 1;
constexpr int kZero = 0;
} // namespace TestCounts

namespace TestJournal {
constexpr int kDefaultStartBar = 1;
constexpr int kDefaultEndBar = 4;
constexpr int kDefaultTargetBpm = 60;
} // namespace TestJournal

#endif // TESTCONSTANTS_H
