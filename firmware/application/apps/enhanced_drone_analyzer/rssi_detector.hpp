#ifndef RSSI_DETECTOR_HPP
#define RSSI_DETECTOR_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include "drone_types.hpp"
#include "constants.hpp"

namespace drone_analyzer {

/**
 * @brief RSSI detection result
 * @note Fixed-size structure, no heap allocation
 */
struct RSSIDetectionResult {
    bool drone_detected;
    ThreatLevel threat_level;
    RssiValue rssi;
    RssiValue average_rssi;
    MovementTrend movement_trend;
    uint8_t sample_count;
    uint8_t reserved[7];
};

/**
 * @brief RSSI statistics
 * @note Fixed-size structure, no heap allocation
 */
struct RSSIStatistics {
    uint32_t total_samples;
    uint32_t detections;
    uint32_t high_threat_count;
    uint32_t critical_threat_count;
    RssiValue min_rssi;
    RssiValue max_rssi;
    uint32_t approaching_count;
    uint32_t receding_count;
    uint32_t static_count;
};

/**
 * @brief RSSI-based drone detector
 * @note No floating-point operations, uses integer arithmetic
 * @note No heap allocation, uses fixed-size arrays
 * @note Thread-safety: @pre Caller must hold DATA_MUTEX before calling any method
 * @note Removed: internal mutex (relies on parent DroneScanner's DATA_MUTEX)
 */
class RSSIDetector {
public:
    RSSIDetector() noexcept;

    [[nodiscard]] ErrorCode initialize(RssiValue detection_threshold) noexcept;

    [[nodiscard]] ErrorCode process_rssi_sample(
        RssiValue rssi,
        SystemTime timestamp
    ) noexcept;

    [[nodiscard]] ErrorCode detect_drone(RSSIDetectionResult& result) noexcept;

    [[nodiscard]] ThreatLevel calculate_threat_level(
        RssiValue rssi
    ) const noexcept;

    void update_statistics(
        bool detected,
        ThreatLevel threat_level
    ) noexcept;

    void reset() noexcept;

    void get_statistics(RSSIStatistics& stats) const noexcept;

    [[nodiscard]] MovementTrend get_movement_trend() const noexcept;

    [[nodiscard]] RssiValue get_average_rssi() const noexcept;

    void set_detection_threshold(RssiValue threshold) noexcept;

    void set_threat_thresholds(const ThreatThresholds& thresholds) noexcept;

    [[nodiscard]] RssiValue get_detection_threshold() const noexcept;

private:
    [[nodiscard]] ErrorCode validate_rssi(RssiValue rssi) const noexcept;

    void add_to_history(RssiValue rssi, SystemTime timestamp) noexcept;

    [[nodiscard]] MovementTrend calculate_movement_trend() const noexcept;

    [[nodiscard]] bool is_approaching() const noexcept;

    [[nodiscard]] bool is_receding() const noexcept;

    [[nodiscard]] bool is_static() const noexcept;

    [[nodiscard]] uint32_t calculate_variance() const noexcept;

    RssiValue detection_threshold_;
    ThreatThresholds threat_thresholds_;

    std::array<RssiValue, RSSI_HISTORY_SIZE> rssi_history_;
    std::array<SystemTime, TIMESTAMP_HISTORY_SIZE> timestamp_history_;
    uint8_t history_index_;

    uint16_t samples_count_;

    RSSIStatistics statistics_;

    bool initialized_;
};

} // namespace drone_analyzer

#endif // RSSI_DETECTOR_HPP
