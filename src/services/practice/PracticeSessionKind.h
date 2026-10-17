#ifndef PRACTICESESSIONKIND_H
#define PRACTICESESSIONKIND_H

/**
 * @brief How a future analysis-backed practice run is interpreted.
 *
 * Phase 0 placeholder — no UI or analyser wiring yet.
 * Practice: N loops at target BPM (mastery path).
 * Probe: typically one loop at last practiced BPM (capability check).
 */
enum class PracticeSessionKind {
    Practice,
    Probe,
};

#endif // PRACTICESESSIONKIND_H
