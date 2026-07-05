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
#include "mahalanobis_gate.hpp"
#include "pattern_matcher.hpp"
#include "pattern_manager.hpp"

namespace drone_analyzer {

/**
 * @brief Scan configuration
 * @note ScannerState is defined in drone_types.hpp
 * @note Size: ~104 bytes (large struct with 4 sweep windows + CFAR + pattern params)
 * @note Passed by const reference (const ScanConfig&) to avoid copy overhead
 * @note Consider partitioning if future extensions increase size significantly
 */
struct ScanConfig {
    ScanningMode mode;
    FreqHz start_frequency;
    FreqHz end_frequency;
    uint32_t scan_interval_ms;
    int32_t rssi_threshold_dbm;
    int32_t threat_low_dbm{DEFAULT_THREAT_LOW_DBM};
    int32_t threat_medium_dbm{DEFAULT_THREAT_MEDIUM_DBM};
    int32_t threat_high_dbm{RSSI_HIGH_THREAT_THRESHOLD_DBM};
    int32_t threat_critical_dbm{RSSI_CRITICAL_THREAT_THRESHOLD_DBM};
    uint32_t stale_timeout_ms;
    
    // Sweep range (Hz) — window 1
    FreqHz sweep_start_freq;
    FreqHz sweep_end_freq;
    FreqHz sweep_step_freq;

    // Sweep range (Hz) — window 2 (2.4 GHz: drone control, Wi-Fi drones)
    FreqHz sweep2_start_freq{2400000000ULL};
    FreqHz sweep2_end_freq{2483500000ULL};
    FreqHz sweep2_step_freq{17813000};
    bool sweep2_enabled{true};

    // Sweep range (Hz) — window 3 (1.2 GHz: long-range FPV)
    FreqHz sweep3_start_freq{1120000000ULL};
    FreqHz sweep3_end_freq{1360000000ULL};
    FreqHz sweep3_step_freq{17813000};
    bool sweep3_enabled{true};

    // Sweep range (Hz) — window 4 (433 MHz + 868/915 MHz: control/telemetry)
    FreqHz sweep4_start_freq{433000000ULL};
    FreqHz sweep4_end_freq{928000000ULL};
    FreqHz sweep4_step_freq{17813000};
    bool sweep4_enabled{true};

    // Advanced detection features (ON by default — matches constructor)
    bool dwell_enabled{true};           // Stay on frequency when signal detected
    bool confirm_count_enabled{true};   // Require multiple confirmations before creating drone
    bool noise_blacklist_enabled{true}; // Skip frequencies with persistent noise
    bool spectrum_detection_enabled{true}; // Detect drone signals by spectrum shape (U/V peaks)
    bool median_enabled{true};              // Median filter for RSSI spike rejection (ON by default)
    uint8_t spectrum_margin{DEFAULT_SPECTRUM_MARGIN};            // Peak margin above noise (FPV-optimized: 25 ≈ 8 dB, Wi-Fi rejection)
    uint8_t spectrum_min_width{DEFAULT_SPECTRUM_MIN_WIDTH};      // Min signal width in bins (rejects narrow noise spikes)
    uint8_t spectrum_max_width{DEFAULT_SPECTRUM_MAX_WIDTH};            // Max signal width (reject flat U/I shapes)
    uint8_t spectrum_peak_sharpness{DEFAULT_SPECTRUM_PEAK_SHARPNESS};  // Min sharpness ratio (rejects noise spikes)
    uint8_t spectrum_peak_ratio{DEFAULT_SPECTRUM_PEAK_RATIO};          // Peak-to-width ratio (inverted-V filter)
    uint8_t spectrum_valley_depth{DEFAULT_SPECTRUM_VALLEY_DEPTH};      // Valley depth threshold (V-shape flanks)
    uint8_t spectrum_flatness{DEFAULT_SPECTRUM_FLATNESS};              // Peak-to-average ratio (reject flat-top WiFi/FM)
    uint8_t spectrum_symmetry{DEFAULT_SPECTRUM_SYMMETRY};              // Left/right width symmetry % (reject asymmetric noise)

    // Mahalanobis Gate Filter
    bool mahalanobis_enabled{true};                                     // FPV-OPTIMIZED: ON by default (analog FM outlier rejection)
    uint8_t mahalanobis_threshold_x10{DEFAULT_MAHALOBIS_THRESHOLD_X10};  // Mahalanobis threshold ×10

    // New anti-false-positive features
    int32_t neighbor_margin_db{DEFAULT_NEIGHBOR_MARGIN_DB};  // 0=disabled, 2=FPV default
    bool rssi_variance_enabled{true};                         // FPV-OPTIMIZED: ON by default (analog FM RSSI stability)
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

    // Sweep exception frequencies (per window, 0 = unused slot)
    FreqHz sweep_exceptions[4][EXCEPTIONS_PER_WINDOW]{};
    uint8_t exception_radius_mhz{DEFAULT_EXCEPTION_RADIUS_MHZ};  // 1-100, configurable exclusion radius
    uint8_t rssi_decrease_cycles{5};  // sweep cycles of RSSI decrease before threat decay
    
    // Pattern matching settings
    bool pattern_matching_enabled{true};              // Enable/disable pattern matching
    uint16_t pattern_similarity_threshold{DEFAULT_PATTERN_SIMILARITY_THRESHOLD};  // 0-1000
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
    int32_t max_rssi_dbm;
    
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
        const FreqHz range = cfg.end_freq - cfg.start_freq;
        if (range == 0 || !cfg.enabled) {
            pixel_step_hz = SWEEP_SLICE_BW / SWEEP_PIXELS_PER_SLICE;
        } else {
            pixel_step_hz = range / SWEEP_PIXELS_PER_SLICE;
        }
        step_hz = SWEEP_BINS_PER_STEP * SWEEP_BIN_SIZE;
        center_ini = cfg.start_freq + (SWEEP_SLICE_BW / 2);
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
     * @param threshold_x10 Threshold offset ×10 in spectrum.db units (e.g., 50 = 5.0 units ≈ 1 dB above noise)
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
                // Use int64_t for intermediate multiplication to prevent overflow
                // (left_var can reach ~4.16M, × 1000 exceeds int32_t max of ~2.14B)
                const int32_t left_mean_safe = (left_mean > 0) ? left_mean : 1;
                const int32_t right_mean_safe = (right_mean > 0) ? right_mean : 1;
                const int32_t left_vi = static_cast<int32_t>(
                    (static_cast<int64_t>(left_var) * 1000) /
                    (static_cast<int64_t>(left_mean_safe) * left_mean_safe));
                const int32_t right_vi = static_cast<int32_t>(
                    (static_cast<int64_t>(right_var) * 1000) /
                    (static_cast<int64_t>(right_mean_safe) * right_mean_safe));
                
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

        // Compute adaptive threshold: T_adaptive = noise_estimate + offset_db
        // spectrum.db is dB-compressed (0.2 dB/unit), so threshold is additive, NOT multiplicative.
        // threshold_x10 is offset × 10 (in spectrum.db units), so: threshold = noise_estimate + (threshold_x10 / 10)
        const int32_t adaptive_threshold = 
            noise_estimate + (static_cast<int32_t>(threshold_x10) / 10);

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
     * @param threshold_x10 Threshold offset ×10 in spectrum.db units (additive, see detect())
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
    static constexpr size_t WINDOW = 8;

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
        // FIX: 5MHz frequency window — wide enough to catch WiFi sidebands and
        // Bluetooth hopping within a 5MHz sub-band, but narrow enough to avoid
        // cross-band suppression between 5.7GHz and 5.8GHz FPV channels
        constexpr FreqHz NEIGHBOR_WINDOW_HZ = 5'000'000ULL;
        for (uint8_t i = 0; i < count_; ++i) {
            const auto freq_diff = (history_[i].freq > current_freq)
                ? (history_[i].freq - current_freq)
                : (current_freq - history_[i].freq);
            if (history_[i].freq != current_freq && freq_diff <= NEIGHBOR_WINDOW_HZ && history_[i].rssi > best_neighbor_rssi) {
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
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     */
    void increment_noise_count(FreqHz frequency) noexcept;

    /**
     * @brief Increment noise count — caller must already hold DATA_MUTEX
     * @note Internal variant for use inside process_spectrum_message()
     *       where DATA_MUTEX is already held. ChibiOS mutexes are NOT
     *       recursive — calling the locking version here would deadlock.
     */
    void increment_noise_count_internal(FreqHz frequency) noexcept;

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
     * @warning ChibiOS fast mutexes are NOT recursive. Nested calls from the
     *          same thread will deadlock. perform_scan_cycle_internal() must
     *          NOT acquire DATA_MUTEX again.
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
     * @brief Process spectrum with explicit frequency (avoids race with scanner thread)
     * @param spectrum Channel spectrum data
     * @param frequency Frequency this spectrum corresponds to
     * @return ErrorCode::SUCCESS if processed, error code otherwise
     * @note Acquires mutex (LockOrder::DATA_MUTEX)
     * @note Updates tracked drones if RSSI above threshold
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
     * @brief Get scan configuration (thread-safe copy)
     * @return Copy of current scan configuration
     * @note Acquires mutex (LockOrder::DATA_MUTEX), returns by value
     *       to prevent data race on reference after mutex release
     * @note Stack: ~400 bytes (ScanConfig is large). Prefer targeted
     *       getters (get_threat_critical_dbm()) in hot paths.
     */
    [[nodiscard]] ScanConfig get_config() const noexcept;
    
    /**
     * @brief Get critical threat RSSI threshold (thread-safe, no full config copy)
     * @return Critical threat threshold in dBm
     * @note Acquires mutex (LockOrder::DATA_MUTEX), copies only 4 bytes
     * @note Use instead of get_config().threat_critical_dbm in hot paths
     */
    [[nodiscard]] int32_t get_threat_critical_dbm() const noexcept;
    
    /**
     * @brief Get sweep step frequency in Hz
     * @return Sweep step frequency (bins per step × bin size)
     * @note Uses unified constant SWEEP_BIN_SIZE from constants.hpp
     */
    [[nodiscard]] FreqHz get_sweep_step() const noexcept {
        return SWEEP_BINS_PER_STEP * SWEEP_BIN_SIZE;
    }
    
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
     *       Bins 134-253 (lower sideband): freq = f_center + (bin-256)*SWEEP_BIN_SIZE
     *       Bins 2-119 (upper sideband):   freq = f_center + (bin-126)*SWEEP_BIN_SIZE
     *       Bins 120-133 and 0-1, 254-255 are DC spike / edge (should be skipped)
     */
    static FreqHz fft_bin_to_freq(FreqHz f_center, size_t bin) noexcept {
        // Looking Glass bin reordering: bin 0 = Nyquist, bin 128 = DC.
        // Lower sideband (bin >= 136): freq = f_center - 120*SWEEP_BIN_SIZE + bin*SWEEP_BIN_SIZE
        //   Avoids negative cast: (bin-256) would overflow uint64_t.
        // Upper sideband (bin < 120):  freq = f_center - 126*SWEEP_BIN_SIZE + bin*SWEEP_BIN_SIZE
        //   Avoids negative cast: (bin-126) would overflow uint64_t.
        if (bin >= FFT_DC_SPIKE_END) {
            return f_center - 120 * SWEEP_BIN_SIZE + static_cast<FreqHz>(bin) * SWEEP_BIN_SIZE;
        }
        return f_center - 126 * SWEEP_BIN_SIZE + static_cast<FreqHz>(bin) * SWEEP_BIN_SIZE;
    }

    /**
     * @brief Lightweight spectrum processing for sweep mode
     * @param spectrum Channel spectrum data (256 bins)
     * @param lg_buffer 240-pixel Looking Glass reordered buffer (from reorder_frame)
     * @param center_freq Current slice center frequency
     * @param f_min Minimum frequency of sweep range (0 = no range check)
     * @param f_max Maximum frequency of sweep range (0 = no range check)
     * @note Peak detection runs on raw FFT bins (CFAR or fixed threshold).
     *       Shape analysis runs on LG-reordered buffer (continuous, no DC gap).
     *       Pattern matching runs on raw FFT (consistent normalization with
     *       saved patterns — normalize() and match() both operate on 256-bin FFT).
     * @note Called from UI thread during sweep (scanner thread stopped, no mutex)
     * @note Implementation in scanner.cpp — delegates tracking to apply_sweep_tracking()
     */
    void process_spectrum_sweep(
        const ChannelSpectrum& spectrum,
        const uint8_t* lg_buffer,
        FreqHz center_freq,
        FreqHz f_min = 0,
        FreqHz f_max = 0
    ) noexcept;

    /**
     * @brief Apply RSSI-based threat decay with SWEEP-aware logic
     * @param is_sweep_mode If true, use cycle-based decay for sweep mode; if false, use time-based (normal mode)
     * @note NORMAL mode: time-based decay (fast, continuous scanning)
     * @note SWEEP mode: cycle-based decay (tolerates long gaps between visits)
     * @note Each drone: if RSSI did not increase for decay_threshold_ms (CYC × 1000ms in normal mode),
     *       OR if drone was not seen for more than MAX_SWEEP_CYCLES_MISSED cycles (in sweep mode),
     *       decay threat by one step. If RSSI increased or drone seen, reset counters.
     * @note Enforces minimum drone lifetime of DRONE_STALE_TIMEOUT_MS (5s) before removal.
     * @note Resets rssi_increased_ flag after each call.
     * @note Called from perform_scan_cycle_internal() (normal mode) and on_sweep_spectrum() (sweep mode)
     */
    void apply_rssi_decay(bool is_sweep_mode = false) noexcept {
        const uint32_t decay_threshold_ms =
            static_cast<uint32_t>(config_.rssi_decrease_cycles) * 1000U;
        const SystemTime now = chTimeNow();
        size_t write_idx = 0;
        for (size_t read_idx = 0; read_idx < tracked_count_; ++read_idx) {
            auto& drone = tracked_drones_[read_idx];
            if (drone.rssi_increased_) {
                drone.rssi_decrease_counter_ = 0;
                drone.last_increase_time_ = now;
                drone.sweep_cycles_missed_ = 0;
            } else {
                if (is_sweep_mode) {
                    // SWEEP mode: cycle-based decay
                    drone.increment_missed_cycle();
                    const uint8_t max_missed = (drone.threat_level >= ThreatLevel::HIGH)
                        ? TrackedDrone::MAX_SWEEP_CYCLES_MISSED * 2  // HIGH/CRITICAL: 6 cycles
                        : TrackedDrone::MAX_SWEEP_CYCLES_MISSED;     // LOW/MEDIUM: 3 cycles
                    if (drone.sweep_cycles_missed_ > max_missed) {
                        drone.rssi_decrease_counter_ = 1;
                    }
                } else {
                    // NORMAL mode: time-based decay (original logic)
                    const uint32_t elapsed = now - drone.last_increase_time_;
                    if (elapsed >= decay_threshold_ms) {
                        drone.rssi_decrease_counter_ = 1;
                    }
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

    /**
     * @brief Get lock timeout counter (for monitoring and debugging)
     * @return Number of times scanner force-resumed due to lock timeout
     * @note uint32_t reads are atomic on Cortex-M4 — no mutex needed
     */
    [[nodiscard]] uint32_t get_lock_timeout_count() const noexcept {
        return lock_timeout_count_;
    }

    /**
     * @brief Get pattern manager (for UI access)
     * @return Reference to pattern manager
     * @note Thread-safe: pattern manager is already thread-safe internally
     */
    [[nodiscard]] PatternManager& get_pattern_manager() noexcept {
        return pattern_manager_;
    }

    /**
     * @brief Get number of loaded patterns
     * @return Pattern count from pattern manager
     * @note Thread-safe: pattern manager is internally thread-safe
     */
    [[nodiscard]] size_t get_pattern_count() const noexcept {
        return pattern_manager_.get_pattern_count();
    }

    /**
     * @brief Get loaded pattern array
     * @return Pointer to first pattern (may be nullptr if count == 0)
     * @note Thread-safe: pattern manager is internally thread-safe
     */
    [[nodiscard]] const SignalPattern* get_patterns() const noexcept {
        return pattern_manager_.get_patterns_array();
    }

    /**
     * @brief Force reload patterns from SD and update matcher
     * @note Called after pattern save/delete to ensure SWEEP sees new patterns
     */
    void refresh_patterns() noexcept;

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
     * @brief Shared spectrum shape analysis with configurable edge skip.
     * @note Called by both analyze_spectrum_shape() and process_spectrum_sweep()
     *       to eliminate 200-line code duplication and ensure identical filter logic.
     * @param spectrum 256-bin FFT data
     * @param out_rssi Output: RSSI in dBm if signal detected
     * @param edge_skip Number of edge bins to skip (FFT_EDGE_SKIP=10 for normal, FFT_EDGE_SKIP_NARROW=6 for sweep)
     * @return true if drone-like signal detected
     */
    [[nodiscard]] bool analyze_spectrum_shape_impl(
        const ChannelSpectrum& spectrum,
        size_t peak_index,
        uint8_t raw_peak,
        uint8_t noise_floor,
        int32_t& out_rssi,
        size_t edge_skip,
        int32_t total_gain
    ) noexcept;

    /**
     * @brief Shape analysis on Looking Glass reordered buffer (240 pixels, no DC gap).
     * @param lg_buffer  240-pixel Looking Glass buffer (from SweepProcessor::reorder_frame())
     * @param peak_pixel Pixel index of detected peak (0-239)
     * @param noise_floor 25th percentile noise floor (computed from raw FFT usable bins)
     * @param out_rssi   Output: RSSI in dBm if signal detected
     * @param total_gain Current hardware gain for RSSI conversion
     * @return true if drone-like signal detected
     * @note Operates on the same continuous line the user sees on screen.
     *       No DC gap to corrupt width/sharpness/valley/flatness/symmetry.
     * @note Edge skip: 4 pixels from each end.
     *       LG pixel layout: px 0-119 = bins 134-253 (lower sideband),
     *       px 120-237 = bins 2-119 (upper sideband), px 238-239 = 0 (DC).
     *       Left skip (px 0-3) = bins 134-137 (near DC).
     *       Right skip (px 236-239) = bins 118-119 + zero padding.
     *       Filter rolloff bins (0-5, 250-255) map to px 116-123 (crossover);
     *       they have attenuated power and naturally terminate width expansion.
     */
    [[nodiscard]] bool analyze_spectrum_shape_lg(
        const uint8_t* lg_buffer,
        size_t peak_pixel,
        uint8_t noise_floor,
        int32_t& out_rssi,
        int32_t total_gain
    ) noexcept;

    /**
     * @brief Shared 11-step spectrum shape filter chain (Steps 3-11).
     * @param data         Power data buffer (spectrum.db.data() or lg_buffer)
     * @param peak_idx     Index of detected peak
     * @param raw_peak     Raw power value at peak
     * @param noise_floor  Computed noise floor (25th percentile)
     * @param out_rssi     Output: RSSI in dBm if signal passes all filters
     * @param edge_skip    Number of edge bins/pixels to skip
     * @param has_dc_gap   true = skip FFT DC spike bins (120-135); false = LG buffer (no DC gap)
     * @param total_gain   Current hardware gain for RSSI conversion
     * @return true if signal passes all shape filters
     * @note Stack: ~0 bytes (all state via parameters).
     * @note Shared by analyze_spectrum_shape_impl() (raw FFT) and
     *       analyze_spectrum_shape_lg() (LG reordered buffer).
     */
    [[nodiscard]] bool apply_shape_filters(
        const uint8_t* data,
        size_t peak_idx,
        uint8_t raw_peak,
        uint8_t noise_floor,
        int32_t& out_rssi,
        size_t edge_skip,
        bool has_dc_gap,
        int32_t total_gain
    ) const noexcept;

    /**
     * @brief Sweep-mode post-detection: range check, exception filter, Mahalanobis gate,
     *        drone tracking, and pattern match assignment.
     * @param peak_freq       Detected peak RF frequency (Hz)
     * @param peak_rssi       Filtered RSSI (dBm) after median filter
     * @param center_freq     FFT slice center frequency for Mahalanobis
     * @param f_min           Sweep range lower bound (0 = use config)
     * @param f_max           Sweep range upper bound (0 = use config)
     * @param highlight_bin   256-bin FFT index for UI red match marker
     * @param pattern_index   Matched pattern index (-1 if none)
     * @param pattern_correlation SAD score (0-1000)
     * @param pattern_matched Whether a pattern matched this frame
     * @note Extracted from process_spectrum_sweep to eliminate code duplication
     *       between the raw-FFT and LG-buffer overloads.
     * @note Called from UI thread during sweep (scanner thread stopped, no mutex).
     */
    void apply_sweep_tracking(
        FreqHz peak_freq,
        int32_t peak_rssi,
        FreqHz center_freq,
        FreqHz f_min,
        FreqHz f_max,
        size_t highlight_bin,
        int8_t pattern_index,
        uint16_t pattern_correlation,
        bool pattern_matched
    ) noexcept;

    /**
     * @brief Convert 256-bin FFT index to 240-pixel Looking Glass index.
     * @param bin FFT bin index (0-255)
     * @return Pixel index (0-239), or COMPOSITE_SIZE (sentinel) if bin maps to
     *         DC spike (bins 120-135, no valid pixel).
     * @note Lower sideband (bins 134-255): pixel = bin - 134  →  pixels 0-121
     * @note Upper sideband (bins 0-119):   pixel = bin + 118  →  pixels 118-237
     * @note Edge bins (0-5, 250-255) still produce valid pixel indices; edge
     *       filtering is handled separately by analyze_spectrum_shape_lg().
     */
    static size_t fft_bin_to_lg_pixel(size_t bin) noexcept {
        if (bin >= SWEEP_FFT_MAP_START) {
            return bin - SWEEP_FFT_MAP_START;  // bins 134-255 → pixels 0-121
        }
        if (bin < FFT_DC_SPIKE_START) {
            return bin + static_cast<size_t>(SWEEP_FFT_MAP_CROSSOVER - 2);  // bins 0-119 → pixels 118-237
        }
        return COMPOSITE_SIZE;  // DC spike (bins 120-135) — invalid
    }

    /**
     * @brief Internal: Try to match spectrum against stored patterns
     * @param spectrum Channel spectrum data (256 bins)
     * @param current_freq Current tuned frequency for proximity filter
     * @return PatternMatchResult with match status
     * @note Single source of truth for pattern matching — called from both
     *       process_spectrum_message() (under mutex) and process_spectrum_sweep()
     *       (no mutex, scanner thread stopped). pattern_manager_ is internally
     *       thread-safe; config_.pattern_matching_enabled is read non-atomically
     *       but the worst case is one extra/stale match on a config flip.
     */
    [[nodiscard]] PatternMatchResult try_match_pattern_internal(
        const uint8_t* spectrum,
        FreqHz current_freq
    ) noexcept;

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

    // Last sweep frequency for per-frequency median filter reset
    // In sweep mode, the median filter must NOT be reset every frame (useless —
    // never reaches warm state). Instead, only reset when the frequency changes,
    // allowing the filter to accumulate across sweep cycles for the same freq.
    FreqHz last_sweep_freq_{0};

    // Pending detection hysteresis (prevent noise from adding phantom drones)
    FreqHz pending_frequency_{0};
    uint8_t pending_count_{0};

    // RSSI hysteresis state (Schmitt trigger: 2 dB to turn ON, 2 dB easier to stay ON)
    bool signal_present_{false};
    FreqHz last_hysteresis_freq_{0};
    static constexpr int32_t RSSI_HYSTERESIS_DB = 2;

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

    // Lock timeout tracking (prevents infinite lock on noisy frequencies)
    SystemTime lock_start_time_{0};
    static constexpr uint32_t MAX_LOCK_DURATION_MS = 5000;  // 5 seconds absolute timeout

    // Confirm timeout tracking (prevents waiting forever for confirmations)
    SystemTime confirm_start_time_{0};
    static constexpr uint32_t CONFIRM_TIMEOUT_MS = 5000;  // 5 seconds to gather confirmations

    // Sort buffer for analyze_spectrum_shape (class member to avoid static in method)
    static constexpr size_t SPECTRUM_SORT_BUF_SIZE = 256;
    uint8_t spectrum_sort_buf_[SPECTRUM_SORT_BUF_SIZE];

    // Lock timeout violation counter (for monitoring and debugging)
    uint32_t lock_timeout_count_{0};

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

    // Mahalanobis detector for statistical outlier detection (Sweep mode only)
    MahalanobisDetector mahalanobis_detector_;

    // Pattern matcher for signal pattern recognition
    PatternMatcher pattern_matcher_;

    // Pattern manager for loading/saving patterns from SD card
    PatternManager pattern_manager_;

    // Matched pattern index (-1 if no pattern matched in sweep)
    int8_t matched_pattern_index_{-1};
    size_t matched_pattern_bin_{0};

    // No gain cache — use get_current_total_gain() directly to avoid stale values.

public:
    [[nodiscard]] bool is_pattern_matched() const noexcept { return matched_pattern_index_ >= 0; }
    [[nodiscard]] size_t get_matched_pattern_bin() const noexcept { return matched_pattern_bin_; }
    void clear_matched_pattern() noexcept { matched_pattern_index_ = -1; matched_pattern_bin_ = 0; }
};

} // namespace drone_analyzer

#endif // SCANNER_HPP
