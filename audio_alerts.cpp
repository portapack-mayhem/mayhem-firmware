/*
 * Copyright (C) 2025 Enhanced Drone Analyzer
 *
 * This file is part of PortaPack Mayhem Firmware.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <cstdint>

#include "ch.h"

#include "audio_alerts.hpp"
#include "baseband_api.hpp"
#include "constants.hpp"

namespace drone_analyzer {

// ============================================================================
// Static Member Initialization
// ============================================================================

bool AudioAlertManager::enabled_ = true;
AlertPriority AudioAlertManager::current_priority_ = AlertPriority::LOW;
uint32_t AudioAlertManager::last_beep_tick_ = 0;
AudioAlertManager::SOSState AudioAlertManager::sos_state_{};

// ============================================================================
// AudioAlertConfig Implementation
// ============================================================================

AudioAlertConfig::AudioAlertConfig() noexcept
    : frequency_hz(AUDIO_ALERT_FREQUENCY_HZ)
    , duration_ms(AUDIO_ALERT_DURATION_MS)
    , sample_rate_hz(AUDIO_ALERT_SAMPLE_RATE_HZ)
    , beep_count(1)
    , beep_gap_ms(0)
    , priority(AlertPriority::MEDIUM) {
}

AudioAlertConfig::AudioAlertConfig(
    uint32_t freq,
    uint32_t duration,
    uint32_t sample_rate,
    uint8_t count,
    uint32_t gap,
    AlertPriority prio
) noexcept
    : frequency_hz(freq)
    , duration_ms(duration)
    , sample_rate_hz(sample_rate)
    , beep_count(count)
    , beep_gap_ms(gap)
    , priority(prio) {
}

// ============================================================================
// Alert configs — reference only, actual playback handled by SOS/multi-beep
// ============================================================================

const AudioAlertConfig& AudioAlertManager::get_alert_config(AlertType alert_type) noexcept {
    static const AudioAlertConfig configs[] = {
        // NEW_DRONE (MEDIUM): 1000 Hz, 100ms
        AudioAlertConfig(AUDIO_ALERT_FREQUENCY_HZ, AUDIO_ALERT_MEDIUM_DURATION_MS, AUDIO_ALERT_SAMPLE_RATE_HZ, 1, 0, AlertPriority::MEDIUM),

        // THREAT_INCREASED (HIGH): 1200 Hz, SOS continuous
        AudioAlertConfig(AUDIO_ALERT_HIGH_FREQUENCY_HZ, AUDIO_ALERT_DURATION_MS, AUDIO_ALERT_SAMPLE_RATE_HZ, 1, 0, AlertPriority::HIGH),

        // THREAT_CRITICAL: 1500 Hz, SOS continuous
        AudioAlertConfig(AUDIO_ALERT_CRITICAL_FREQUENCY_HZ, AUDIO_ALERT_LONG_DURATION_MS, AUDIO_ALERT_SAMPLE_RATE_HZ, 1, 0, AlertPriority::CRITICAL),

        // DRONE_APPROACHING (HIGH): 1200 Hz, SOS continuous
        AudioAlertConfig(AUDIO_ALERT_HIGH_FREQUENCY_HZ, AUDIO_ALERT_DURATION_MS, AUDIO_ALERT_SAMPLE_RATE_HZ, 1, 0, AlertPriority::HIGH),

        // DRONE_RECEDING (LOW): 800 Hz, 80ms
        AudioAlertConfig(AUDIO_ALERT_LOW_FREQUENCY_HZ, AUDIO_ALERT_SHORT_DURATION_MS, AUDIO_ALERT_SAMPLE_RATE_HZ, 1, 0, AlertPriority::LOW)
    };

    const size_t index = static_cast<size_t>(alert_type);

    if (index < sizeof(configs) / sizeof(configs[0])) {
        return configs[index];
    }

    static const AudioAlertConfig no_alert(0, 0, AUDIO_ALERT_SAMPLE_RATE_HZ, 0, 0, AlertPriority::LOW);
    return no_alert;
}

const AudioAlertConfig& AudioAlertManager::get_threat_alert_config(ThreatLevel threat_level) noexcept {
    switch (threat_level) {
        case ThreatLevel::CRITICAL:
            return get_alert_config(AlertType::THREAT_CRITICAL);
        case ThreatLevel::HIGH:
            return get_alert_config(AlertType::THREAT_INCREASED);
        case ThreatLevel::MEDIUM:
            return get_alert_config(AlertType::NEW_DRONE);
        case ThreatLevel::LOW:
            return get_alert_config(AlertType::DRONE_RECEDING);
        case ThreatLevel::NONE:
        default:
            static const AudioAlertConfig no_alert(0, 0, 24000, 0, 0, AlertPriority::LOW);
            return no_alert;
    }
}

// ============================================================================
// SOS / Multi-beep pattern engine
// ============================================================================

void AudioAlertManager::start_sos(uint32_t freq, uint32_t sample_rate, bool continuous) noexcept {
    sos_state_.active = true;
    sos_state_.freq = freq;
    sos_state_.sample_rate = sample_rate;
    sos_state_.step = 0;
    sos_state_.step_start_tick = chTimeNow();
    sos_state_.continuous = continuous;

    // Play first beep immediately
    baseband::request_audio_beep(freq, sample_rate, SHORT_BEEP_MS);
}

void AudioAlertManager::update() noexcept {
    if (!sos_state_.active) return;

    const uint32_t now = chTimeNow();
    const uint32_t elapsed = now - sos_state_.step_start_tick;

    /*
     * SOS pattern:  S       O         S
     *               ···     ---       ···
     *
     * Step:  0 1 2 3 4 5 6 7 8 9 10 11 12 13 14
     * Type:  B g B g B g B g B g  B  g  B  g  S
     * Dur:   s g s g s g l g l g  l  g  s  g  s
     *
     * B = beep, g = gap, s = SHORT_BEEP_MS, l = LONG_BEEP_MS
     * After steps 2, 8, 14: letter gap (LETTER_GAP_MS)
     * After step 14: cycle gap (CYCLE_GAP_MS), then loop or stop
     *
     * For MEDIUM (3 beeps, one-shot): only steps 0-4 (B g B g B)
     */

    // Step timing: even steps = beep (already fired), odd steps = gap
    uint32_t step_duration;

    if (sos_state_.step % 2 == 0) {
        // Beep step — duration = beep length
        step_duration = (sos_state_.step == 6 || sos_state_.step == 8 ||
                         sos_state_.step == 10 || sos_state_.step == 12)
            ? LONG_BEEP_MS : SHORT_BEEP_MS;
    } else {
        // Gap step — duration = gap length
        // Letter gaps after steps 3 (end of first S), 9 (end of O), 15 (end of second S)
        if (sos_state_.step == 5 || sos_state_.step == 11) {
            step_duration = LETTER_GAP_MS;
        } else if (sos_state_.step == 14) {
            step_duration = CYCLE_GAP_MS;
        } else {
            step_duration = BEEP_GAP_MS;
        }
    }

    if (elapsed < step_duration) return;

    // Advance to next step
    sos_state_.step++;
    sos_state_.step_start_tick = now;

    // One-shot mode (MEDIUM): stop after 3 beeps (step 4 = third beep played)
    if (!sos_state_.continuous && sos_state_.step > TRIPLE_PATTERN_LEN) {
        sos_state_.active = false;
        return;
    }

    // Continuous mode (HIGH/CRITICAL): wrap at end of SOS pattern
    if (sos_state_.step >= SOS_PATTERN_LEN) {
        sos_state_.step = 0;
    }

    // Play beep on even steps (new beep)
    if (sos_state_.step % 2 == 0) {
        const uint32_t dur = (sos_state_.step == 6 || sos_state_.step == 8 ||
                              sos_state_.step == 10 || sos_state_.step == 12)
            ? LONG_BEEP_MS : SHORT_BEEP_MS;
        baseband::request_audio_beep(sos_state_.freq, sos_state_.sample_rate, dur);
    }
}

// ============================================================================
// Play alert — routes to SOS or single beep
// ============================================================================

void AudioAlertManager::play_alert(const AudioAlertConfig& config) noexcept {
    if (!enabled_) return;

    // HARD BLOCK: if SOS is active, drop everything except CRITICAL.
    // Prevents beep queue buildup → hard fault.
    if (sos_state_.active) {
        if (config.priority == AlertPriority::CRITICAL) {
            // CRITICAL interrupts any SOS (even another CRITICAL)
            baseband::request_beep_stop();
            sos_state_.active = false;
            current_priority_ = AlertPriority::CRITICAL;
            last_beep_tick_ = chTimeNow();
            start_sos(config.frequency_hz, config.sample_rate_hz, true);
        }
        // All other priorities: silently dropped while SOS plays
        return;
    }

    const uint32_t now = chTimeNow();

    // Priority decay: reset gate after 500ms of silence
    constexpr uint32_t PRIORITY_DECAY_MS = 500;
    if (current_priority_ != AlertPriority::LOW) {
        if ((now - last_beep_tick_) > PRIORITY_DECAY_MS) {
            current_priority_ = AlertPriority::LOW;
        }
    }

    // LOW: single beep (only if no higher priority active)
    if (config.priority == AlertPriority::LOW) {
        if (current_priority_ >= AlertPriority::MEDIUM) return;
        current_priority_ = AlertPriority::LOW;
        last_beep_tick_ = now;
        baseband::request_audio_beep(config.frequency_hz, config.sample_rate_hz, config.duration_ms);
        return;
    }

    // CRITICAL: start SOS continuous
    if (config.priority == AlertPriority::CRITICAL) {
        current_priority_ = AlertPriority::CRITICAL;
        last_beep_tick_ = now;
        start_sos(config.frequency_hz, config.sample_rate_hz, true);
        return;
    }

    // HIGH: start SOS continuous (only if no higher priority active)
    if (config.priority == AlertPriority::HIGH) {
        if (current_priority_ >= AlertPriority::HIGH) return;
        current_priority_ = AlertPriority::HIGH;
        last_beep_tick_ = now;
        start_sos(config.frequency_hz, config.sample_rate_hz, true);
        return;
    }

    // MEDIUM: 3 beeps one-shot (only if no SOS-level priority active)
    if (config.priority == AlertPriority::MEDIUM) {
        if (current_priority_ >= AlertPriority::HIGH) return;
        current_priority_ = AlertPriority::MEDIUM;
        last_beep_tick_ = now;
        start_sos(config.frequency_hz, config.sample_rate_hz, false);
        return;
    }
}

void AudioAlertManager::play_alert(AlertType alert_type) noexcept {
    play_alert(get_alert_config(alert_type));
}

void AudioAlertManager::play_alert(ThreatLevel threat_level) noexcept {
    play_alert(get_threat_alert_config(threat_level));
}

void AudioAlertManager::stop_alert() noexcept {
    baseband::request_beep_stop();
    sos_state_.active = false;
    sos_state_.step = 0;
    current_priority_ = AlertPriority::LOW;
}

bool AudioAlertManager::is_enabled() noexcept {
    return enabled_;
}

void AudioAlertManager::set_enabled(bool enabled) noexcept {
    enabled_ = enabled;
    if (!enabled) {
        stop_alert();
    }
}

bool AudioAlertManager::is_sos_looping() noexcept {
    return sos_state_.active && sos_state_.continuous;
}

} // namespace drone_analyzer
