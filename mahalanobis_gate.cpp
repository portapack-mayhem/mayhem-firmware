/*
 * Copyright (C) 2025 Enhanced Drone Analyzer
 *
 * Mahalanobis Gate Filter Implementation
 *
 * All arithmetic is integer-only using Q8.8 fixed-point.
 * No heap allocation, no exceptions, no floating-point.
 */

#include "mahalanobis_gate.hpp"
#include "constants.hpp"

namespace drone_analyzer {

// ============================================================================
// Public Methods
// ============================================================================

bool MahalanobisDetector::validate(
    RssiValue rssi,
    FreqHz frequency,
    const MahalanobisStatistics& stats,
    uint8_t threshold_x10
) const noexcept {
    if (stats.sample_count < 3) {
        return true;
    }

    if (threshold_x10 == 0) {
        return true;
    }

    FeatureVector sample = extract_features(rssi, frequency, frequency);
    int32_t distance_sq = compute_distance_squared(sample, stats);

    int32_t threshold_sq = static_cast<int32_t>(threshold_x10) * threshold_x10;
    threshold_sq = (threshold_sq * Q_SCALE) / 100;

    return distance_sq < threshold_sq;
}

void MahalanobisDetector::update_statistics(
    MahalanobisStatistics& stats,
    RssiValue rssi,
    FreqHz center_freq,
    FreqHz tuned_freq
) noexcept {
    FeatureVector sample = extract_features(rssi, center_freq, tuned_freq);

    stats.history[stats.history_index] = sample;
    stats.history_index = (stats.history_index + 1) % MAHALANOBIS_HISTORY_SIZE;

    if (stats.sample_count < MAHALANOBIS_HISTORY_SIZE) {
        stats.sample_count++;
    }

    for (uint8_t i = 0; i < MAHALANOBIS_DIMENSIONS; ++i) {
        int32_t n = stats.sample_count;

        if (n < 2) {
            stats.mean[i] = sample[i];
            continue;
        }

        int32_t delta = sample[i] - stats.mean[i];
        stats.mean[i] += delta / n;
        int32_t delta2 = sample[i] - stats.mean[i];
        stats.variance[i] += (delta * delta2) / (n - 1);
    }

    // Variance decay: prevent unbounded growth during long scans.
    // Without decay, variance accumulates indefinitely and eventually
    // hits the 32767 clamp, making the gate permanently permissive.
    // Decay by 1/8 every HISTORY_SIZE samples (exponential moving average).
    if (stats.sample_count >= MAHALANOBIS_HISTORY_SIZE &&
        stats.history_index == 0) {
        for (uint8_t i = 0; i < MAHALANOBIS_DIMENSIONS; ++i) {
            stats.variance[i] = (stats.variance[i] * 7) / 8;
            if (stats.variance[i] < MAHALANOBIS_MIN_VARIANCE) {
                stats.variance[i] = MAHALANOBIS_MIN_VARIANCE;
            }
        }
    }
}

// ============================================================================
// Private Methods
// ============================================================================

MahalanobisDetector::FeatureVector MahalanobisDetector::extract_features(
    RssiValue rssi,
    FreqHz center_freq,
    FreqHz tuned_freq
) const noexcept {
    FeatureVector features{};

    int16_t rssi_clamped = static_cast<int16_t>(
        (rssi < MAHALANOBIS_RSSI_MIN_DBM) ? MAHALANOBIS_RSSI_MIN_DBM :
        (rssi > MAHALANOBIS_RSSI_MAX_DBM) ? MAHALANOBIS_RSSI_MAX_DBM : rssi
    );

    int32_t rssi_norm = (rssi_clamped - MAHALANOBIS_RSSI_MIN_DBM) * Q_SCALE;
    rssi_norm = rssi_norm / (MAHALANOBIS_RSSI_MAX_DBM - MAHALANOBIS_RSSI_MIN_DBM) * 255;
    features[0] = static_cast<int16_t>(rssi_norm);

    uint64_t abs_diff;
    if (tuned_freq >= center_freq) {
        abs_diff = tuned_freq - center_freq;
    } else {
        abs_diff = center_freq - tuned_freq;
    }

    int32_t stability = Q_SCALE;
    if (abs_diff < FREQUENCY_BANDWIDTH_HZ) {
        stability = (Q_SCALE * (FREQUENCY_BANDWIDTH_HZ - abs_diff)) / FREQUENCY_BANDWIDTH_HZ;
    }
    features[1] = static_cast<int16_t>(stability);

    return features;
}

int32_t MahalanobisDetector::compute_distance_squared(
    const FeatureVector& sample,
    const MahalanobisStatistics& stats
) const noexcept {
    int32_t distance_sq = 0;

    for (uint8_t i = 0; i < MAHALANOBIS_DIMENSIONS; ++i) {
        int32_t diff_Q = sample[i] - stats.mean[i];
        int32_t diff_sq = (diff_Q * diff_Q) / Q_SCALE;

        int32_t var = stats.variance[i];
        if (var < MAHALANOBIS_MIN_VARIANCE) {
            var = MAHALANOBIS_MIN_VARIANCE;
        }
        if (var > 32767) {
            var = 32767;
        }

        int32_t term = (diff_sq * Q_SCALE) / var;
        distance_sq += term;
    }

    return distance_sq;
}

} // namespace drone_analyzer
