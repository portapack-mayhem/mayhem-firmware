#ifndef SETTINGS_MANAGER_HPP
#define SETTINGS_MANAGER_HPP

#include <cstdint>
#include <cstddef>

#include "drone_types.hpp"
#include "constants.hpp"
#include "scanner.hpp"

namespace drone_analyzer {

/**
 * @brief Unified settings data structure (all settings + sweep in one POD)
 * @note Single source of truth — replaces duplicated DroneSettings + ScanConfig sweep fields
 * @note ~120 bytes total, stack-safe for local usage
 * @note No heap allocation, no virtual functions
 */
struct SettingsStruct {
    // Scanning
    ScanningMode scanning_mode;
    uint32_t scan_interval_ms;
    uint8_t scan_sensitivity;
    int32_t alert_rssi_threshold_dbm;
    int32_t threat_low_dbm;
    int32_t threat_medium_dbm;
    int32_t threat_high_dbm;
    int32_t threat_critical_dbm;

    // Display
    bool spectrum_visible;
    bool histogram_visible;

    // Alerts
    bool audio_alerts_enabled;
    uint8_t volume{50};

    // Detection features
    bool dwell_enabled;
    bool confirm_count_enabled;
    bool noise_blacklist_enabled;
    bool spectrum_detection_enabled;
    bool median_enabled;

    // Spectrum shape filter
    uint8_t spectrum_margin;
    uint8_t spectrum_min_width;
    uint8_t spectrum_max_width;
    uint8_t spectrum_peak_sharpness;
    uint8_t spectrum_peak_ratio;
    uint8_t spectrum_valley_depth;
    uint8_t spectrum_flatness;
    uint8_t spectrum_symmetry;

    // CFAR detection
    CFARMode cfar_mode;
    uint8_t cfar_ref_cells;
    uint8_t cfar_guard_cells;
    uint8_t cfar_threshold_x10;

    // CFAR extended parameters (Hybrid weights, OS-CFAR k, VI-CFAR threshold)
    uint8_t cfar_hybrid_alpha{DEFAULT_CFAR_HYBRID_ALPHA};
    uint8_t cfar_hybrid_beta{DEFAULT_CFAR_HYBRID_BETA};
    uint8_t cfar_hybrid_gamma{DEFAULT_CFAR_HYBRID_GAMMA};
    uint8_t os_cfar_k_percent{DEFAULT_OS_CFAR_K_PERCENT};
    uint8_t vi_cfar_threshold_x10{DEFAULT_VI_CFAR_THRESHOLD_X10};

    // Anti-false-positive
    int32_t neighbor_margin_db;
    bool rssi_variance_enabled;
    uint8_t confirm_count;

    // Mahalanobis Gate Filter
    bool mahalanobis_enabled;
    uint8_t mahalanobis_threshold_x10;

    // Pattern matching
    bool pattern_matching_enabled;

    // Sweep window 1
    FreqHz sweep_start_freq;
    FreqHz sweep_end_freq;
    FreqHz sweep_step_freq;

    // Sweep window 2
    FreqHz sweep2_start_freq;
    FreqHz sweep2_end_freq;
    FreqHz sweep2_step_freq;
    bool sweep2_enabled;

    // Sweep window 3
    FreqHz sweep3_start_freq;
    FreqHz sweep3_end_freq;
    FreqHz sweep3_step_freq;
    bool sweep3_enabled;

    // Sweep window 4
    FreqHz sweep4_start_freq;
    FreqHz sweep4_end_freq;
    FreqHz sweep4_step_freq;
    bool sweep4_enabled;

    // Sweep exception frequencies (per window, 0 = unused)
    FreqHz sweep_exceptions[4][EXCEPTIONS_PER_WINDOW]{};
    uint8_t exception_radius_mhz{DEFAULT_EXCEPTION_RADIUS_MHZ};  // 1-100 MHz exclusion radius
    uint8_t rssi_decrease_cycles{5};  // sweep cycles of RSSI decrease before threat decay

    SettingsStruct() noexcept;
};

/**
 * @brief Centralized settings file manager for EDA
 * @note Single parser, single save path — replaces 3 duplicated parsers
 * @note All settings (general + sweep) in one file: SETTINGS/eda_settings.txt
 * @note No heap allocation, no exceptions
 */
class SettingsFileManager {
public:
    /**
     * @brief Load all settings from SD card into SettingsStruct
     * @param out Destination for loaded settings
     * @return ErrorCode::SUCCESS if loaded, error code otherwise
     * @note Missing file is not an error — out retains constructor defaults
     */
    [[nodiscard]] static ErrorCode load(SettingsStruct& out) noexcept;

    /**
     * @brief Save all settings (general + sweep) to SD card
     * @param scanner_ptr Scanner to read current config from (may be nullptr)
     * @param general_settings General settings to save
     * @return ErrorCode::SUCCESS if saved, error code otherwise
     * @note Reads sweep config from scanner if available, otherwise from general_settings
     */
    [[nodiscard]] static ErrorCode save(
        DroneScanner* scanner_ptr,
        const SettingsStruct& general_settings
    ) noexcept;

    /**
     * @brief Apply loaded settings to ScanConfig
     * @param settings Source settings
     * @param config Destination ScanConfig to update
     */
    static void apply_to_config(
        const SettingsStruct& settings,
        ScanConfig& config
    ) noexcept;

    /**
     * @brief Extract settings from ScanConfig (for view initialization)
     * @param config Source ScanConfig
     * @param settings Destination SettingsStruct to update
     */
    static void extract_from_config(
        const ScanConfig& config,
        SettingsStruct& settings
    ) noexcept;
};

} // namespace drone_analyzer

#endif // SETTINGS_MANAGER_HPP
