#ifndef PRACTICETRACKERSERVICE_H
#define PRACTICETRACKERSERVICE_H

#include <optional>

#include <QDateTime>
#include <QElapsedTimer>
#include <QtGlobal>

class IPracticeJournalRepository;

struct PracticeTrackerParams {
    qlonglong assetId{};
    qlonglong userId{1};
    int startBar{1};
    int endBar{1};
    int targetBpm{50};
    int totalReps{};
    int successfulReps{};
    QDateTime practiceDate{};
};

class PracticeTrackerService {
  public:
    explicit PracticeTrackerService(IPracticeJournalRepository &journalRepo);

    static int computeStreak(int totalReps, int successfulReps);
    static int durationMinutesFromSeconds(int durationSeconds);

    [[nodiscard]] bool isTimerRunning() const;
    [[nodiscard]] int elapsedSeconds() const;
    [[nodiscard]] bool canSave(const PracticeTrackerParams &params) const;

    bool startTimer();
    std::optional<qlonglong> stopAndSave(const PracticeTrackerParams &params,
                                         int elapsedSecondsOverride = -1);
    void cancelTimer();

  private:
    IPracticeJournalRepository &m_journalRepo;
    bool m_timerRunning{false};
    QElapsedTimer m_elapsedTimer{};
};

#endif // PRACTICETRACKERSERVICE_H
