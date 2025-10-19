// Consolidated configuration and types for Enhanced Drone Analyzer
// Merged from ui_drone_core_types.hpp and ui_drone_config_and_presets.hpp

#ifndef __UI_DRONE_CONFIG_HPP__
#define __UI_DRONE_CONFIG_HPP__

#include <cstdint>
#include <vector>
#include <string>

namespace ui::external_app::enhanced_drone_analyzer {

// ================================
// CORE TYPES AND ENUMERATIONS
// ================================

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

// IMPORTANT: Bandwidth presets moved to ui_drone_config.hpp for consolidation

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
        // Now using consolidated config constant instead of local TREND_THRESHOLD_DB
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
    // DIRECT EMBEDDED PATTERN: RSSI validation like Level app
    static bool validate_rssi_signal(int32_t rssi_db, ThreatLevel threat) {
        // Step 1: Hardware range validation (like Level app constraints)
        if (rssi_db < -120 || rssi_db > -10) {
            return false;  // Outside Portapack hardware range
        }

        // Step 2: Threat-based threshold validation (following Recon patterns)
        int8_t detection_threshold;
        switch (threat) {
            case ThreatLevel::CRITICAL:
                detection_threshold = -75;  // Very sensitive for critical threats
                break;
            case ThreatLevel::HIGH:
                detection_threshold = -80;  // Sensitive for high threats
                break;
            case ThreatLevel::MEDIUM:
                detection_threshold = -85;  // Moderate sensitivity
                break;
            case ThreatLevel::LOW:
                detection_threshold = -90;  // Standard civilian sensitivity
                break;
            case ThreatLevel::NONE:
            default:
                detection_threshold = -95;  // Very relaxed for none/unknown
                break;
        }

        return rssi_db >= detection_threshold;
    }

    // ADDITIONAL: Signal strength classification (following Scanner patterns)
    static ThreatLevel classify_signal_strength(int32_t rssi_db) {
        if (rssi_db >= -75) return ThreatLevel::CRITICAL;
        if (rssi_db >= -80) return ThreatLevel::HIGH;
        if (rssi_db >= -85) return ThreatLevel::MEDIUM;
        if (rssi_db >= -90) return ThreatLevel::LOW;
        return ThreatLevel::NONE;
    }

    // ADDITIONAL: Frequency range validation (following Recon safety checks)
    static bool validate_frequency_range(rf::Frequency freq_hz) {
        // Portapack hardware limits (following radio.hpp constraints)
        return freq_hz >= 50000000 && freq_hz <= 6000000000UL;
    }
};

// ================================
// WIDEBAND SCANNING DATA STRUCTURES
// ================================

/**
 * WIDEBAND SLICE STRUCTURE
 * Adapted from Search app slice management pattern
 */

// Single slice for wideband scanning (fixed-size for embedded constraints)
struct WidebandSlice {
    rf::Frequency center_frequency;  // Center frequency of this slice
    uint8_t max_power;               // Maximum RSSI power found in this slice
    int16_t max_index;               // Bin index with maximum power (-1 if none)

    // Constructor - initialize to defaults
    WidebandSlice() : center_frequency(0), max_power(0), max_index(-1) {}
    explicit WidebandSlice(rf::Frequency freq) : center_frequency(freq), max_power(0), max_index(-1) {}
};

// Wideband scan state tracking (embedded-friendly fixed arrays)
struct WidebandScanData {
    WidebandSlice slices[WIDEBAND_MAX_SLICES];  // Fixed array for defined max slices
    uint32_t slices_nb;                          // Current number of active slices
    uint32_t slice_counter;                      // Current slice being processed
    rf::Frequency min_freq;                      // Scanning range minimum
    rf::Frequency max_freq;                      // Scanning range maximum

    // Reset scan state
    void reset() {
        slices_nb = 0;
        slice_counter = 0;
        min_freq = WIDEBAND_DEFAULT_MIN;
        max_freq = WIDEBAND_DEFAULT_MAX;
        // Clear slice data
        for (auto& slice : slices) {
            slice = WidebandSlice();  // Reset to defaults
        }
    }
};

// ================================
// HARDWARE CONSTRAINTS & LIMITS
// ================================

/**
 * PORTAPACK EMBEDDED LIMITATIONS
 * Cortex-M4: 256KB Flash, 32KB RAM, No heap allocation preferred
 */

// Scanning thread stack size (bytes)
static constexpr uint32_t SCAN_THREAD_STACK_SIZE = 2048;

// ================================
// WIDEBAND SCANNING CONSTANTS
// ================================

/**
 * WIDEBAND SCANNING PARAMETERS
 * Adapted from Search app slice-based approach
 */

// Wideband slice bandwidth for frequency range division
static constexpr rf::Frequency WIDEBAND_SLICE_WIDTH = 2500000;  // 2.5MHz per slice (matching Search app)

// FFT bins for spectrum analysis
static constexpr uint32_t WIDEBAND_BIN_NB = 256;                 // FFT power bins

// Bins after DC spike trimming (center 12 bins)
static constexpr uint32_t WIDEBAND_BIN_NB_NO_DC = WIDEBAND_BIN_NB - 16;

// Bin width in Hz
static constexpr uint32_t WIDEBAND_BIN_WIDTH = WIDEBAND_SLICE_WIDTH / WIDEBAND_BIN_NB;

// Default wideband scanning range (1MHz to 6GHz - full range)
static constexpr rf::Frequency WIDEBAND_DEFAULT_MIN = 1000000;     // 1MHz minimum
static constexpr rf::Frequency WIDEBAND_DEFAULT_MAX = 6000000000;  // 6GHz maximum

// Maximum slices per wideband scan (to prevent resource exhaustion)
static constexpr uint32_t WIDEBAND_MAX_SLICES = 32;

// ================================
// SPECTRUM BIN BOUNDS
// ================================

// DC spike filtering (center 12 bins around 122-134)
static constexpr uint8_t WIDEBAND_DC_LOWER_BOUND = 122;
static constexpr uint8_t WIDEBAND_DC_UPPER_BOUND = 134;

// Edge filtering (first/last 2 bins to avoid artifacts)
static constexpr uint8_t WIDEBAND_EDGE_MARGIN = 2;

// Maximum tracked drones (fits in static arrays)
static constexpr size_t MAX_TRACKED_DRONES = 8;

// Frequency database entries (64 is reasonable for drone frequencies)
static constexpr size_t MAX_DATABASE_ENTRIES = 64;

// Spectral analysis buffer size (optimized for performance)
static constexpr size_t SPECTRUM_BUFFER_SIZE = 256;

// Detection hysteresis table size (mem: 512 + 512*4 + 512*2 = 3KB)
static constexpr size_t DETECTION_TABLE_SIZE = 512;

// ================================
// SPECTRUM MODES & BANDWIDTH
// ================================

/**
 * SPECTRUM CONFIGURATION MODES
 * Following Level/Detector bandwidth patterns
 */

// Spectrum bandwidth options (Hz) - matching Portapack capabilities
static constexpr int32_t SPECTRUM_BANDWIDTHS[] = {
    4000000,   // ULTRA_NARROW: Low sensitivity, high resolution
    8000000,   // NARROW: Balanced performance
    12000000,  // MEDIUM: Good for drone analysis (default)
    20000000,  // WIDE: Broad coverage
    24000000   // ULTRA_WIDE: Maximum bandwidth (Looking Glass max)
};

// Matching sampling rates (Hz) - must match bandwidths
static constexpr int32_t SPECTRUM_SAMPLING_RATES[] = {
    4000000,   // ULTRA_NARROW
    8000000,   // NARROW
    12000000,  // MEDIUM
    20000000,  // WIDE
    24000000   // ULTRA_WIDE
};

// ================================
// DETECTION & TRACKING PARAMETERS
// ================================

/**
 * DRONE DETECTION ALGORITHMS
 * Calibrated from field testing and SpecAn measurements
 */

// Minimum consecutive detections to confirm signal (stability)
static constexpr uint8_t MIN_DETECTION_COUNT = 3;

// RSSI hysteresis margin to prevent noise triggers (dB)
static constexpr int32_t HYSTERESIS_MARGIN_DB = 5;

// RSSI trend analysis threshold (dB change required for movement)
static constexpr int16_t TREND_THRESHOLD_DB = 3;

// Weighted average alpha for RSSI smoothing (0.0-1.0)
static constexpr float RSSI_SMOOTHING_ALPHA = 0.3f;

// Default RSSI threshold for unknown frequencies (dB)
static constexpr int32_t DEFAULT_RSSI_THRESHOLD_DB = -90;

// Wideband-specific thresholds (more conservative due to broadband noise)
static constexpr int32_t WIDEBAND_RSSI_THRESHOLD_DB = -85;        // Less sensitive for unknown signals
static constexpr int32_t WIDEBAND_DYNAMIC_THRESHOLD_OFFSET_DB = 5; // Additional offset for dynamic calculations

// ================================
// TIMING & SCHEDULING
// ================================

/**
 * SCANNING FREQUENCIES & INTERVALS
 * Optimized for embedded real-time constraints
 */

// Minimum scan cycle interval (ms) - OPTIMIZED for 60km/h threat response: 500ms
static constexpr uint32_t MIN_SCAN_INTERVAL_MS = 500;

// Stale drone timeout (ms) - how long before removing inactive drones
static constexpr uint32_t STALE_DRONE_TIMEOUT_MS = 30000;

// Detection table reset interval (scan cycles)
static constexpr uint32_t DETECTION_RESET_INTERVAL = 50;

// Hybrid scanning ratio - wideband frequency (OPTIMIZED: every 4th cycle for 60km/h response)
static constexpr uint32_t HYBRID_WIDEBAND_RATIO = 4;  // Every Nth cycle is wideband (was 10)

// ================================
// FREQUENCY DATABASE
// ================================

/**
 * FREQUENCY DATABASE PRESETS
 * Bandwidth presets for different drone types
 */

// Drone channel bandwidth presets (Hz)
static constexpr uint32_t DRONE_BANDWIDTH_PRESETS[] = {
    3000000,  // 0: 3MHz - standard civilian drone channel
    6000000,  // 1: 6MHz - wideband consumer drones
    10000000, // 2: 10MHz - military/various payloads
    20000000, // 3: 20MHz - experimental/commercial
    0         // END MARKER
};

// ================================
// AUDIO & ALERT PARAMETERS
// ================================

/**
 * AUDIO ALERT SYSTEM
 * Beep patterns for threat levels
 */

// SOS audio frequency (Hz) for critical threats
static constexpr uint16_t SOS_FREQUENCY_HZ = 1500;

// Beep duration (ms) for detection alerts
static constexpr uint32_t DETECTION_BEEP_DURATION_MS = 200;

// ================================
// UI & DISPLAY PARAMETERS
// ================================

/**
 * UI CONSTRAINTS & LAYOUT
 * Screen: 240x320, optimized for readability
 */

// Mini spectrum waterfall dimensions (pixels)
static constexpr size_t MINI_SPECTRUM_WIDTH = 120;   // Half screen width
static constexpr size_t MINI_SPECTRUM_HEIGHT = 8;    // 8 lines height
static constexpr uint32_t MINI_SPECTRUM_Y_START = 180; // Vertical position

// Display limits
static constexpr size_t MAX_DISPLAYED_DRONES = 3;    // Show top 3 by RSSI

// Frequency range for mini spectrum display (Hz)
static constexpr rf::Frequency MINI_SPECTRUM_MIN_FREQ = 2400000000ULL; // 2.4GHz
static constexpr rf::Frequency MINI_SPECTRUM_MAX_FREQ = 2500000000ULL; // 2.5GHz

// ================================
// FREQUENCY VALIDATION
// ================================

/**
 * HARDWARE LIMITS & VALIDATION
 * Portapack frequency range constraints
 */

// Absolute frequency limits (Hz) - Portapack hardware constraints
static constexpr rf::Frequency MIN_HARDWARE_FREQ = 50000000ULL;        // 50MHz
static constexpr rf::Frequency MAX_HARDWARE_FREQ = 6000000000ULL;      // 6GHz

/**
 * DRONE-SPECIFIC FREQUENCY LIMITS
 * Following Portapack MAX_UFREQ pattern but with drone safety constraints
 */

// Drone operational frequency limits (Hz) - typical drone bands (240MHz-6GHz)
static constexpr rf::Frequency MIN_DRONE_FREQUENCY = 240000000ULL;     // 240MHz (DJI Mini/Avata)
static constexpr rf::Frequency MAX_DRONE_FREQUENCY = 6000000000ULL;    // 6GHz (military wideband)

// ISM band center frequency (Hz) - default center for drone scanning
static constexpr rf::Frequency ISM_CENTER_FREQ = 433000000ULL;         // 433MHz

// ================================
// THREAT SCORE CALCULATION
// ================================

/**
 * THREAT SCORING WEIGHTS
 * Normalized scoring for combined threat assessment
 */

// RSSI contribution to threat score (0.0-1.0)
static constexpr float RSSI_THREAT_WEIGHT = 0.4f;

// Frequency database contribution
static constexpr float DATABASE_THREAT_WEIGHT = 0.6f;

// ================================
// MEMORY MANAGEMENT
// ================================

/**
 * EMBEDDED MEMORY CONSTRAINS
 * Total size checks for static allocations
 */

// Per-drone tracking structure size (bytes)
static constexpr size_t DRONE_TRACKING_SIZE = 15;  // sizeof(TrackedDrone)

// Total tracking memory (MAX_TRACKED_DRONES * DRONE_TRACKING_SIZE)
static constexpr size_t TOTAL_TRACKING_MEMORY = MAX_TRACKED_DRONES * DRONE_TRACKING_SIZE;

// Detection table memory (bytes)
static constexpr size_t DETECTION_MEMORY_SIZE = DETECTION_TABLE_SIZE * sizeof(uint8_t) * 2;

// ================================
// DRONE PRESETS - MIGRATED FROM ui_drone_presets.hpp/.cpp
// ================================

/**
 * DRONE FREQUENCY PRESETS
 * Predefined frequencies and settings for popular drone models
 */

// Preset structure for drone frequency templates
struct DronePreset {
    std::string display_name;      // Name shown in preset selection menu
    uint32_t frequency_hz;        // Primary frequency
    DroneType drone_type;         // Drone model category
    ThreatLevel threat_level;     // Risk assessment
    int32_t rssi_threshold_db;   // Detection sensitivity
    std::string name_template;    // Generated name template
};

// Collection of known drone frequency presets
// TODO: Replace with actual frequencies provided by user
class DroneFrequencyPresets {
public:
    // Get all available presets for menu display
    static const std::vector<DronePreset>& get_all_presets();

    // Get presets filtered by drone type
    static std::vector<DronePreset> get_presets_by_type(DroneType type);

    // Get preset by display name (for fast lookup)
    static const DronePreset* find_preset_by_name(const std::string& display_name);

    // Get all unique display names for menu construction
    static std::vector<std::string> get_preset_names();

private:
    // STATIC PRESET DATA - TO BE REPLACED WITH USER-PROVIDED FREQUENCIES
    // Example presets (will be updated with real data)
    static const std::vector<DronePreset> s_presets;
};

// Preset selector UI helper class
class DronePresetSelector {
public:
    static void show_preset_menu(NavigationView& nav,
                                std::function<void(const DronePreset&)> on_selected);
};

// REAL DRONE PRESETS - Based on actual drone frequency database
// All frequencies converted from MHz/GHz to Hz
static const std::vector<DronePreset> DroneFrequencyPresets::s_presets = {

    // ===== CIVILIAN/COMMERCIAL DRONES =====

    // DJI Consumer drones (most common consumer UAVs)
    {"DJI Mini 4 Pro",       2400000000, DroneType::DJI_MINI,   ThreatLevel::NONE,     -75, "DJI Mini 4 Pro"},
    {"DJI Mini 4 Pro (5.8GHz)", 5800000000, DroneType::DJI_MINI,   ThreatLevel::NONE,     -75, "DJI Mini 4 Pro (Video)"},
    {"DJI Mavic 3 Pro",      2400000000, DroneType::DJI_MAVIC,  ThreatLevel::NONE,     -70, "DJI Mavic 3 Pro"},
    {"DJI Mavic 3 Pro (5.8GHz)", 5800000000, DroneType::DJI_MAVIC,  ThreatLevel::NONE,     -70, "DJI Mavic 3 Pro (Video)"},

    // Autel UAVs (commercial competitor to DJI)
    {"Autel EVO Max 4T",     902000000,  DroneType::DJI_MAVIC,  ThreatLevel::NONE,     -72, "Autel EVO Max 4T"},
    {"Autel EVO Max 4T (2.4GHz)", 2400000000, DroneType::DJI_MAVIC,  ThreatLevel::NONE,     -72, "Autel EVO Max 4T (2.4GHz)"},
    {"Autel EVO Max 4T (5.8GHz)", 5800000000, DroneType::DJI_MAVIC,  ThreatLevel::NONE,     -72, "Autel EVO Max 4T (Video)"},

    // Parrot drones (European manufacturer)
    {"Parrot ANAFI USA",     2400000000, DroneType::UNKNOWN,   ThreatLevel::NONE,     -78, "Parrot ANAFI USA"},
    {"Parrot ANAFI USA (Video)", 5800000000, DroneType::UNKNOWN,   ThreatLevel::NONE,     -78, "Parrot ANAFI USA (5.8GHz)"},

    // FPV Racing drones (hobby/sport drones)
    {"Analog FPV (2.4GHz)",  2400000000, DroneType::FPV_RACING, ThreatLevel::NONE,     -80, "Analog FPV Racing"},
    {"Digital FPV (5.8GHz)", 5800000000, DroneType::FPV_RACING, ThreatLevel::NONE,     -75, "Digital FPV Racing"},
    {"Long Range FPV (433MHz)", 433920000,  DroneType::FPV_RACING, ThreatLevel::NONE,     -85, "Long Range FPV"},
    {"Long Range FPV (868MHz)", 868000000,  DroneType::FPV_RACING, ThreatLevel::NONE,     -82, "Long Range FPV (868MHz)"},

    // Skydio autonomous drones
    {"Skydio X10 (5.8GHz)",  5800000000, DroneType::UNKNOWN,   ThreatLevel::NONE,     -75, "Skydio X10"},

    // ===== MILITARY/TACTICAL DRONES =====

    // Russian tactical UAVs (high threat)
    {"Orlan-10 (200MHz)",    200000000,  DroneType::ORLAN_10,  ThreatLevel::HIGH,    -85, "Orlan-10 Tactical"},
    {"Orlan-10 (433MHz)",    433920000,  DroneType::ORLAN_10,  ThreatLevel::HIGH,    -85, "Orlan-10 Tactical"},
    {"Orlan-10 (868MHz)",    868000000,  DroneType::ORLAN_10,  ThreatLevel::HIGH,    -85, "Orlan-10 Tactical"},
    {"Orlan-10 (915MHz)",    915000000,  DroneType::ORLAN_10,  ThreatLevel::HIGH,    -85, "Orlan-10 Tactical"},
    {"Orlan-10 (1.2GHz)",    1200000000, DroneType::ORLAN_10,  ThreatLevel::HIGH,    -80, "Orlan-10 Tactical"},
    {"Orlan-10 (2.4GHz)",    2400000000, DroneType::ORLAN_10,  ThreatLevel::HIGH,    -80, "Orlan-10 Tactical"},
    {"Orlan-10 (Video)",     900000000,  DroneType::ORLAN_10,  ThreatLevel::HIGH,    -82, "Orlan-10 Telemetry"},

    // Russian precision munitions (critical threat)
    {"Lancet-3 (900MHz)",    900000000,  DroneType::LANCET,    ThreatLevel::CRITICAL,-82, "Lancet-3 Loitering"},
    {"Lancet-3 (5.8GHz)",    5800000000, DroneType::LANCET,    ThreatLevel::CRITICAL,-75, "Lancet-3 Loitering"},
    {"Shahed-131/136 (800MHz)", 800000000, DroneType::LANCET,    ThreatLevel::CRITICAL,-85, "Shahed Loitering"},
    {"Shahed-131/136 (2.6GHz)", 2600000000, DroneType::LANCET,    ThreatLevel::CRITICAL,-78, "Shahed Loitering"},

    // Eleron tactical UAV
    {"Eleron-3SV (868MHz)",  868000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "Eleron-3SV Tactical"},
    {"Eleron-3SV (915MHz)",  915000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "Eleron-3SV Tactical"},
    {"Eleron-3SV (1.2GHz)",  1200000000, DroneType::UNKNOWN,   ThreatLevel::HIGH,    -82, "Eleron-3SV Tactical"},

    // Forpost tactical UAV (Ukrainian/Russian variant)
    {"Forpost-R (465MHz)",   465000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "Forpost-R Tactical"},
    {"Forpost-R (5.8GHz)",   5800000000, DroneType::UNKNOWN,   ThreatLevel::HIGH,    -75, "Forpost-R Tactical"},

    // Bayraktar Turkish MALE UAV (very high threat)
    {"Bayraktar TB2 (868MHz)", 868000000,  DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-85, "Bayraktar TB2 MALE"},
    {"Bayraktar TB2 (915MHz)", 915000000,  DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-85, "Bayraktar TB2 MALE"},
    {"Bayraktar TB2 (2.4GHz)", 2400000000, DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-80, "Bayraktar TB2 MALE"},
    {"Bayraktar TB2 (5.8GHz)", 5800000000, DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-75, "Bayraktar TB2 MALE"},

    // Chinese MALE/export UAVs (high threat for regional conflicts)
    {"CH-4B Rainbow (900MHz)", 900000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "CH-4B Rainbow MALE"},
    {"CH-4B Rainbow (4.4GHz)", 4400000000, DroneType::UNKNOWN,   ThreatLevel::HIGH,    -78, "CH-4B Rainbow MALE"},
    {"CH-5 Rainbow (841MHz)",  841000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "CH-5 Rainbow MALE"},
    {"CH-5 Rainbow (1.4GHz)",  1400000000, DroneType::UNKNOWN,   ThreatLevel::HIGH,    -80, "CH-5 Rainbow MALE"},

    // Israeli tactical UAVs
    {"Hermes 450 (UHF)",     600000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "Hermes 450 Tactical"},
    {"Hermes 450 (C-band)",  5000000000, DroneType::UNKNOWN,   ThreatLevel::HIGH,    -75, "Hermes 450 Tactical"},

    // US military drones (maximum threat)
    {"MQ-9 Reaper (UHF)",    300000000,  DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-90, "MQ-9 Reaper HALE"},
    {"MQ-9 Reaper (C-band)", 5500000000, DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-75, "MQ-9 Reaper HALE"},
    {"RQ-4 Global Hawk (UHF)", 300000000, DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-90, "RQ-4 Global Hawk HALE"},
    {"RQ-4 Global Hawk (Ku)", 13000000000,DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-70, "RQ-4 Global Hawk HALE"},

    // Precision munitions
    {"Switchblade 600",      1300000000, DroneType::LANCET,    ThreatLevel::CRITICAL,-82, "Switchblade 600 Munition"},

    // Ground vehicles (lower priority but still tracked)
    {"Marker UGV (2.4GHz)",  2400000000, DroneType::UNKNOWN,   ThreatLevel::MEDIUM,  -80, "Marker UGV"},
    {"Marker UGV (LTE 700)", 700000000,  DroneType::UNKNOWN,   ThreatLevel::MEDIUM,  -85, "Marker UGV"},
    {"Uran-6 UGV",           400000000,  DroneType::UNKNOWN,   ThreatLevel::MEDIUM,  -85, "Uran-6 UGV"},

    // SATCOM frequencies (for long-range drones)
    {"Military SATCOM (UHF)", 300000000,  DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-88, "Military SATCOM UHF"},
    {"Military SATCOM (C-band)", 5500000000, DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-75, "Military SATCOM C-band"},
    {"Military SATCOM (Ku)", 13000000000, DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-70, "Military SATCOM Ku-band"},
};

// Preset implementation
const std::vector<DronePreset>& DroneFrequencyPresets::get_all_presets() {
    return s_presets;
}

std::vector<DronePreset> DroneFrequencyPresets::get_presets_by_type(DroneType type) {
    std::vector<DronePreset> filtered;
    for (const auto& preset : s_presets) {
        if (preset.drone_type == type) {
            filtered.push_back(preset);
        }
    }
    return filtered;
}

const DronePreset* DroneFrequencyPresets::find_preset_by_name(const std::string& display_name) {
    for (const auto& preset : s_presets) {
        if (preset.display_name == display_name) {
            return &preset;
        }
    }
    return nullptr;
}

std::vector<std::string> DroneFrequencyPresets::get_preset_names() {
    std::vector<std::string> names;
    for (const auto& preset : s_presets) {
        names.push_back(preset.display_name);
    }
    return names;
}

// Preset selector implementation
void DronePresetSelector::show_preset_menu(NavigationView& nav,
                                          std::function<void(const DronePreset&)> on_selected) {
    // Create menu with preset options
    auto preset_names = DroneFrequencyPresets::get_preset_names();

    // Create menu view dynamically
    class PresetMenuView : public MenuView {
    public:
        PresetMenuView(std::vector<std::string> names, std::function<void(const DronePreset&)> callback)
            : MenuView({"Select Drone Preset", Theme::getInstance()->fg_light->foreground, nullptr}),
              on_selected_fn(callback), preset_names(names) {
        }

        virtual bool on_key(const KeyEvent key) override {
            if (key == KeyEvent::Up) {
                set_dirty();
                return true;
            }
            if (key == KeyEvent::Down) {
                set_dirty();
                return true;
            }
            if (key == KeyEvent::Select) {
                // Get selected name
                size_t idx = get_selected_index();
                if (idx < preset_names.size()) {
                    const DronePreset* preset = DroneFrequencyPresets::find_preset_by_name(preset_names[idx]);
                    if (preset && on_selected_fn) {
                        on_selected_fn(*preset);
                    }
                }
                return true;
            }
            return MenuView::on_key(key);
        }

    private:
        std::function<void(const DronePreset&)> on_selected_fn;
        std::vector<std::string> preset_names;
    };

    nav.push<PresetMenuView>(preset_names, on_selected);
}

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_CONFIG_HPP__
