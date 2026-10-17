/**
 * @file PracticeTransport.cpp
 * @brief Phase-0 stub for a future GP + audio transport.
 */

#include "PracticeTransport.h"

void PracticeTransport::configure(const PracticeSessionConfig &config) {
    m_config = config;
    m_state = State::Idle;
}

const PracticeSessionConfig &PracticeTransport::config() const { return m_config; }

PracticeTransport::State PracticeTransport::state() const { return m_state; }

bool PracticeTransport::play() {
    if (!m_config.hasAnyReference()) {
        return false;
    }
    m_state = State::Playing;
    return true;
}

void PracticeTransport::pause() {
    if (m_state == State::Playing) {
        m_state = State::Paused;
    }
}

void PracticeTransport::stop() { m_state = State::Idle; }
