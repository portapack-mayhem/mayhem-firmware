/*
 * Copyright (C) 2025 Enhanced Drone Analyzer
 *
 * Mahalanobis Gate Filter - Statistical outlier detection for drone signals.
 * Uses integer-only arithmetic with Q8.8 fixed-point representation.
 *
 * Constraints:
 * - NO heap allocation (all stack/static)
 * - NO floating-point operations
 * - NO exceptions or RTTI
 * - Thread-safe: caller must hold appropriate mutex
 */

#ifndef MAHALANOBIS_GATE_HPP
#define MAHALANOBIS_GATE_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include "constants.hpp"
#include "drone_types.hpp"

namespace drone_analyzer {

/**
 * @brief Mahalanobis distance-based signal validator
 *
 * Implements simplified Mahalanobis distance for outlier detection:
 * D²_M = Σ((x_i - μ_i)² / σ_i²)
 *
 * Assumes diagonal covariance matrix (uncorrelated features).
 *
 * @note All arithmetic is integer-only using Q8.8 fixed-point
 * @note No heap allocation - uses fixed-size arrays
 * @note Thread-safety: @pre Caller must hold DATA_MUTEX before calling any method
 */
class MahalanobisDetector {
public:
    /**
     * @brief Feature vector for Mahalanobis calculation
     * @note [0] = RSSI normalized (Q8.8, range 0-65535 → 0-255.99)
     * @note [1] = Frequency stability (Q8.8, 0-256 = 0-100%)
     */
    using FeatureVector = std::array<int16_t, MAHALANOBIS_DIMENSIONS>;

    /**
     * @brief Validate signal against statistical model
     * @param rssi Current RSSI value (dBm)
     * @param frequency Current frequency (Hz)
     * @param stats Statistics for this drone
     * @param threshold_x10 Mahalanobis threshold ×10
     * @return true if signal is valid (not outlier), false otherwise
     *
     * @note If sample_count < 3, returns true (pass through - insufficient data)
     * @note If threshold_x10 == 0, returns true (disabled)
     */
    [[nodiscard]] bool validate(
        RssiValue rssi,
        FreqHz frequency,
        const MahalanobisStatistics& stats,
        uint8_t threshold_x10
    ) const noexcept;

    /**
     * @brief Update statistics with new sample
     * @param stats Statistics to update
     * @param rssi Current RSSI value (dBm)
     * @param center_freq Center frequency (Hz)
     * @param tuned_freq Tuned frequency (Hz)
     *
     * @note Uses simplified Welford's algorithm with integer approximation
     */
    void update_statistics(
        MahalanobisStatistics& stats,
        RssiValue rssi,
        FreqHz center_freq,
        FreqHz tuned_freq
    ) noexcept;

    /**
     * @brief Reset statistics to initial state
     * @param stats Statistics to reset
     */
    static void reset(MahalanobisStatistics& stats) noexcept {
        stats.mean = {};
        stats.variance = {};
        stats.history = {};
        stats.sample_count = 0;
        stats.history_index = 0;
    }

private:
    /**
     * @brief Extract feature vector from RSSI and frequency
     * @param rssi Current RSSI value (dBm)
     * @param center_freq Center frequency (Hz)
     * @param tuned_freq Tuned frequency (Hz)
     * @return Feature vector in Q8.8 format
     */
    [[nodiscard]] FeatureVector extract_features(
        RssiValue rssi,
        FreqHz center_freq,
        FreqHz tuned_freq
    ) const noexcept;

    /**
     * @brief Compute squared Mahalanobis distance
     * @param sample Sample feature vector (Q8.8)
     * @param stats Statistics (mean and variance in Q8.8)
     * @return D²_M in Q8.8 format
     *
     * @note Clamps variance to MAHALANOBIS_MIN_VARIANCE to avoid division by 0
     */
    [[nodiscard]] int32_t compute_distance_squared(
        const FeatureVector& sample,
        const MahalanobisStatistics& stats
    ) const noexcept;

    static constexpr int32_t Q_SCALE = MAHALANOBIS_Q_SCALE;
};

} // namespace drone_analyzer

#endif // MAHALANOBIS_GATE_HPP
