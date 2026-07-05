#ifndef DRONE_TYPES_HPP
#define DRONE_TYPES_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include "freqman_types.hpp"

namespace drone_analyzer {

/**
 * @brief Maximum pattern name length (including null terminator)
 * @note Defined here (not in constants.hpp or pattern_types.hpp) to break circular dependency:
 *       pattern_types.hpp → drone_types.hpp → pattern_types.hpp
 */
constexpr size_t PATTERN_NAME_MAX_LEN = 28;

/**
 * @brief Type alias for frequency in Hz
 */
using FreqHz = uint64_t;

/**
 * @brief Type alias for signal strength (RSSI)
 */
using RssiValue = int32_t;

/**
 * @brief Type alias for system time (ChibiOS)
 */
using SystemTime = uint32_t;

/**
 * @brief Drone type classification
 */
enum class DroneType : uint8_t {
    UNKNOWN = 0,
    DJI = 1,
    PARROT = 2,
    YUNEEC = 3,
    DR_3DR = 4,
    AUTEL = 5,
    HOBBY = 6,
    FPV = 7,
    CUSTOM = 8,
    OTHER = 255
};

/**
 * @brief Threat level classification
 */
enum class ThreatLevel : uint8_t {
    NONE = 0,
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    CRITICAL = 4
};

/**
 * @brief Scanning mode
 * @note SPECTROMETER: Wideband spectrum analysis using 20MHz slices (like Looking Glass)
 */
enum class ScanningMode : uint8_t {
    SEQUENTIAL = 0,
};

/**
 * @brief Scanner state
 */
enum class ScannerState : uint8_t {
    IDLE = 0,
    SCANNING = 1,
    LOCKING = 2,
    TRACKING = 3,
    PAUSED = 4,
    ERROR = 5
};

/**
 * @brief Movement trend for tracked drones
 */
enum class MovementTrend : uint8_t {
    UNKNOWN = 0,
    STATIC = 1,
    APPROACHING = 2,
    RECEDING = 3
};


/**
 * @brief Error codes for EDA operations
 * @note All errors are recoverable or have fallback behavior
 */
enum class ErrorCode : uint8_t {
    SUCCESS = 0,
    
    // Hardware errors (1-9)
    HARDWARE_NOT_INITIALIZED = 1,
    HARDWARE_TIMEOUT = 2,
    HARDWARE_FAILURE = 3,
    SPI_FAILURE = 4,
    PLL_LOCK_FAILURE = 5,
    
    // Database errors (10-19)
    DATABASE_NOT_LOADED = 10,
    DATABASE_LOAD_TIMEOUT = 11,
    DATABASE_CORRUPTED = 12,
    DATABASE_EMPTY = 13,
    DATABASE_FORMAT_INVALID = 14,
    
    // Buffer errors (20-29)
    BUFFER_EMPTY = 20,
    BUFFER_FULL = 21,
    BUFFER_INVALID = 22,
    
    // Synchronization errors (30-39)
    MUTEX_TIMEOUT = 30,
    MUTEX_LOCK_FAILED = 31,
    SEMAPHORE_TIMEOUT = 32,
    
    // Initialization errors (40-49)
    INITIALIZATION_FAILED = 40,
    INITIALIZATION_INCOMPLETE = 41,
    
    // General errors (50-59)
    INVALID_PARAMETER = 50,
    NOT_IMPLEMENTED = 51,
    FILE_SYSTEM_ERROR = 52,
    UNKNOWN_ERROR = 255
};

/**
 * @brief Result type similar to std::optional<T> with error code
 * @note No heap allocation, uses static storage
 * @tparam T Type of value to store
 */
template<typename T>
class ErrorResult {
public:
    ErrorResult() noexcept
        : has_value_(false), error_(ErrorCode::UNKNOWN_ERROR), value_{} {}

    explicit ErrorResult(const T& value) noexcept
        : has_value_(true), error_(ErrorCode::SUCCESS), value_(value) {}

    explicit ErrorResult(ErrorCode error) noexcept
        : has_value_(false), error_(error), value_{} {}

    [[nodiscard]] bool has_value() const noexcept {
        return has_value_;
    }

    [[nodiscard]] bool is_valid() const noexcept {
        return has_value_ || error_ == ErrorCode::SUCCESS;
    }

    [[nodiscard]] ErrorCode error() const noexcept {
        return error_;
    }

    [[nodiscard]] const T& value() const noexcept {
        return value_;
    }

    [[nodiscard]] T value_or(const T& default_value) const noexcept {
        return has_value_ ? value_ : default_value;
    }

    [[nodiscard]] static ErrorResult<T> success(const T& value) noexcept {
        return ErrorResult<T>(value);
    }

    [[nodiscard]] static ErrorResult<T> failure(ErrorCode error) noexcept {
        return ErrorResult<T>(error);
    }

private:
    bool has_value_;
    ErrorCode error_;
    T value_;
};

// ============================================================================
// Mahalanobis Statistics (POD type - no heap allocation)
// ============================================================================

/**
 * @brief Mahalanobis statistics for outlier detection
 * @note POD type - no virtual functions, no heap allocation
 * @note Stored in Q8.8 fixed-point format
 */
struct MahalanobisStatistics {
    using FeatureVector = std::array<int16_t, 2>;

    FeatureVector mean{};           ///< Running mean (Q8.8)
    FeatureVector variance{};       ///< Running variance (Q8.8)
    std::array<FeatureVector, 8> history{};  ///< Sample history
    uint8_t sample_count{0};     ///< Number of samples collected
    uint8_t history_index{0};    ///< Circular buffer index
    FreqHz last_tuned_frequency{0}; ///< Previous tuned frequency for drift measurement
};

template<>
class ErrorResult<void> {
public:
    ErrorResult() noexcept
        : error_(ErrorCode::SUCCESS) {}

    explicit ErrorResult(ErrorCode error) noexcept
        : error_(error) {}

    [[nodiscard]] bool is_valid() const noexcept {
        return error_ == ErrorCode::SUCCESS;
    }

    [[nodiscard]] ErrorCode error() const noexcept {
        return error_;
    }

    [[nodiscard]] static ErrorResult<void> success() noexcept {
        return ErrorResult<void>();
    }

    [[nodiscard]] static ErrorResult<void> failure(ErrorCode error) noexcept {
        return ErrorResult<void>(error);
    }

private:
    ErrorCode error_;
};

// ============================================================================
// Struct Definitions (must appear before functions that use them)
// ============================================================================

/**
 * @brief Configurable RSSI threat level thresholds (dBm)
 * @note Passed to TrackedDrone::update_rssi() for threat classification
 */
struct ThreatThresholds {
    int32_t low;
    int32_t medium;
    int32_t high;
    int32_t critical;
};

/**
 * @brief Tracked drone data structure (56 bytes)
 * @note No virtual functions, no vtable
 * @note Memory layout optimized for cache efficiency
 */
struct TrackedDrone {
    FreqHz frequency;                    // 8 bytes (uint64_t)
    DroneType drone_type;              // 1 byte (uint8_t)
    ThreatLevel threat_level;           // 1 byte (uint8_t)
    uint8_t update_count;              // 1 byte
    SystemTime last_seen;              // 4 bytes (uint32_t on ChibiOS)
    RssiValue rssi;                    // 4 bytes
    int16_t rssi_history_[6];              // 12 bytes (6 × int16_t) — RSSI_HISTORY_SIZE
    SystemTime timestamp_history_[3];       // 12 bytes (3 × uint32_t) — TIMESTAMP_HISTORY_SIZE
    uint8_t history_index_;
    uint8_t missed_cycles_;             // 1 byte — consecutive scans without detection
    int16_t last_rssi_;                 // 2 bytes — RSSI from previous cycle (for decay)
    uint8_t rssi_decrease_counter_;     // 1 bytes — consecutive cycles with RSSI decrease
    bool rssi_increased_;               // 1 byte — set true when new RSSI > previous
    SystemTime last_increase_time_;     // 4 bytes — timestamp when RSSI last increased (for time-based decay)
    SystemTime created_time_;           // 4 bytes — timestamp when drone was first detected (min lifetime)

    // Sweep-aware decay fields (cycle-based for sweep mode)
    SystemTime last_seen_time_;         // 4 bytes — timestamp when drone was last seen (ANY detection)
    uint8_t sweep_cycles_missed_{0};    // 1 byte — consecutive full sweep cycles without seeing this drone
    static constexpr uint8_t MAX_SWEEP_CYCLES_MISSED = 3;  // Allow 3 full cycles (~6-15 sec) before decay

    // ========================================================================
    // Pattern match state
    // ========================================================================

    bool pattern_matched_{false};              // 1 byte — pattern match indicator
    uint16_t pattern_score_{0};                // 2 bytes — similarity score (0-1000)
    char pattern_name_[PATTERN_NAME_MAX_LEN]; // 28 bytes — matched pattern name

    // ========================================================================
    // Mahalanobis statistics
    // ========================================================================

    /**
     * @brief Mahalanobis statistics for this tracked drone
     * @note Used for outlier detection
     */
    MahalanobisStatistics mahalanobis_stats_;

    /**
     * @brief Get reference to Mahalanobis statistics
     */
    [[nodiscard]] MahalanobisStatistics& get_mahalanobis_stats() noexcept {
        return mahalanobis_stats_;
    }

    /**
     * @brief Get const reference to Mahalanobis statistics
     */
    [[nodiscard]] const MahalanobisStatistics& get_mahalanobis_stats() const noexcept {
        return mahalanobis_stats_;
    }

    // Total: 65 bytes base + 48 bytes Mahalanobis = 113 bytes (no vtable, no virtual functions)
    
    /**
     * @brief Default constructor
     */
    TrackedDrone() noexcept;
    
    /**
     * @brief Constructor with initial values
     */
    TrackedDrone(
        FreqHz freq,
        DroneType type,
        ThreatLevel threat
    ) noexcept;
    
    /**
     * @brief Update drone with new RSSI reading
     */
    void update_rssi(RssiValue new_rssi, SystemTime timestamp, const ThreatThresholds& thresholds) noexcept;
    
    /**
     * @brief Get average RSSI from history
     */
    [[nodiscard]] RssiValue get_average_rssi() const noexcept;
    
    /**
     * @brief Get movement trend based on RSSI history
     */
    [[nodiscard]] MovementTrend get_movement_trend() const noexcept;

    /**
     * @brief Get current threat level
     */
    [[nodiscard]] ThreatLevel get_threat() const noexcept {
        return threat_level;
    }

    /**
     * @brief Check if drone is stale (not seen recently)
     */
    [[nodiscard]] bool is_stale(SystemTime current_time, SystemTime timeout_ms) const noexcept;

    /**
     * @brief Decay threat level by one step (CRITICAL→HIGH→MEDIUM→LOW→NONE)
     * @return true if threat is now NONE (should be removed)
     */
    bool decay_threat() noexcept;

    /**
     * @brief Mark drone as seen at current time (for sweep-aware decay)
     * @param now Current system time
     * @note Resets sweep_cycles_missed_ and updates rssi_increased_/last_increase_time_
     */
    void mark_seen(SystemTime now) noexcept {
        last_seen_time_ = now;
        rssi_increased_ = true;
        last_increase_time_ = now;
        sweep_cycles_missed_ = 0;
    }

    /**
     * @brief Set pattern match state on this tracked drone
     * @param score Similarity score (0-1000)
     * @param name Pattern name string (copied internally, truncated to PATTERN_NAME_MAX_LEN-1)
     */
    void set_pattern_match(uint16_t score, const char* name) noexcept {
        pattern_matched_ = true;
        pattern_score_ = score;
        size_t i = 0;
        while (name != nullptr && name[i] != '\0' && i < PATTERN_NAME_MAX_LEN - 1) {
            pattern_name_[i] = name[i];
            ++i;
        }
        pattern_name_[i] = '\0';
    }

    // Pattern match state is overwritten on every match; no explicit clear needed
    // because a drone is removed from tracked_drones_ when its threat decays to NONE.

    /**
     * @brief Increment missed sweep cycle counter
     * @note Called at end of each full sweep cycle when drone was NOT seen
     */
    void increment_missed_cycle() noexcept {
        if (sweep_cycles_missed_ < 255) {
            ++sweep_cycles_missed_;
        }
    }

    /**
     * @brief Reset missed cycles counter (drone was detected)
     */
    void reset_missed() noexcept { missed_cycles_ = 0; }

    /**
     * @brief Increment missed cycles counter
     */
    void increment_missed() noexcept { missed_cycles_++; }

    /**
     * @brief Get missed cycles count
     */
    [[nodiscard]] uint8_t get_missed_cycles() const noexcept { return missed_cycles_; }

    /**
     * @brief Calculate RSSI variance from history (integer arithmetic)
     * @return Variance value (0 if insufficient samples)
     * @note Real drones: variance < 25 (stable signal)
     * @note Noise: variance > 100 (chaotic fluctuations)
     * @note Uses int16_t history, promotes to int32_t for variance calc
     */
    [[nodiscard]] uint32_t calculate_rssi_variance() const noexcept {
        if (history_index_ < 2) return 0;
        const uint8_t n = (history_index_ < 6) ? history_index_ : 6;
        int32_t sum = 0;
        for (uint8_t i = 0; i < n; ++i) {
            sum += static_cast<int32_t>(rssi_history_[i]);
        }
        const int32_t mean = sum / static_cast<int32_t>(n);
        uint32_t variance = 0;
        for (uint8_t i = 0; i < n; ++i) {
            const int32_t diff = static_cast<int32_t>(rssi_history_[i]) - mean;
            variance += static_cast<uint32_t>(diff * diff);
        }
        return variance / static_cast<uint32_t>(n);
    }
};

/**
 * @brief Display drone entry for UI (39 bytes)
 * @note POD type, no vtable
 */
struct DisplayDroneEntry {
    FreqHz frequency;           // 8 bytes
    DroneType type;            // 1 byte (uint8_t)
    ThreatLevel threat;         // 1 byte (uint8_t)
    RssiValue rssi;             // 4 bytes
    SystemTime last_seen;       // 4 bytes
    char type_name[16];         // 16 bytes
    uint32_t display_color;     // 4 bytes (RGBA)
    MovementTrend trend;        // 1 byte (uint8_t)
    bool pattern_matched;         // 1 byte - Pattern match indicator
    uint16_t pattern_score;       // 2 bytes - Similarity score (0-1000)
    char pattern_name[16];       // 16 bytes - Matched pattern name
    
    // Total: 47 bytes (no vtable, POD type)
    
    /**
     * @brief Default constructor
     */
    DisplayDroneEntry() noexcept;
    
    /**
     * @brief Constructor from TrackedDrone
     */
    explicit DisplayDroneEntry(const TrackedDrone& drone) noexcept;
    
    /**
     * @brief Set display color based on threat level
     */
    void set_color_from_threat() noexcept;
    
    /**
     * @brief Get type name string
     */
    [[nodiscard]] const char* get_type_name() const noexcept;
};

/**
 * @brief Scanner state snapshot
 */
struct ScannerStateSnapshot {
    ThreatLevel max_detected_threat;
    size_t approaching_count;
    size_t static_count;
    size_t receding_count;
    bool scanning_active;
    bool is_fresh;
    
    /**
     * @brief Default constructor
     */
    ScannerStateSnapshot() noexcept;
};

/**
 * @brief Display data for UI updates
 */
struct DisplayData {
    DisplayDroneEntry drones[16];     // 16 × 39 = 624 bytes
    size_t drone_count;
    ScannerStateSnapshot state;
    
    /**
     * @brief Default constructor
     */
    DisplayData() noexcept;
    
    /**
     * @brief Clear display data
     */
    void clear() noexcept;
};

// ============================================================================
// FreqmanDB Embedded-Compatible Types
// ============================================================================

/**
 * @brief freqman_type is defined in freqman_types.hpp
 * @note No heap allocation, embedded-safe enum definition
 */

/**
 * @brief Embedded-compatible freqman entry (no std::string, no heap)
 * @note 80 bytes total, aligned for 8-byte access
 * @note Replaces std::string-based freqman_entry from Mayhem core
 */
struct freqman_entry_fixed {
    int64_t frequency_a{0};
    int64_t frequency_b{0};
    char description[32];  // Fixed-size array, no std::string
    freqman_type type{static_cast<freqman_type>(0)};
    uint8_t modulation{255};  // freqman_invalid_index
    uint8_t bandwidth{255};
    uint8_t step{255};
    uint8_t tone{255};
    
    // Total: 8 + 8 + 32 + 1 + 1 + 1 + 1 + 1 = 53 bytes (padded to 56)
    
    /**
     * @brief Default constructor
     */
    freqman_entry_fixed() noexcept;
    
    /**
     * @brief Constructor with values
     */
    freqman_entry_fixed(
        int64_t freq_a,
        int64_t freq_b,
        const char* desc,
        freqman_type t
    ) noexcept;
    
    /**
     * @brief Copy description safely
     * @param src Source string
     * @return ErrorCode::SUCCESS if copied, error otherwise
     */
    [[nodiscard]] ErrorCode set_description(const char* src) noexcept;
    
    /**
     * @brief Check if entry is valid
     */
    [[nodiscard]] bool is_valid() const noexcept;
};

// ============================================================================
// Validation Functions
// ============================================================================

/**
 * @brief Validate spectrum data buffer
 * @param spectrum_data Spectrum data buffer
 * @param length Buffer length
 * @return ErrorCode::SUCCESS if valid, error code otherwise
 */
[[nodiscard]] inline ErrorCode validate_spectrum_buffer(
    const uint8_t* spectrum_data,
    size_t length
) noexcept {
    if (spectrum_data == nullptr) {
        return ErrorCode::BUFFER_INVALID;
    }
    if (length == 0) {
        return ErrorCode::BUFFER_EMPTY;
    }
    if (length > 256) {
        return ErrorCode::BUFFER_INVALID;
    }
    return ErrorCode::SUCCESS;
}

/**
 * @brief Validate drone list buffer
 * @param drones Drone list buffer
 * @param count Number of drones
 * @param max_count Maximum allowed count
 * @return ErrorCode::SUCCESS if valid, error code otherwise
 */
[[nodiscard]] inline ErrorCode validate_drone_buffer(
    const DisplayDroneEntry* drones,
    size_t count,
    size_t max_count
) noexcept {
    if (drones == nullptr) {
        return ErrorCode::BUFFER_INVALID;
    }
    if (count > max_count) {
        return ErrorCode::BUFFER_FULL;
    }
    return ErrorCode::SUCCESS;
}

/**
 * @brief Validate histogram buffer
 * @param histogram Histogram buffer
 * @param length Buffer length
 * @return ErrorCode::SUCCESS if valid, error code otherwise
 */
[[nodiscard]] inline ErrorCode validate_histogram_buffer(
    const uint16_t* histogram,
    size_t length
) noexcept {
    if (histogram == nullptr) {
        return ErrorCode::BUFFER_INVALID;
    }
    if (length == 0) {
        return ErrorCode::BUFFER_EMPTY;
    }
    if (length > 240) {
        return ErrorCode::BUFFER_INVALID;
    }
    return ErrorCode::SUCCESS;
}

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Convert error code to human-readable string
 * @param error Error code to convert
 * @return Static string (Flash storage)
 */
[[nodiscard]] inline const char* error_to_string(ErrorCode error) noexcept {
    switch (error) {
        case ErrorCode::SUCCESS:
            return "Success";
        case ErrorCode::HARDWARE_NOT_INITIALIZED:
            return "Hardware not initialized";
        case ErrorCode::HARDWARE_TIMEOUT:
            return "Hardware timeout";
        case ErrorCode::HARDWARE_FAILURE:
            return "Hardware failure";
        case ErrorCode::SPI_FAILURE:
            return "SPI failure";
        case ErrorCode::PLL_LOCK_FAILURE:
            return "PLL lock failure";
        case ErrorCode::DATABASE_NOT_LOADED:
            return "Database not loaded";
        case ErrorCode::DATABASE_LOAD_TIMEOUT:
            return "Database load timeout";
        case ErrorCode::DATABASE_CORRUPTED:
            return "Database corrupted";
        case ErrorCode::DATABASE_EMPTY:
            return "Database empty";
        case ErrorCode::DATABASE_FORMAT_INVALID:
            return "Database format invalid";
        case ErrorCode::BUFFER_EMPTY:
            return "Buffer empty";
        case ErrorCode::BUFFER_FULL:
            return "Buffer full";
        case ErrorCode::BUFFER_INVALID:
            return "Buffer invalid";
        case ErrorCode::MUTEX_TIMEOUT:
            return "Mutex timeout";
        case ErrorCode::MUTEX_LOCK_FAILED:
            return "Mutex lock failed";
        case ErrorCode::SEMAPHORE_TIMEOUT:
            return "Semaphore timeout";
        case ErrorCode::INITIALIZATION_FAILED:
            return "Initialization failed";
        case ErrorCode::INITIALIZATION_INCOMPLETE:
            return "Initialization incomplete";
        case ErrorCode::INVALID_PARAMETER:
            return "Invalid parameter";
        case ErrorCode::NOT_IMPLEMENTED:
            return "Not implemented";
        case ErrorCode::UNKNOWN_ERROR:
        default:
            return "Unknown error";
    }
}

/**
 * @brief Safe string copy with bounds checking
 * @param dest Destination buffer
 * @param dest_size Destination buffer size (must be >= 2)
 * @param src Source string
 * @return ErrorCode::SUCCESS if copied, error otherwise
 * @note Always null-terminates destination
 * @note No heap allocation, uses inline implementation
 */
[[nodiscard]] inline ErrorCode safe_str_copy(
    char* dest,
    size_t dest_size,
    const char* src
) noexcept {
    if (dest == nullptr || src == nullptr || dest_size < 2) {
        return ErrorCode::BUFFER_INVALID;
    }

    size_t i = 0;
    for (; i < dest_size - 1 && src[i] != '\0'; ++i) {
        dest[i] = src[i];
    }
    dest[i] = '\0';

    return ErrorCode::SUCCESS;
}

/**
 * @brief Convert drone type enum to human-readable string
 * @param type Drone type to convert
 * @return Static string pointer (Flash storage, no heap allocation)
 * @note Returns pointer to constexpr string constants in Flash
 * @note Thread-safe (read-only data)
 */
[[nodiscard]] const char* drone_type_to_string(DroneType type) noexcept;

} // namespace drone_analyzer

#endif // DRONE_TYPES_HPP
