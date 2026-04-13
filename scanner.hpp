#ifndef SCANNER_HPP
#define SCANNER_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include "ch.h"
#include "drone_types.hpp"
#include "locking.hpp"
#include "constants.hpp"
#include "database.hpp"
#include "hardware_controller.hpp"
#include "audio_alerts.hpp"
#include "rssi_detector.hpp"
#include "histogram_processor.hpp"
#include "median_filter.hpp"
#include "message.hpp"
#include "spectrum_shape.hpp"
#include "mahalanobis_gate.hpp"

namespace drone_analyzer {

/**
 * @brief Scan configuration
 * @note ScannerState is defined in drone_types.hpp
 */
struct ScanConfig {
    ScanningMode mode;
    FreqHz start_frequency;
    FreqHz end_frequency;
    uint32_t scan_interval_ms;
    int32_t rssi_threshold_dbm;
    uint32_t stale_timeout_ms;
    
    // Sweep range (Hz) — window 1
    FreqHz sweep_start_freq;
    FreqHz sweep_end_freq;
    FreqHz sweep_step_freq;

    // Sweep range (Hz) — window 2 (independent, optional)
    FreqHz sweep2_start_freq{2400000000ULL};
    FreqHz sweep2_end_freq{2500000000ULL};
    FreqHz sweep2_step_freq{20000000};
    bool sweep2_enabled{false};

    // Sweep range (Hz) — window 3 (independent, optional, disabled by default)
    FreqHz sweep3_start_freq{900000000ULL};
    FreqHz sweep3_end_freq{1000000000ULL};
    FreqHz sweep3_step_freq{20000000};
    bool sweep3_enabled{false};

    // Sweep range (Hz) — window 4 (independent, optional, disabled by default)
    FreqHz sweep4_start_freq{1200000000ULL};
    FreqHz sweep4_end_freq{1300000000ULL};
    FreqHz sweep4_step_freq{20000000};
    bool sweep4_enabled{false};

    // Advanced detection features (OFF by default)
    bool dwell_enabled{false};           // Stay on frequency when signal detected
    bool confirm_count_enabled{false};   // Require multiple confirmations before creating drone
    bool noise_blacklist_enabled{false}; // Skip frequencies with persistent noise
    bool spectrum_detection_enabled{false}; // Detect drone signals by spectrum shape (U/V peaks)
    bool median_enabled{true};              // Median filter for RSSI spike rejection (ON by default)
    uint8_t spectrum_margin{15};            // Peak margin above noise (0-200, default 15 ≈ 5 dB)
    uint8_t spectrum_min_width{1};          // Min signal width in bins (1-20, default 1)
    uint8_t spectrum_max_width{DEFAULT_SPECTRUM_MAX_WIDTH};            // Max signal width (reject flat U/I shapes)
    uint8_t spectrum_peak_sharpness{DEFAULT_SPECTRUM_PEAK_SHARPNESS};  // Min peak sharpness ratio (enforce V-shape)
    uint8_t spectrum_peak_ratio{DEFAULT_SPECTRUM_PEAK_RATIO};          // Peak-to-width ratio (inverted-V filter)
    uint8_t spectrum_valley_depth{DEFAULT_SPECTRUM_VALLEY_DEPTH};      // Valley depth threshold (V-shape flanks)
    uint8_t spectrum_flatness{DEFAULT_SPECTRUM_FLATNESS};              // Peak-to-average ratio (reject flat-top WiFi/FM)
    uint8_t spectrum_symmetry{DEFAULT_SPECTRUM_SYMMETRY};              // Left/right width symmetry % (reject asymmetric noise)

    // New anti-false-positive features
    int32_t neighbor_margin_db{DEFAULT_NEIGHBOR_MARGIN_DB};  // 0=disabled, 3=default
    bool rssi_variance_enabled{false};                        // RSSI variance noise rejection
    uint8_t confirm_count{DEFAULT_CONFIRM_COUNT};             // Configurable confirm count

    // CFAR detection (Constant False Alarm Rate)
    CFARMode cfar_mode{DEFAULT_CFAR_MODE};                    // CFAR mode (OFF/CA/GO/SO/HYBRID/OS/VI)
    uint8_t cfar_ref_cells{DEFAULT_CFAR_REF_CELLS};          // Reference cells (8-64)
    uint8_t cfar_guard_cells{DEFAULT_CFAR_GUARD_CELLS};      // Guard cells (0-8)
    uint8_t cfar_threshold_x10{DEFAULT_CFAR_THRESHOLD_X10};  // Threshold ×10 (10-100 = 1.0-10.0)
    uint8_t cfar_hybrid_alpha{DEFAULT_CFAR_HYBRID_ALPHA};    // CA weight (0-100)
    uint8_t cfar_hybrid_beta{DEFAULT_CFAR_HYBRID_BETA};      // GO weight (0-100)
    uint8_t cfar_hybrid_gamma{DEFAULT_CFAR_HYBRID_GAMMA};    // SO weight (0-100)
    uint8_t os_cfar_k_percent{DEFAULT_OS_CFAR_K_PERCENT};    // OS-CFAR k-th order (50-90%)
    uint8_t vi_cfar_threshold_x10{DEFAULT_VI_CFAR_THRESHOLD_X10};  // VI-CFAR threshold ×10 (5-50)

    // ========================================================================
    // Mahalanobis Gate Filter
    // ========================================================================

    /**
     * @brief Enable Mahalanobis gate validation
     */
    bool mahalanobis_enabled{false};

    /**
     * @brief Mahalanobis threshold ×10 (e.g., 30 = 3.0)
     */
    uint8_t mahalanobis_threshold_x10{DEFAULT_MAHALOBIS_THRESHOLD_X10};

    // Sweep exception frequencies (per window, 0 = unused slot)
    FreqHz sweep_exceptions[4][EXCEPTIONS_PER_WINDOW]{};
    uint8_t exception_radius_mhz{DEFAULT_EXCEPTION_RADIUS_MHZ};  // 1-100, configurable exclusion radius
    uint8_t rssi_decrease_cycles{5};  // sweep cycles of RSSI decrease before threat decay

    /**
     * @brief Default constructor
     */
    ScanConfig() noexcept;
    
    /**
     * @brief Constructor with values
     */
    ScanConfig(ScanningMode m, FreqHz start, FreqHz end) noexcept;
};

/**
 * @brief Scan statistics
 */
struct ScanStatistics {
    uint32_t total_scan_cycles;
    uint32_t successful_cycles;
    uint32_t failed_cycles;
    uint32_t drones_detected;
    uint32_t max_rssi_dbm;
    
    /**
     * @brief Default constructor
     */
    ScanStatistics() noexcept;
    
    /**
     * @brief Reset statistics
     */
    void reset() noexcept;
};

// ============================================================================
// Multi-Zone Sweep Structures
// ============================================================================

/**
 * @brief Number of sweep zones (4 horizontal display bands)
 */
constexpr uint8_t SWEEP_ZONE_COUNT = 4;

/**
 * @brief Per-zone sweep configuration
 */
struct SweepZoneConfig {
    FreqHz start_freq{0};
    FreqHz end_freq{0};
    bool enabled{false};
};

/**
 * @brief Multi-zone sweep configuration (stored in SD card settings)
 */
struct SweepZonesConfig {
    SweepZoneConfig zones[SWEEP_ZONE_COUNT];

    SweepZonesConfig() noexcept {
        // Default: zone 0 = 2.4 GHz ISM, others disabled
        zones[0] = {2400000000ULL, 2500000000ULL, true};
        zones[1] = {5700000000ULL, 5800000000ULL, false};
        zones[2] = {5800000000ULL, 5900000000ULL, false};
        zones[3] = {1000000000ULL, 1100000000ULL, false};
    }
};

/**
 * @brief Per-zone sweep runtime state
 */
struct SweepZoneRuntime {
    FreqHz current_center{0};
    FreqHz pixel_step_hz{0};
    FreqHz step_hz{0};
    FreqHz bins_hz_acc{0};
    FreqHz center_ini{0};
    uint16_t pixel_index{0};
    uint8_t pixel_max{0};

    void init(const SweepZoneConfig& cfg) noexcept {
        constexpr FreqHz SLICE_BW = 20000000;
        constexpr FreqHz BIN_SIZE = SLICE_BW / FFT_BIN_COUNT;

        const FreqHz range = cfg.end_freq - cfg.start_freq;
        pixel_step_hz = (range > 0) ? range / SWEEP_PIXELS_PER_SLICE : 0;
        step_hz = SWEEP_BINS_PER_STEP * BIN_SIZE;
        center_ini = cfg.start_freq - (2 * BIN_SIZE) + (SLICE_BW / 2);
        current_center = center_ini;
        pixel_index = 0;
        pixel_max = 0;
        bins_hz_acc = 0;
    }

    void reset_pass() noexcept {
        current_center = center_ini;
        pixel_index = 0;
        pixel_max = 0;
        bins_hz_acc = 0;
    }

    [[nodiscard]] bool is_complete() const noexcept {
        return pixel_index >= SWEEP_PIXELS_PER_SLICE;
    }
};

/**
 * @brief CFAR (Constant False Alarm Rate) detector
 * @note Adapts detection threshold to local noise level
 * @note Supports CA-CFAR, GO-CFAR, SO-CFAR, and Hybrid modes
 * @note Formula: F_CFAR(i) = 1 if P(i) > T * (1/N_ref) * sum(P(k))
 * @note T = G * (N_ref^(1/N_ref) - 1) for CA-CFAR
 * @note Hybrid: w_hybrid = α*w_CA + β*w_GO + γ*w_SO
 */
class CFARDetector {
public:
    /**
     * @brief Run CFAR detection on spectrum data
     * @param spectrum FFT spectrum data (0-255 power values)
     * @param bin_count Total number of bins
     * @param cbin Cell Under Test index
     * @param mode CFAR mode (CA/GO/SO/HYBRID/OS/VI)
     * @param ref_cells Number of reference cells (N_ref)
     * @param guard_cells Number of guard cells
     * @param threshold_x10 Threshold multiplier ×10 (e.g., 50 = 5.0)
     * @param alpha CA weight for hybrid mode ×100
     * @param beta GO weight for hybrid mode ×100
     * @param gamma SO weight for hybrid mode ×100
     * @param os_k_percent OS-CFAR k-th order percentile (50-90)
     * @param vi_threshold_x10 VI-CFAR variability threshold ×10 (5-50)
     * @return true if signal detected (power > adaptive threshold)
     */
    [[nodiscard]] static bool detect(
        const uint8_t* spectrum,
        size_t bin_count,
        size_t cbin,
        CFARMode mode,
        uint8_t ref_cells,
        uint8_t guard_cells,
        uint8_t threshold_x10,
        uint8_t alpha = 50,
        uint8_t beta = 30,
        uint8_t gamma = 20,
        uint8_t os_k_percent = 75,
        uint8_t vi_threshold_x10 = 15
    ) noexcept {
        if (mode == CFARMode::OFF) return false;
        if (spectrum == nullptr || bin_count == 0) return false;
        if (cbin >= bin_count) return false;

        // Guard: ref_cells must be reasonable
        if (ref_cells < CFAR_REF_CELLS_MIN) ref_cells = CFAR_REF_CELLS_MIN;
        if (ref_cells > CFAR_REF_CELLS_MAX) ref_cells = CFAR_REF_CELLS_MAX;
        if (guard_cells > CFAR_GUARD_CELLS_MAX) guard_cells = CFAR_GUARD_CELLS_MAX;

        // Calculate reference window boundaries
        // Left window: [cbin - guard_cells - ref_cells ... cbin - guard_cells - 1]
        // Right window: [cbin + guard_cells + 1 ... cbin + guard_cells + ref_cells]
        const int32_t total_span = static_cast<int32_t>(guard_cells + ref_cells);
        
        // Sum left reference window
        int32_t left_sum = 0;
        int32_t left_count = 0;
        for (int32_t k = static_cast<int32_t>(cbin) - total_span; 
             k < static_cast<int32_t>(cbin) - static_cast<int32_t>(guard_cells); ++k) {
            if (k >= 0 && k < static_cast<int32_t>(bin_count)) {
                // Skip DC spike region
                if (k >= static_cast<int32_t>(FFT_DC_SPIKE_START) && 
                    k < static_cast<int32_t>(FFT_DC_SPIKE_END)) continue;
                left_sum += spectrum[k];
                ++left_count;
            }
        }

        // Sum right reference window
        int32_t right_sum = 0;
        int32_t right_count = 0;
        for (int32_t k = static_cast<int32_t>(cbin) + static_cast<int32_t>(guard_cells) + 1;
             k <= static_cast<int32_t>(cbin) + total_span; ++k) {
            if (k >= 0 && k < static_cast<int32_t>(bin_count)) {
                // Skip DC spike region
                if (k >= static_cast<int32_t>(FFT_DC_SPIKE_START) && 
                    k < static_cast<int32_t>(FFT_DC_SPIKE_END)) continue;
                right_sum += spectrum[k];
                ++right_count;
            }
        }

        // Need at least one reference cell on each side
        if (left_count == 0 || right_count == 0) return false;

        // Compute noise estimates for each CFAR mode
        // CA-CFAR: average of both windows
        const int32_t ca_noise = (left_sum + right_sum) / (left_count + right_count);
        
        // GO-CFAR: maximum of the two window averages
        const int32_t left_avg = (left_count > 0) ? left_sum / left_count : 0;
        const int32_t right_avg = (right_count > 0) ? right_sum / right_count : 0;
        const int32_t go_noise = (left_avg > right_avg) ? left_avg : right_avg;
        
        // SO-CFAR: minimum of the two window averages
        const int32_t so_noise = (left_avg < right_avg) ? left_avg : right_avg;

        // Compute final noise estimate based on mode
        int32_t noise_estimate = 0;
        switch (mode) {
            case CFARMode::CA:
                noise_estimate = ca_noise;
                break;
            case CFARMode::GO:
                noise_estimate = go_noise;
                break;
            case CFARMode::SO:
                noise_estimate = so_noise;
                break;
            case CFARMode::HYBRID: {
                // Hybrid: w_hybrid = α*w_CA + β*w_GO + γ*w_SO
                // Weights are ×100, so divide by 100 at the end
                const int32_t weighted = 
                    static_cast<int32_t>(alpha) * ca_noise +
                    static_cast<int32_t>(beta) * go_noise +
                    static_cast<int32_t>(gamma) * so_noise;
                noise_estimate = weighted / 100;
                break;
            }
            case CFARMode::OS: {
                // OS-CFAR (Ordered Statistic): collect all reference cells, sort, take k-th value
                // Better in multi-target environments (resists masking from nearby signals)
                // Use a small stack buffer to collect and sort reference cells
                uint8_t ref_buf[CFAR_REF_CELLS_MAX * 2];  // Max 128 cells (64 left + 64 right)
                size_t ref_idx = 0;
                
                // Collect left window cells
                for (int32_t k = static_cast<int32_t>(cbin) - total_span;
                     k < static_cast<int32_t>(cbin) - static_cast<int32_t>(guard_cells) && 
                     ref_idx < sizeof(ref_buf); ++k) {
                    if (k >= 0 && k < static_cast<int32_t>(bin_count)) {
                        if (k >= static_cast<int32_t>(FFT_DC_SPIKE_START) &&
                            k < static_cast<int32_t>(FFT_DC_SPIKE_END)) continue;
                        ref_buf[ref_idx++] = spectrum[k];
                    }
                }
                // Collect right window cells
                for (int32_t k = static_cast<int32_t>(cbin) + static_cast<int32_t>(guard_cells) + 1;
                     k <= static_cast<int32_t>(cbin) + total_span &&
                     ref_idx < sizeof(ref_buf); ++k) {
                    if (k >= 0 && k < static_cast<int32_t>(bin_count)) {
                        if (k >= static_cast<int32_t>(FFT_DC_SPIKE_START) &&
                            k < static_cast<int32_t>(FFT_DC_SPIKE_END)) continue;
                        ref_buf[ref_idx++] = spectrum[k];
                    }
                }
                
                if (ref_idx == 0) return false;
                
                // Insertion sort (small array, O(n²) is acceptable)
                for (size_t i = 1; i < ref_idx; ++i) {
                    const uint8_t key = ref_buf[i];
                    size_t j = i;
                    while (j > 0 && ref_buf[j - 1] > key) {
                        ref_buf[j] = ref_buf[j - 1];
                        --j;
                    }
                    ref_buf[j] = key;
                }
                
                // Select k-th order statistic: k = (N_ref * os_k_percent) / 100
                const size_t k_idx = (ref_idx * os_k_percent) / 100;
                const size_t k_safe = (k_idx < ref_idx) ? k_idx : ref_idx - 1;
                noise_estimate = static_cast<int32_t>(ref_buf[k_safe]);
                break;
            }
            case CFARMode::VI: {
                // VI-CFAR (Variability Index): adaptively select CA/GO/SO based on local statistics
                // VI = variance / mean^2 measures clutter homogeneity
                // Low VI → homogeneous → CA-CFAR (best noise estimate)
                // High VI → clutter edge → GO-CFAR (robust at edges) or SO-CFAR (in clutter)
                
                // Compute mean and variance for left window
                int32_t left_mean = (left_count > 0) ? left_sum / left_count : 0;
                int32_t left_var = 0;
                if (left_count > 1) {
                    for (int32_t k = static_cast<int32_t>(cbin) - total_span;
                         k < static_cast<int32_t>(cbin) - static_cast<int32_t>(guard_cells); ++k) {
                        if (k >= 0 && k < static_cast<int32_t>(bin_count)) {
                            if (k >= static_cast<int32_t>(FFT_DC_SPIKE_START) &&
                                k < static_cast<int32_t>(FFT_DC_SPIKE_END)) continue;
                            const int32_t diff = static_cast<int32_t>(spectrum[k]) - left_mean;
                            left_var += diff * diff;
                        }
                    }
                    left_var /= static_cast<int32_t>(left_count);
                }
                
                // Compute mean and variance for right window
                int32_t right_mean = (right_count > 0) ? right_sum / right_count : 0;
                int32_t right_var = 0;
                if (right_count > 1) {
                    for (int32_t k = static_cast<int32_t>(cbin) + static_cast<int32_t>(guard_cells) + 1;
                         k <= static_cast<int32_t>(cbin) + total_span; ++k) {
                        if (k >= 0 && k < static_cast<int32_t>(bin_count)) {
                            if (k >= static_cast<int32_t>(FFT_DC_SPIKE_START) &&
                                k < static_cast<int32_t>(FFT_DC_SPIKE_END)) continue;
                            const int32_t diff = static_cast<int32_t>(spectrum[k]) - right_mean;
                            right_var += diff * diff;
                        }
                    }
                    right_var /= static_cast<int32_t>(right_count);
                }
                
                // Variability Index: VI = variance / mean^2 (×1000 for integer precision)
                // To avoid division by zero, use max(mean, 1)
                const int32_t left_mean_safe = (left_mean > 0) ? left_mean : 1;
                const int32_t right_mean_safe = (right_mean > 0) ? right_mean : 1;
                const int32_t left_vi = (left_var * 1000) / (left_mean_safe * left_mean_safe);
                const int32_t right_vi = (right_var * 1000) / (right_mean_safe * right_mean_safe);
                
                // vi_threshold_x10 is threshold × 10, compare with VI × 1000
                // So: vi_threshold × 100 = vi_threshold_x10 × 10
                const int32_t vi_threshold = static_cast<int32_t>(vi_threshold_x10) * 100;
                
                // Adaptive mode selection based on variability
                if (left_vi < vi_threshold && right_vi < vi_threshold) {
                    // Both windows homogeneous → CA-CFAR
                    noise_estimate = ca_noise;
                } else if (left_vi >= vi_threshold && right_vi >= vi_threshold) {
                    // Both windows have clutter → GO-CFAR (more conservative)
                    noise_estimate = go_noise;
                } else {
                    // One window has clutter → SO-CFAR (use cleaner window)
                    noise_estimate = so_noise;
                }
                break;
            }
            default:
                return false;
        }

        // Compute adaptive threshold: T_adaptive = G * noise_estimate
        // threshold_x10 is G × 10, so: threshold = threshold_x10 * noise_estimate / 10
        const int32_t adaptive_threshold = 
            (static_cast<int32_t>(threshold_x10) * noise_estimate) / 10;

        // Signal detected if CUT power > adaptive threshold
        return static_cast<int32_t>(spectrum[cbin]) > adaptive_threshold;
    }

    /**
     * @brief Run CFAR on entire spectrum and return peak bin
     * @param spectrum FFT spectrum data
     * @param bin_count Total bins
     * @param mode CFAR mode
     * @param ref_cells Reference cells
     * @param guard_cells Guard cells
     * @param threshold_x10 Threshold ×10
     * @param skip_start Skip bins from start (for edge/DC)
     * @param skip_end Skip bins from end (for edge)
     * @param alpha CA weight for hybrid mode ×100
     * @param beta GO weight for hybrid mode ×100
     * @param gamma SO weight for hybrid mode ×100
     * @param os_k_percent OS-CFAR k-th order percentile (50-90)
     * @param vi_threshold_x10 VI-CFAR variability threshold ×10 (5-50)
     * @return Peak bin index that passed CFAR, or bin_count if none detected
     */
    [[nodiscard]] static size_t find_peak_cfar(
        const uint8_t* spectrum,
        size_t bin_count,
        CFARMode mode,
        uint8_t ref_cells,
        uint8_t guard_cells,
        uint8_t threshold_x10,
        size_t skip_start,
        size_t skip_end,
        uint8_t alpha = 50,
        uint8_t beta = 30,
        uint8_t gamma = 20,
        uint8_t os_k_percent = 75,
        uint8_t vi_threshold_x10 = 15
    ) noexcept {
        if (mode == CFARMode::OFF || spectrum == nullptr) return bin_count;
        
        size_t peak_bin = bin_count;
        uint8_t peak_power = 0;

        for (size_t i = skip_start; i < bin_count - skip_end; ++i) {
            // Skip DC spike
            if (i >= FFT_DC_SPIKE_START && i < FFT_DC_SPIKE_END) continue;
            
            if (detect(spectrum, bin_count, i, mode, ref_cells, guard_cells, 
                       threshold_x10, alpha, beta, gamma, os_k_percent, vi_threshold_x10)) {
                if (spectrum[i] > peak_power) {
                    peak_power = spectrum[i];
                    peak_bin = i;
                }
            }
        }
        return peak_bin;
    }
};

/**
 * @brief Neighbor frequency margin checker (anti-false-positive)
 * @note Stores last N frequency/RSSI pairs in circular buffer
 * @note When signal detected, checks if current freq is stronger than neighbors
 * @note Eliminates wideband noise false positives (WiFi, BT, microwave)
 * @note Inspired by FPV detect app's MIN_NEIGHBOR_MARGIN_FOR_LOCK_DB
 */
class NeighborMarginChecker {
public:
    static constexpr size_t WINDOW = 5;

    /**
     * @brief Add frequency/RSSI sample
     * @param freq Tuned frequency
     * @param rssi RSSI in dBm
     */
    void add(FreqHz freq, int32_t rssi) noexcept {
        history_[head_] = {freq, rssi};
        head_ = (head_ + 1) % WINDOW;
        if (count_ < WINDOW) count_++;
    }

    /**
     * @brief Check if current frequency is stronger than neighbors
     * @param current_freq Current tuned frequency
     * @param current_rssi Current RSSI in dBm
     * @param min_margin_db Minimum dB margin over strongest neighbor
     * @return true if current freq dominates neighbors, false if wideband noise
     */
    [[nodiscard]] bool check_margin(FreqHz current_freq, int32_t current_rssi, int32_t min_margin_db) const noexcept {
        if (count_ < 2) return true;  // Not enough data — pass through
        int32_t best_neighbor_rssi = -120;
        for (uint8_t i = 0; i < count_; ++i) {
            if (history_[i].freq != current_freq && history_[i].rssi > best_neighbor_rssi) {
                best_neighbor_rssi = history_[i].rssi;
            }
        }
        return (current_rssi - best_neighbor_rssi) >= min_margin_db;
    }

    /**
     * @brief Reset checker state
     */
    void reset() noexcept {
        head_ = 0;
        count_ = 0;
    }

private:
    struct Entry { FreqHz freq; int32_t rssi; };
    Entry history_[WINDOW]{};
    uint8_t head_{0};
    uint8_t count_{0};
};

/**
 * @brief Alert callback function type
 * @param threat_level Threat level based on RSSI
 * @note Audio tone varies by threat: CRITICAL=1500Hz, HIGH=1200Hz, MEDIUM=1000Hz
 */
using ThreatAlertCallback = void(*)(ThreatLevel threat_level);

/**
 * @brief Drone scanner
 * @note Main scanning logic for drone detection
 * @note Simplified: removed wideband/hybrid/panoramic modes, removed FHSS detection
 * @note Thread-safe with mutex protection
 */
class DroneScanner {
public:
    /**
     * @brief Constructor
     * @param database Reference to database manager
     * @param hardware Reference to hardware controller
     */
    DroneScanner(DatabaseManager& database, HardwareController& hardware) noexcept;
    
    /**
     * @brief Destructor
     */
    ~DroneScanner() noexcept;
    
    // Delete copy and move operations
    DroneScanner(const DroneScanner&) = delete;
    DroneScanner& operator=(const DroneScanner&) = delete;
    DroneScanner(DroneScanner&&) = delete;
    DroneScanner& operator=(DroneScanner&&) = delete;
    
    /**
     * @brief Initialize scanner
     * @return ErrorCode::SUCCESS if initialized, error code otherwise
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] ErrorCode initialize() noexcept;
    
    /**
     * @brief Start scanning
     * @return ErrorCode::SUCCESS if started, error code otherwise
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] ErrorCode start_scanning() noexcept;
    
    /**
     * @brief Stop scanning
     * @return ErrorCode::SUCCESS if stopped, error code otherwise
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] ErrorCode stop_scanning() noexcept;
    
    /**
     * @brief Pause scanning
     * @return ErrorCode::SUCCESS if paused, error code otherwise
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] ErrorCode pause_scanning() noexcept;
    
    /**
     * @brief Resume scanning from paused state
     * @return ErrorCode::SUCCESS if resumed, error code otherwise
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] ErrorCode resume_scanning() noexcept;

    /**
     * @brief Force resume scanning from LOCKING/TRACKING state
     * @note Called by scanner thread when max dwell time expires
     * @note Uses AtomicFlag trick to avoid mutex deadlock
     *       (scanner thread already can't hold the mutex)
     */
    void force_resume_scanning() noexcept;

    /**
     * @brief Consume force-resume flag and transition to SCANNING
     * @return true if flag was set and state transitioned, false otherwise
     * @note Thread-safe: uses AtomicFlag test_and_clear + mutex for state
     * @note Called by scanner thread BEFORE dwell logic to break out of LOCKING
     */
    bool try_consume_force_resume_flag() noexcept;
    
    /**
     * @brief Remove tracked drone on a specific frequency (no mutex)
     * @note Called by scanner thread after force-resume
     */
    void remove_drone_on_frequency(FreqHz frequency) noexcept;

    /**
     * @brief Increment noise count for a frequency (blacklist tracking)
     * @note Called by scanner thread when force-resuming from noise
     */
    void increment_noise_count(FreqHz frequency) noexcept;

    /**
     * @brief Reset noise count for a frequency (real signal confirmed)
     */
    void reset_noise_count(FreqHz frequency) noexcept;

    /**
     * @brief Check if frequency is blacklisted (persistent noise)
     */
    bool is_blacklisted(FreqHz frequency) const noexcept;
    
    /**
     * @brief Perform single scan cycle (frequency hop)
     * @note Called periodically by scanner thread
     * @return ErrorCode::SUCCESS if cycle completed, error code otherwise
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     * @note ChibiOS mutexes are recursive per-thread: nested calls from the
     *       same thread (perform_scan_cycle → perform_scan_cycle_internal)
     *       succeed without deadlock.
     * @note This method only advances the frequency; RSSI detection is done
     *       by the UI thread via process_spectrum_message().
     */
    [[nodiscard]] ErrorCode perform_scan_cycle() noexcept;
    
    /**
     * @brief Update tracked drones with new data
     * @param frequency Frequency of detected signal
     * @param rssi RSSI value
     * @param timestamp Timestamp of detection
     * @return ErrorCode::SUCCESS if updated, error code otherwise
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] ErrorCode update_tracked_drones(
        FreqHz frequency,
        RssiValue rssi,
        SystemTime timestamp
    ) noexcept;

    /**
     * @brief Process spectrum data and extract RSSI
     * @param spectrum Channel spectrum data
     * @param current_frequency Current tuned frequency (for tracking)
     * @return ErrorResult containing RSSI value or error
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     * @note Extracts maximum power from spectrum and converts to dBm
     * @note Updates tracked drones if RSSI above threshold
     */
    [[nodiscard]] ErrorResult<RssiValue> process_spectrum_data(
        const ChannelSpectrum& spectrum,
        FreqHz current_frequency
    ) noexcept;
    
    /**
     * @brief Process ChannelSpectrum message directly
     * @param spectrum Channel spectrum data
     * @return ErrorCode::SUCCESS if processed, error code otherwise
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     * @note Uses scanner's internal current_frequency
     * @note Updates tracked drones if RSSI above threshold
     */
    [[nodiscard]] ErrorCode process_spectrum_message(const ChannelSpectrum& spectrum) noexcept;

    /**
     * @brief Process spectrum with explicit frequency (avoids race with scanner thread)
     * @param spectrum Channel spectrum data
     * @param frequency Frequency this spectrum corresponds to
     * @return ErrorCode::SUCCESS if processed, error code otherwise
     */
    [[nodiscard]] ErrorCode process_spectrum_message(const ChannelSpectrum& spectrum, FreqHz frequency) noexcept;

    /**
     * @brief Get current frequency for spectrum association (thread-safe)
     * @return Current frequency under mutex lock
     */
    [[nodiscard]] FreqHz get_spectrum_frequency() noexcept;

    [[nodiscard]] size_t get_tracked_drones(
        TrackedDrone* drones,
        size_t max_count
    ) const noexcept;
    
    /**
     * @brief Get scanner state
     * @return Current scanner state
     */
    [[nodiscard]] ScannerState get_state() const noexcept;
    
    /**
     * @brief Check if scanning is active
     * @return true if scanning, false otherwise
     */
    [[nodiscard]] bool is_scanning() const noexcept;
    
    /**
     * @brief Get scan configuration
     * @return Current scan configuration
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] ScanConfig get_config() const noexcept;
    
    /**
     * @brief Set scan configuration
     * @param config Configuration to apply
     * @return ErrorCode::SUCCESS if set, error code otherwise
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] ErrorCode set_config(const ScanConfig& config) noexcept;
    
    /**
     * @brief Get scan statistics
     * @return Current scan statistics
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] ScanStatistics get_statistics() const noexcept;
    
    /**
     * @brief Reset scan statistics
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    void reset_statistics() noexcept;
    
    /**
     * @brief Get current scan frequency
     * @return ErrorResult containing current frequency or error
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] ErrorResult<FreqHz> get_current_frequency() const noexcept;

    /**
     * @brief Set scan frequency directly (for continue after sweep)
     * @param frequency Frequency to set
     * @note After sweep exit, continue scanning from this frequency
     */
    void set_scan_frequency(FreqHz frequency) noexcept;

    /**
     * @brief Clear lock state (LOCKING/TRACKING → SCANNING, reset lock counters)
     * @note Called before entering sweep mode to prevent stale lock after resume
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    void clear_lock_state() noexcept;
    
    /**
     * @brief Get number of tracked drones
     * @return Number of tracked drones
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] size_t get_tracked_count() const noexcept;

    // ========================================================================
    // Fast Scanner Integration Methods
    // ========================================================================

    /**
     * @brief Get frequency lock count
     * @return Current lock count
     * @note Thread-safe: acquires mutex (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] uint32_t get_freq_lock_count() const noexcept;

    /**
     * @brief Set frequency lock count
     * @param count New lock count
     * @note Thread-safe: acquires mutex (LockOrder::DATA_MUTEX)
     */
    void set_freq_lock_count(uint32_t count) noexcept;

    /**
     * @brief Get locked frequency
     * @return Currently locked frequency (0 if not locked)
     * @note Thread-safe: acquires mutex (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] FreqHz get_locked_frequency() const noexcept;

    /**
     * @brief Get current drone type string
     * @param buffer Destination buffer for drone type string
     * @param buffer_size Size of destination buffer (must be >= 2)
     * @return ErrorCode::SUCCESS if copied, error otherwise
     * @note Only valid during LOCKING state
     * @note Thread-safe: acquires mutex (LockOrder::DATA_MUTEX)
     * @note Copies to caller's buffer while holding mutex to prevent race conditions
     */
    [[nodiscard]] ErrorCode get_current_drone_type(char* buffer, size_t buffer_size) const noexcept;

    /**
     * @brief Clear all tracked drones
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    void clear_tracked_drones() noexcept;

    /**
     * @brief Reset scanner frequency to first database entry
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    void reset_frequency() noexcept;

    /**
     * @brief Remove drones not seen since stale timeout
     * @param current_time Current system time
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    void remove_stale_drones(SystemTime current_time) noexcept;

    /**
     * @brief Set the alert callback function
     * @param callback Function to call when alerts are triggered
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    void set_alert_callback(ThreatAlertCallback callback) noexcept;

    /**
     * @brief Enable or disable median filter for RSSI spike rejection
     * @param enabled true to enable, false to disable
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    void set_median_filter_enabled(bool enabled) noexcept;

    /**
     * @brief Reset neighbor margin checker state
     * @note Called on frequency change to prevent stale neighbor data
     */
    void reset_neighbor_checker() noexcept;

    /**
     * @brief Request dwell hold (called by UI thread when signal detected)
     * @note Thread-safe: uses AtomicFlag (lock-free)
     * @note Tells scanner thread to skip frequency hop on next cycle
     */
    void request_dwell() noexcept;

    /**
     * @brief Reset dwell cycle counter
     * @note Called when entering sweep mode to clear stale dwell state
     */
    void reset_dwell_cycles() noexcept { dwell_cycles_ = 0; }

    /**
     * @brief Check if scanner is currently in dwell (holding frequency)
     * @return true if dwelling, false if scanning normally
     */
    [[nodiscard]] bool is_dwelling() const noexcept { return dwell_cycles_ > 0; }

    /**
     * @brief Get filtered RSSI through median filter
     * @return Filtered or raw RSSI
     */
    [[nodiscard]] int32_t get_filtered_rssi() const noexcept {
        return median_filter_enabled_ ? rssi_median_filter_.get_median() : 0;
    }

    /**
     * @brief Set current frequency for sweep mode
     * @note Used by UI sweep loop to keep scanner frequency in sync
     * @note No mutex — called from UI thread during sweep (scanner thread stopped)
     */
    void set_sweep_frequency(FreqHz freq) noexcept {
        current_frequency_ = freq;
    }

    /**
     * @brief Convert FFT bin index (Looking Glass reordering) to actual RF frequency
     * @param f_center Slice center frequency (Hz)
     * @param bin FFT bin index (0-255, after Looking Glass reordering)
     * @return Actual RF frequency for this bin (Hz)
     * @note Looking Glass reordering: bin 0 = Nyquist, bin 128 = DC.
     *       Bins 134-253 (lower sideband): freq = f_center + (bin-256)*BIN_SIZE
     *       Bins 2-119 (upper sideband):   freq = f_center + (bin-126)*BIN_SIZE
     *       Bins 120-133 and 0-1, 254-255 are DC spike / edge (should be skipped)
     */
    static FreqHz fft_bin_to_freq(FreqHz f_center, size_t bin) noexcept {
        constexpr FreqHz SLICE_BW = 20000000;
        constexpr FreqHz BIN_SIZE = SLICE_BW / FFT_BIN_COUNT;  // 78125
        // Looking Glass bin reordering: bin 0 = Nyquist, bin 128 = DC.
        // Lower sideband (bin >= 136): freq = f_center - 120*BIN_SIZE + bin*BIN_SIZE
        //   Avoids negative cast: (bin-256) would overflow uint64_t.
        // Upper sideband (bin < 120):  freq = f_center - 126*BIN_SIZE + bin*BIN_SIZE
        //   Avoids negative cast: (bin-126) would overflow uint64_t.
        if (bin >= FFT_DC_SPIKE_END) {
            return f_center - 120 * BIN_SIZE + static_cast<FreqHz>(bin) * BIN_SIZE;
        }
        return f_center - 126 * BIN_SIZE + static_cast<FreqHz>(bin) * BIN_SIZE;
    }

    /**
     * @brief Lightweight spectrum processing for sweep mode
     * @param spectrum Channel spectrum data (256 bins)
     * @param center_freq Current slice center frequency
     * @param f_min Minimum frequency of sweep range (0 = no range check)
     * @param f_max Maximum frequency of sweep range (0 = no range check)
     * @note Uses UNIFIED 40-bin window (100-119, 136-155) — same as spectrum mode.
     *       This ensures LNA/VGA tuning affects both modes identically.
     * @note Full spectrum shape analysis: margin + min_width + max_width + peak_sharpness + peak_ratio + valley_depth
     * @note Called from UI thread during sweep (scanner thread stopped, no mutex needed)
     * @note Implementation in scanner.cpp — NOT inline (200+ lines, too large for header)
     */
    void process_spectrum_sweep(const ChannelSpectrum& spectrum, FreqHz center_freq, FreqHz f_min = 0, FreqHz f_max = 0) noexcept;

    /**
     * @brief Apply RSSI-based threat decay (time-based, unified for normal and sweep modes)
     * @note Each drone: if RSSI did not increase for decay_threshold_ms (CYC × 1000ms),
     *       decay threat by one step. If RSSI increased, reset timer.
     * @note Enforces minimum drone lifetime of DRONE_STALE_TIMEOUT_MS (5s) before removal.
     * @note Resets rssi_increased_ flag after each call.
     * @note Called from perform_scan_cycle_internal() (normal mode) and on_sweep_spectrum() (sweep mode)
     */
    void apply_rssi_decay() noexcept {
        const uint32_t decay_threshold_ms =
            static_cast<uint32_t>(config_.rssi_decrease_cycles) * 1000U;
        const SystemTime now = chTimeNow();
        size_t write_idx = 0;
        for (size_t read_idx = 0; read_idx < tracked_count_; ++read_idx) {
            auto& drone = tracked_drones_[read_idx];
            if (drone.rssi_increased_) {
                drone.rssi_decrease_counter_ = 0;
                drone.last_increase_time_ = now;
            } else {
                const uint32_t elapsed = now - drone.last_increase_time_;
                if (elapsed >= decay_threshold_ms) {
                    drone.rssi_decrease_counter_ = 1;
                }
            }
            drone.rssi_increased_ = false;
            if (drone.rssi_decrease_counter_ > 0) {
                // Enforce minimum lifetime before allowing removal
                const uint32_t lifetime = now - drone.created_time_;
                if (lifetime < DRONE_STALE_TIMEOUT_MS) {
                    // Too young to remove — keep even if threat would be NONE
                    if (write_idx != read_idx) {
                        tracked_drones_[write_idx] = tracked_drones_[read_idx];
                    }
                    ++write_idx;
                    continue;
                }
                if (drone.decay_threat()) {
                    drone.rssi_decrease_counter_ = 0;
                    continue;  // drone removed (threat = NONE)
                }
            }
            if (write_idx != read_idx) {
                tracked_drones_[write_idx] = tracked_drones_[read_idx];
            }
            ++write_idx;
        }
        tracked_count_ = static_cast<uint8_t>(write_idx);
    }

    [[nodiscard]] HistogramProcessor& get_histogram_processor() noexcept {
        return histogram_processor_;
    }

    /**
     * @brief Get histogram data snapshot (thread-safe)
     * @param buffer Output buffer for histogram data
     * @param max_length Maximum buffer length
     * @return Number of histogram entries copied
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     * @note Use this from UI thread instead of get_histogram_processor().get_histogram_data()
     */
    [[nodiscard]] size_t get_histogram_snapshot(
        uint16_t* buffer,
        size_t max_length
    ) noexcept {
        MutexTryLock<LockOrder::DATA_MUTEX> lock(mutex_);
        if (!lock.is_locked()) {
            return 0;
        }
        return histogram_processor_.get_histogram_data(buffer, max_length);
    }

private:
    /**
     * @brief Internal: Perform scan cycle
     * @note Called by perform_scan_cycle() with mutex held
     * @return ErrorCode::SUCCESS if cycle completed, error code otherwise
     * @pre Mutex must be held (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] ErrorCode perform_scan_cycle_internal() noexcept;
    
    /**
     * @brief Internal: Update tracked drone
     * @note Called by update_tracked_drones() with mutex held
     * @param frequency Frequency of detected signal
     * @param rssi RSSI value
     * @param timestamp Timestamp of detection
     * @return ErrorCode::SUCCESS if updated, error code otherwise
     * @pre Mutex must be held (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] ErrorCode update_tracked_drone_internal(
        FreqHz frequency,
        RssiValue rssi,
        SystemTime timestamp
    ) noexcept;
    
    /**
     * @brief Internal: Find drone by frequency
     * @param frequency Frequency to find
     * @return ErrorResult containing index or error
     * @pre Mutex must be held (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] ErrorResult<size_t> find_drone_by_frequency_internal(
        FreqHz frequency
    ) const noexcept;
    
    /**
     * @brief Internal: Add new tracked drone
     * @param frequency_hz Frequency of detected signal (Hz)
     * @param rssi_dbm RSSI value (dBm)
     * @param timestamp_ms Timestamp of detection (ms)
     * @return ErrorCode::SUCCESS if added, error code otherwise
     * @pre Mutex must be held (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] ErrorCode add_tracked_drone_internal(
        FreqHz frequency_hz,
        RssiValue rssi_dbm,
        SystemTime timestamp_ms
    ) noexcept;
    
    /**
     * @brief Internal: Remove stale drones
     * @note Called by remove_stale_drones() with mutex held
     * @param current_time Current system time
     * @pre Mutex must be held (LockOrder::DATA_MUTEX)
     */
    void remove_stale_drones_internal(SystemTime current_time) noexcept;
    
    /**
     * @brief Internal: Determine drone type from frequency
     * @param frequency Frequency to analyze
     * @return Drone type
     * @pre Mutex must be held (LockOrder::DATA_MUTEX)
     * @note Uses database_.find_entry() to look up drone type from freqman DB
     */
    [[nodiscard]] DroneType determine_drone_type_internal(FreqHz frequency) const noexcept;

    /**
     * @brief Internal: Validate scan configuration
     * @param config Configuration to validate
     * @return ErrorCode::SUCCESS if valid, error code otherwise
     */
    [[nodiscard]] ErrorCode validate_config_internal(const ScanConfig& config) const noexcept;

    /**
     * @brief Internal: Analyze spectrum shape for U/V signal peaks
     * @param spectrum Channel spectrum data (256 bins, 0-255 each)
     * @param out_rssi Estimated RSSI in dBm if signal detected
     * @return true if drone-like signal detected (elevated peak with width)
     * @note Noise floor = flat line. Signal = elevated U/V peak above noise.
     * @pre Mutex must be held (LockOrder::DATA_MUTEX)
     */
    [[nodiscard]] bool analyze_spectrum_shape(const ChannelSpectrum& spectrum, int32_t& out_rssi) noexcept;

    /**
     * @brief Internal: Trigger alert callback if set
     * @param threat_level Threat level to report
     * @note Re-entrant safe via AtomicFlag guard
     * @pre Mutex must NOT be held (callback must be lock-free)
     */
    void trigger_alert(ThreatLevel threat_level) noexcept;

    // References to dependencies
    DatabaseManager& database_;
    HardwareController& hardware_;
    
    // Scanner state
    ScannerState state_;
    
    // Scan configuration
    ScanConfig config_;

    // Fast scanner state (protected by mutex_)
    uint32_t freq_lock_count_{0};              // Frequency lock counter (0-10)
    FreqHz locked_frequency_{0};                // Locked frequency for tracking
    SystemTime track_start_time_{0};           // Tracking start time
    char current_drone_type_[5]{'\0', '\0', '\0', '\0', '\0'};  // All bytes initialized
    bool drone_type_valid_{false};              // Drone type valid flag
    
    // Scan statistics
    ScanStatistics statistics_;
    
    // Tracked drones (fixed-size array, no heap allocation)
    std::array<TrackedDrone, MAX_TRACKED_DRONES> tracked_drones_;
    
    // Number of tracked drones (uint8_t sufficient for MAX_TRACKED_DRONES=16)
    uint8_t tracked_count_;
    
    // Current scan frequency
    FreqHz current_frequency_;

    // Pending detection hysteresis (prevent noise from adding phantom drones)
    FreqHz pending_frequency_{0};
    uint8_t pending_count_{0};
    static constexpr uint8_t DETECT_CONFIRM_COUNT = 2;

    // Consecutive missed detections on locked frequency (prevents premature lock-break)
    uint8_t missed_lock_count_{0};

    // Noise blacklist: track force-resume count per frequency
    // If we force-resume from a freq 3+ times without threat upgrade → skip it
    static constexpr size_t MAX_NOISE_ENTRIES = 8;
    struct NoiseEntry { FreqHz freq; uint8_t count; };
    NoiseEntry noise_blacklist_[MAX_NOISE_ENTRIES]{};

    // Last scan time
    SystemTime last_scan_time_;
    
    // Scanning active flag
    AtomicFlag scanning_active_;
    
    // Alert callback
    ThreatAlertCallback alert_callback_;
    
    // Mutex for thread safety (LockOrder::DATA_MUTEX)
    mutable Mutex mutex_;

    // State transition control flag
    AtomicFlag state_transition_allowed_;

    // Force-resume flag (set by scanner thread, cleared inside mutex-protected scan cycle)
    AtomicFlag force_resume_flag_;

    // Dwell request flag (set by UI thread on signal detection, consumed by scanner thread)
    AtomicFlag dwell_request_;

    // Dwell cycle counter (persistent across scan cycles, managed by scanner class)
    uint8_t dwell_cycles_{0};

    // Sort buffer for analyze_spectrum_shape (class member to avoid static in method)
    static constexpr size_t SPECTRUM_SORT_BUF_SIZE = 256;
    uint8_t spectrum_sort_buf_[SPECTRUM_SORT_BUF_SIZE];

    // Usable bins buffer for process_spectrum_sweep (class member to avoid static in method)
    // (FFT_DC_SPIKE_START - FFT_EDGE_SKIP_NARROW) + (FFT_BIN_COUNT - FFT_EDGE_SKIP_NARROW - FFT_DC_SPIKE_END)
    // = (120 - 6) + (256 - 6 - 136) = 114 + 114 = 228
    static constexpr size_t SWEEP_USABLE_BINS = 228;
    uint8_t sweep_usable_buf_[SWEEP_USABLE_BINS];

    // Alert callback in progress flag (prevents re-entrant calls)
    AtomicFlag alert_callback_in_progress_;

    // RSSI detector for signal analysis and threat classification
    RSSIDetector rssi_detector_;

    // Histogram processor for spectrum analysis
    HistogramProcessor histogram_processor_;

    // Median filter for RSSI spike rejection (window=7 samples)
    MedianFilter<int32_t, 7> rssi_median_filter_;
    bool median_filter_enabled_{false};

    // Neighbor margin checker for anti-false-positive detection
    NeighborMarginChecker neighbor_margin_checker_;

    // Mahalanobis gate for statistical outlier detection
    MahalanobisDetector mahalanobis_detector_;
};

} // namespace drone_analyzer

#endif // SCANNER_HPP
