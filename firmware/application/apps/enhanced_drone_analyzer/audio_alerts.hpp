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

#ifndef AUDIO_ALERTS_HPP
#define AUDIO_ALERTS_HPP

#include <cstdint>
#include <cstddef>
#include "drone_types.hpp"

namespace drone_analyzer {

/**
 * @brief Alert types for drone detection
 */
enum class AlertType : uint8_t {
    NEW_DRONE = 0,        ///< New drone detected
    THREAT_INCREASED = 1,  ///< Threat level increased
    THREAT_CRITICAL = 2,     ///< Critical threat level
    DRONE_APPROACHING = 3,  ///< Drone approaching (RSSI increasing)
    DRONE_RECEDING = 4      ///< Drone receding (RSSI decreasing)
};

/**
 * @brief Alert priority levels
 */
enum class AlertPriority : uint8_t {
    LOW = 0,      ///< Low priority (visual only)
    MEDIUM = 1,    ///< Medium priority
    HIGH = 2,      ///< High priority
    CRITICAL = 3    ///< Critical priority (immediate alert)
};

/**
 * @brief Audio alert configuration
 */
struct AudioAlertConfig {
    uint32_t frequency_hz;      ///< Alert frequency in Hz
    uint32_t duration_ms;       ///< Alert duration in milliseconds
    uint32_t sample_rate_hz;    ///< Sample rate in Hz
    uint8_t beep_count;         ///< Number of beeps in sequence
    uint32_t beep_gap_ms;       ///< Gap between beeps in milliseconds
    AlertPriority priority;       ///< Alert priority
    
    /**
     * @brief Default constructor
     */
    AudioAlertConfig() noexcept;
    
    /**
     * @brief Constructor with values
     */
    AudioAlertConfig(
        uint32_t freq,
        uint32_t duration,
        uint32_t sample_rate,
        uint8_t count,
        uint32_t gap,
        AlertPriority prio
    ) noexcept;
};

/**
 * @brief Audio aler manager
 * @note Handles audio alerts for drone detection
 * @note Non-blocking implementation using baseband API
 */
class AudioAlertManager {
public:
    static const AudioAlertConfig& get_alert_config(AlertType alert_type) noexcept;
    static const AudioAlertConfig& get_threat_alert_config(ThreatLevel threat_level) noexcept;
    static void play_alert(const AudioAlertConfig& config) noexcept;
    static void play_alert(AlertType alert_type) noexcept;
    static void play_alert(ThreatLevel threat_level) noexcept;
    static void stop_alert() noexcept;
    static bool is_enabled() noexcept;
    static void set_enabled(bool enabled) noexcept;
    static bool is_sos_looping() noexcept;

    /**
     * @brief Tick-based alert driver — call from UI refresh (~60Hz)
     * Handles multi-beep patterns (3 beeps for MEDIUM, SOS for HIGH/CRITICAL)
     */
    static void update() noexcept;

private:
    static bool enabled_;
    static AlertPriority current_priority_;
    static uint32_t last_beep_tick_;

    // SOS / multi-beep pattern state
    static constexpr uint8_t SOS_PATTERN_LEN = 15;
    static constexpr uint8_t TRIPLE_PATTERN_LEN = 5;

    // Pattern timing constants (ms)
    static constexpr uint32_t SHORT_BEEP_MS = 80;
    static constexpr uint32_t LONG_BEEP_MS = 200;
    static constexpr uint32_t BEEP_GAP_MS = 80;
    static constexpr uint32_t LETTER_GAP_MS = 400;
    static constexpr uint32_t CYCLE_GAP_MS = 1000;

    struct SOSState {
        bool active;
        uint32_t freq;
        uint32_t sample_rate;
        uint8_t step;
        uint32_t step_start_tick;
        bool continuous;   // true = loop (HIGH/CRITICAL), false = one-shot (MEDIUM)
    };

    static SOSState sos_state_;

    static void start_sos(uint32_t freq, uint32_t sample_rate, bool continuous) noexcept;
};

} // namespace drone_analyzer

#endif // AUDIO_ALERTS_HPP
