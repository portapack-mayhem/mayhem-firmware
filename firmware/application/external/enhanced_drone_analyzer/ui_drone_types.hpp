// ui_drone_types.hpp
// Common type definitions for Enhanced Drone Analyzer
// Unified types to eliminate duplications

#ifndef __UI_DRONE_TYPES_HPP__
#define __UI_DRONE_TYPES_HPP__

#include <cstdint>

// DRONE TYPE ENUMERATION
enum class DroneType {
    UNKNOWN = 0,
    DJI_MAVIC,
    DJI_MINI,
    DJI_AIR,
    DJI_PHANTOM,
    DJI_AVATA,
    FPV_RACING,
    FPV_FREESTYLE,
    ORLAN_10,
    LANCET,
    SHAHED_136,
    BAYRAKTAR_TB2,
    MILITARY_UAV,
    RUSSIAN_EW_STATION,
    FIBER_OPTIC_DRONE,
    SVOY,        // Claims to be friendly
    CHUZHOY,     // Enemy
    HUYLO,
    HUYLINA,
    PVD,
    VSU,
    BRAT,
    KROT,
    PLENNYY,
    NE_PLENNYY,
    CODE_300,
    CODE_200,
    CODE_500,
    MAX_TYPES
};

enum class ThreatLevel {
    NONE = 0,
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    CRITICAL = 4
};

// PORTAPACK EMBEDDED TRACKING SYSTEM - Hardware-compatible implementation
// Optimized for Cortex-M4 constraints (16KB RAM, no heap allocation)
enum class MovementTrend : uint8_t {
    STATIC = 0,      // RSSI стабильен (+/- 3dB)
    APPROACHING = 1, // RSSI растет (дрон приближается)
    RECEDING = 2,    // RSSI падает (дрон удаляется)
    UNKNOWN = 3      // Недостаточно данных для анализа
};

    // PRODUCTION FIX #3: OPTIMIZE DATABASE FOOTPRINT
    // Remove duplicate bandwidth fields (cleanup)
    // Remove unused offset fields (embedde constraints)

    // FREQUENCY DATABASE ENTRY - OPTIMIZED FOR PRODUCTION
    struct DroneFrequencyEntry {
        uint32_t frequency_hz;          // Core frequency in Hz
        uint8_t drone_type_idx;         // Compact ref to DroneType enum
        uint8_t threat_level_idx;       // Compact ref to ThreatLevel enum
        int8_t rssi_threshold_db;       // Signed byte for threshold
        uint8_t bandwidth_idx;          // Index to bandwidth preset (saves space)
        uint16_t name_offset;           // Offset in string pool (future optimization)

        DroneFrequencyEntry(uint32_t freq, DroneType type, ThreatLevel threat,
                           int32_t rssi_thresh, uint32_t bw_hz, const char* desc)
            : frequency_hz(freq),
              drone_type_idx(static_cast<uint8_t>(type)),
              threat_level_idx(static_cast<uint8_t>(threat)),
              rssi_threshold_db(static_cast<int8_t>(rssi_thresh)),
              bandwidth_idx(0),  // Default - map to actual bandwidth later
              name_offset(0) {}  // Placeholder for name pooling

        // Production validation
        bool is_valid() const {
            return frequency_hz >= 50000000 && frequency_hz <= 6000000000UL &&
                   rssi_threshold_db >= -120 && rssi_threshold_db <= -20;
        }

        // PRODUCTION: Add conversion methods for compact storage
        DroneType get_drone_type() const {
            return static_cast<DroneType>(drone_type_idx);
        }

        ThreatLevel get_threat_level() const {
            return static_cast<ThreatLevel>(threat_level_idx);
        }
    };

// PRODUCTION: Add preset bandwidth mappings to save space
static constexpr uint32_t DRONE_BANDWIDTH_PRESETS[] = {
    3000000,  // 0: 3MHz - standard drone channel
    6000000,  // 1: 6MHz - wideband civilian drones
    10000000, // 2: 10MHz - military/various payloads
    20000000, // 3: 20MHz - experimental/commercial
    0         // END MARKER
};

// EMBEDDED-FRIENDLY: Fixed-size structure (15 bytes), no dynamic allocation
struct TrackedDrone {
    uint32_t frequency;       // 4 bytes: Частота в Hz (433MHz = 433000000)
    int16_t last_rssi;        // 2 bytes: Последний RSSI (-120 to -20 dB)
    int16_t prev_rssi;        // 2 bytes: Предыдущий RSSI для тренда
    systime_t last_seen;      // 4 bytes: ChibiOS system time
    uint8_t drone_type;       // 1 byte: DroneType enum as uint8_t
    uint8_t trend_history;    // 1 byte: Битовая маска последних 4 трендов
    uint8_t update_count;     // 1 byte: Количество обновлений (0 = free slot)

    // EMBEDDED CONSTRUCTOR: Initialize to zero (free slot)
    TrackedDrone() : frequency(0), last_rssi(-120), prev_rssi(-120),
                     last_seen(0), drone_type(0), trend_history(0), update_count(0) {}

    // ADD RSSI WITH TREND CALCULATION - Embedded optimized
    void add_rssi(int16_t rssi, systime_t time) {
        prev_rssi = last_rssi;
        last_rssi = rssi;
        last_seen = time;

        if (update_count < 255) update_count++; // Prevent overflow

        // MINIMUM 2 measurements for trend calculation
        if (update_count >= 2) {
            calculate_simple_trend();
        }
    }

    // GET CURRENT TREND - Extract from bit mask
    MovementTrend get_trend() const {
        return static_cast<MovementTrend>(trend_history & 0x03); // Last 2 bits
    }

private:
    // CALCULATE TREND BASED ON RSSI CHANGE - Hardware optimized
    void calculate_simple_trend() {
        const int16_t TREND_THRESHOLD_DB = 3; // 3dB minimum change
        int16_t diff = last_rssi - prev_rssi;

        // PORTAPACK CONSTRAINT: Gradual trend accumulation
        // Store last 4 trends in 8-bit mask (2 bits per trend)
        MovementTrend new_trend = MovementTrend::STATIC;

        if (diff > TREND_THRESHOLD_DB) {
            new_trend = MovementTrend::APPROACHING;  // Signal stronger = closer
        } else if (diff < -TREND_THRESHOLD_DB) {
            new_trend = MovementTrend::RECEDING;     // Signal weaker = farther
        }

        // BIT SHIFT: Rotate history mask left by 2, add new trend
        trend_history = (trend_history << 2) | static_cast<uint8_t>(new_trend);
    }
};

// INTEGRATED: Direct Level/Recon pattern validation
// Using embedded-friendly structures like in Scanner/Recon/Recon
struct SimpleDroneValidation {
    // EMBEDDED PATTERN: Direct RSSI validation like Level app
    static bool validate_rssi_signal(int32_t rssi_db, ThreatLevel threat) {
        // DIRECT from Level/Recon pattern: Simple threshold checks
        if (rssi_db < -100 || rssi_db > -20) return false;  // Like Level hardware limits

        // EMBEDDED: Simple threat-based threshold like in Scanner
        int8_t min_rssi = (threat >= ThreatLevel::HIGH) ? -85 : -90;
        return rssi_db > min_rssi;
    }
};

#endif // __UI_DRONE_TYPES_HPP__
