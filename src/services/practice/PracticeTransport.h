#ifndef PRACTICETRANSPORT_H
#define PRACTICETRANSPORT_H

#include "PracticeSessionConfig.h"

/**
 * @brief Future shared play/stop coordinator for Guitar Pro and/or backing audio.
 *
 * Phase 0: holds session config and playback state only — does not drive
 * AlphaTab or AudioPlaybackEngine yet (Phase 2).
 */
class PracticeTransport {
  public:
    enum class State {
        Idle,
        Playing,
        Paused,
    };

    void configure(const PracticeSessionConfig &config);
    [[nodiscard]] const PracticeSessionConfig &config() const;

    [[nodiscard]] State state() const;

    /** Begins a run conceptually; no engines until Phase 2. */
    bool play();
    void pause();
    void stop();

  private:
    PracticeSessionConfig m_config{};
    State m_state{State::Idle};
};

#endif // PRACTICETRANSPORT_H
