// ui_drone_common_types.hpp - Shared types and enums for both Settings and Scanner apps
// Created during migration: Split of monolithic EDA into Settings + Scanner

#ifndef __UI_DRONE_COMMON_TYPES_HPP__
#define __UI_DRONE_COMMON_TYPES_HPP__

#include <cstdint>
#include <string>
#include <vector>

// Forward declarations for external types
using Frequency = uint64_t;

// Basic enums shared between apps
enum class ThreatLevel {
    NONE,
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

enum class DroneType {
    UNKNOWN,
    MAVIC,
    PHANTOM,
    DJI_MINI,
    PARROT_ANAFI,
    PARROT_BEBOP,
    PX4_DRONE,
    MILITARY_DRONE
};

enum class MovementTrend {
    UNKNOWN,
    STATIC,
    APPROACHING,
    RECEDING
};

enum class SpectrumMode {
    NARROW,
    MEDIUM,
    WIDE,
    ULTRA_WIDE
};

// Struct definitions - moved from various headers
struct TrackedDrone {
    uint32_t frequency;
    uint8_t drone_type;
    uint8_t threat_level;
    uint8_t update_count;
    systime_t last_seen;
};

// Detection entry from scanner
struct DisplayDroneEntry {
    Frequency frequency;
    DroneType type;
    ThreatLevel threat;
    int32_t rssi;
    systime_t last_seen;
    std::string type_name;
    Color display_color;
    MovementTrend trend;
};

// Preset definition for frequency management
struct DronePreset {
    std::string display_name;
    std::string name_template;
    Frequency frequency_hz;
    ThreatLevel threat_level;
    DroneType drone_type;

    bool is_valid() const {
        return !display_name.empty() && frequency_hz > 0;
    }
};

// Constants shared across apps
static constexpr size_t MAX_TRACKED_DRONES = 8;
static constexpr size_t MAX_DISPLAYED_DRONES = 3;
static constexpr size_t MINI_SPECTRUM_WIDTH = 200;
static constexpr size_t MINI_SPECTRUM_HEIGHT = 24;
static constexpr uint32_t MIN_HARDWARE_FREQ = 1'000'000;     // 1MHz
static constexpr uint32_t MAX_HARDWARE_FREQ = 6'000'000'000; // 6GHz
static constexpr uint32_t WIDEBAND_DEFAULT_MIN = 2'400'000'000ULL; // 2.4GHz
static constexpr uint32_t WIDEBAND_DEFAULT_MAX = 2'500'000'000ULL; // 2.5GHz
static constexpr uint32_t WIDEBAND_SLICE_WIDTH = 25'000'000;  // 25MHz
static constexpr uint32_t WIDEBAND_MAX_SLICES = 20;
static constexpr int32_t DEFAULT_RSSI_THRESHOLD_DB = -90;
static constexpr int32_t WIDEBAND_RSSI_THRESHOLD_DB = -95;
static constexpr int32_t WIDEBAND_DYNAMIC_THRESHOLD_OFFSET_DB = 5;
static constexpr uint8_t MIN_DETECTION_COUNT = 3; // Activates tracking
static constexpr int32_t HYSTERESIS_MARGIN_DB = 5;

// Type aliases
using freqman_entry = struct {
    uint64_t frequency_a;  // Start frequency
    uint64_t frequency_b;  // End frequency (single for single freq)
    uint8_t type;          // Single, Range, HamRadio, etc.
    uint16_t modulation;   // NFM, AM, etc.
    uint16_t bandwidth;    // 25kHz, etc.
    uint16_t step;         // 25kHz step
    std::string description;
    std::string tonal;
};

// Detection ring buffer constants
static constexpr size_t DETECTION_TABLE_SIZE = 256;

// Wideband scanning slice
struct WidebandSlice {
    Frequency center_frequency;
    size_t index;
};

// Wideband scan data structure
struct WidebandScanData {
    Frequency min_freq;
    Frequency max_freq;
    size_t slices_nb;
    WidebandSlice slices[20]; // MAX_SLICES
    size_t slice_counter;

    void reset() {
        min_freq = WIDEBAND_DEFAULT_MIN;
        max_freq = WIDEBAND_DEFAULT_MAX;
        slices_nb = 0;
        slice_counter = 0;
    }
};

struct DetectionLogEntry {
    uint32_t timestamp;
    uint32_t frequency_hz;
    int32_t rssi_db;
    ThreatLevel threat_level;
    DroneType drone_type;
    uint8_t detection_count;
    float confidence_score;
};

#endif // __UI_DRONE_COMMON_TYPES_HPP__
