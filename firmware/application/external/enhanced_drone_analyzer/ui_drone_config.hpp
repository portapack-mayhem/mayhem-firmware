// ui_drone_config.hpp
// Consolidated constants and configuration for Enhanced Drone Analyzer
// Centralizes all magic numbers, embedded constraints, and tuning parameters

#ifndef __UI_DRONE_CONFIG_HPP__
#define __UI_DRONE_CONFIG_HPP__

#include <cstdint>

namespace ui::external_app::enhanced_drone_analyzer {

// ================================
// HARDWARE CONSTRAINTS & LIMITS
// ================================

/**
 * PORTAPACK EMBEDDED LIMITATIONS
 * Cortex-M4: 256KB Flash, 32KB RAM, No heap allocation preferred
 */

// Scanning thread stack size (bytes)
static constexpr uint32_t SCAN_THREAD_STACK_SIZE = 2048;

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

// ================================
// TIMING & SCHEDULING
// ================================

/**
 * SCANNING FREQUENCIES & INTERVALS
 * Optimized for embedded real-time constraints
 */

// Minimum scan cycle interval (ms) - prevents UI freezing
static constexpr uint32_t MIN_SCAN_INTERVAL_MS = 750;

// Stale drone timeout (ms) - how long before removing inactive drones
static constexpr uint32_t STALE_DRONE_TIMEOUT_MS = 30000;

// Detection table reset interval (scan cycles)
static constexpr uint32_t DETECTION_RESET_INTERVAL = 50;

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

} // namespace ui::external_app::enhanced_drone_analyzer

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_CONFIG_HPP__
