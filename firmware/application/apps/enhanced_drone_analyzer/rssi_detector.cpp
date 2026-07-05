#include "rssi_detector.hpp"

namespace drone_analyzer {

RSSIDetector::RSSIDetector() noexcept
    : detection_threshold_(RSSI_DETECTION_THRESHOLD_DBM)
    , threat_thresholds_{DEFAULT_THREAT_LOW_DBM, DEFAULT_THREAT_MEDIUM_DBM,
                          RSSI_HIGH_THREAT_THRESHOLD_DBM, RSSI_CRITICAL_THREAT_THRESHOLD_DBM}
    , rssi_history_{}
    , timestamp_history_{}
    , history_index_(0)
    , samples_count_(0)
    , statistics_{}
    , initialized_(false) {
}

ErrorCode RSSIDetector::initialize(RssiValue detection_threshold) noexcept {
    if (detection_threshold < RSSI_MIN_DBM || detection_threshold > RSSI_MAX_DBM) {
        return ErrorCode::INVALID_PARAMETER;
    }

    detection_threshold_ = detection_threshold;

    for (size_t i = 0; i < RSSI_HISTORY_SIZE; ++i) {
        rssi_history_[i] = RSSI_MIN_DBM;
    }
    for (size_t i = 0; i < TIMESTAMP_HISTORY_SIZE; ++i) {
        timestamp_history_[i] = 0;
    }
    history_index_ = 0;
    samples_count_ = 0;

    statistics_.total_samples = 0;
    statistics_.detections = 0;
    statistics_.high_threat_count = 0;
    statistics_.critical_threat_count = 0;
    statistics_.min_rssi = RSSI_MAX_DBM;
    statistics_.max_rssi = RSSI_MIN_DBM;
    statistics_.approaching_count = 0;
    statistics_.receding_count = 0;
    statistics_.static_count = 0;

    initialized_ = true;
    return ErrorCode::SUCCESS;
}

ErrorCode RSSIDetector::process_rssi_sample(
    RssiValue rssi,
    SystemTime timestamp
) noexcept {
    ErrorCode error = validate_rssi(rssi);
    if (error != ErrorCode::SUCCESS) {
        return error;
    }

    // Store every sample — the caller (process_spectrum_message) already gates
    // on signal_detected before calling. This gives the detector an unbiased
    // view of signal strength for trend analysis and statistics.
    add_to_history(rssi, timestamp);

    statistics_.total_samples++;
    if (rssi < statistics_.min_rssi) {
        statistics_.min_rssi = rssi;
    }
    if (rssi > statistics_.max_rssi) {
        statistics_.max_rssi = rssi;
    }

    return ErrorCode::SUCCESS;
}

ErrorCode RSSIDetector::detect_drone(RSSIDetectionResult& result) noexcept {
    result.drone_detected = false;
    result.threat_level = ThreatLevel::NONE;
    result.rssi = RSSI_MIN_DBM;
    result.average_rssi = RSSI_MIN_DBM;
    result.movement_trend = MovementTrend::UNKNOWN;
    result.sample_count = static_cast<uint8_t>(samples_count_);

    if (samples_count_ < 2) {
        return ErrorCode::SUCCESS;
    }

    result.average_rssi = get_average_rssi();

    const size_t latest_index = (history_index_ > 0) ? (history_index_ - 1) : 0;
    result.rssi = rssi_history_[latest_index % RSSI_HISTORY_SIZE];

    // Use averaged RSSI for detection decision when we have enough samples.
    // This rejects single-sample noise spikes that could trigger false positives.
    // When fewer than 3 samples, fall back to raw sample for fast response.
    const RssiValue detection_rssi = (samples_count_ >= 3) ? result.average_rssi : result.rssi;

    if (detection_rssi >= detection_threshold_) {
        result.drone_detected = true;
        result.threat_level = calculate_threat_level(detection_rssi);
        result.movement_trend = get_movement_trend();
        update_statistics(true, result.threat_level);
    }

    return ErrorCode::SUCCESS;
}

ThreatLevel RSSIDetector::calculate_threat_level(
    RssiValue rssi
) const noexcept {
    if (rssi >= threat_thresholds_.critical) {
        return ThreatLevel::CRITICAL;
    } else if (rssi >= threat_thresholds_.high) {
        return ThreatLevel::HIGH;
    } else if (rssi >= threat_thresholds_.medium) {
        return ThreatLevel::MEDIUM;
    } else if (rssi >= threat_thresholds_.low) {
        return ThreatLevel::LOW;
    } else {
        return ThreatLevel::NONE;
    }
}

void RSSIDetector::update_statistics(
    bool detected,
    ThreatLevel threat_level
) noexcept {
    if (detected) {
        statistics_.detections++;

        if (threat_level == ThreatLevel::HIGH) {
            statistics_.high_threat_count++;
        } else if (threat_level == ThreatLevel::CRITICAL) {
            statistics_.critical_threat_count++;
        }

        const MovementTrend trend = get_movement_trend();
        if (trend == MovementTrend::APPROACHING) {
            statistics_.approaching_count++;
        } else if (trend == MovementTrend::RECEDING) {
            statistics_.receding_count++;
        } else if (trend == MovementTrend::STATIC) {
            statistics_.static_count++;
        }
    }
}

void RSSIDetector::reset() noexcept {
    for (size_t i = 0; i < RSSI_HISTORY_SIZE; ++i) {
        rssi_history_[i] = RSSI_MIN_DBM;
    }
    for (size_t i = 0; i < TIMESTAMP_HISTORY_SIZE; ++i) {
        timestamp_history_[i] = 0;
    }
    history_index_ = 0;
    samples_count_ = 0;

    statistics_.total_samples = 0;
    statistics_.detections = 0;
    statistics_.high_threat_count = 0;
    statistics_.critical_threat_count = 0;
    statistics_.min_rssi = RSSI_MAX_DBM;
    statistics_.max_rssi = RSSI_MIN_DBM;
    statistics_.approaching_count = 0;
    statistics_.receding_count = 0;
    statistics_.static_count = 0;
}

void RSSIDetector::get_statistics(RSSIStatistics& stats) const noexcept {
    stats = statistics_;
}

MovementTrend RSSIDetector::get_movement_trend() const noexcept {
    if (samples_count_ < 2) {
        return MovementTrend::UNKNOWN;
    }

    return calculate_movement_trend();
}

RssiValue RSSIDetector::get_average_rssi() const noexcept {
    if (samples_count_ == 0) {
        return RSSI_MIN_DBM;
    }

    uint32_t sum = 0;
    const uint32_t count = (samples_count_ > RSSI_HISTORY_SIZE) ? RSSI_HISTORY_SIZE : static_cast<uint32_t>(samples_count_);

    for (size_t i = 0; i < count; ++i) {
        sum += static_cast<uint32_t>(rssi_history_[i] - RSSI_MIN_DBM);
    }

    const uint32_t avg = sum / count;
    return static_cast<RssiValue>(avg + RSSI_MIN_DBM);
}

void RSSIDetector::set_detection_threshold(RssiValue threshold) noexcept {
    if (threshold >= RSSI_MIN_DBM && threshold <= RSSI_MAX_DBM) {
        detection_threshold_ = threshold;
    }
}

void RSSIDetector::set_threat_thresholds(const ThreatThresholds& thresholds) noexcept {
    threat_thresholds_ = thresholds;
}

RssiValue RSSIDetector::get_detection_threshold() const noexcept {
    return detection_threshold_;
}

ErrorCode RSSIDetector::validate_rssi(RssiValue rssi) const noexcept {
    if (rssi < RSSI_MIN_DBM || rssi > RSSI_MAX_DBM) {
        return ErrorCode::INVALID_PARAMETER;
    }
    return ErrorCode::SUCCESS;
}

void RSSIDetector::add_to_history(RssiValue rssi, SystemTime timestamp) noexcept {
    const size_t rssi_index = history_index_ % RSSI_HISTORY_SIZE;
    rssi_history_[rssi_index] = rssi;

    const size_t timestamp_index = history_index_ % TIMESTAMP_HISTORY_SIZE;
    timestamp_history_[timestamp_index] = timestamp;

    history_index_++;
    if (samples_count_ < RSSI_HISTORY_SIZE) {
        samples_count_++;
    }
}

MovementTrend RSSIDetector::calculate_movement_trend() const noexcept {
    if (samples_count_ < 2) {
        return MovementTrend::UNKNOWN;
    }

    const uint32_t variance = calculate_variance();
    constexpr uint32_t VARIANCE_THRESHOLD = 25;

    if (variance < VARIANCE_THRESHOLD) {
        return MovementTrend::STATIC;
    }

    if (is_approaching()) {
        return MovementTrend::APPROACHING;
    } else if (is_receding()) {
        return MovementTrend::RECEDING;
    } else {
        return MovementTrend::STATIC;
    }
}

bool RSSIDetector::is_approaching() const noexcept {
    if (samples_count_ < 2) {
        return false;
    }
    const size_t latest_idx = (history_index_ > 0) ? (history_index_ - 1) : 0;
    const RssiValue latest = rssi_history_[latest_idx % RSSI_HISTORY_SIZE];
    // When buffer wraps, the oldest occupies the slot history_index_ % RSSI_HISTORY_SIZE
    // (the next slot to be overwritten). When not yet full: index 0.
    const size_t oldest_idx = (samples_count_ >= RSSI_HISTORY_SIZE)
        ? (history_index_ % RSSI_HISTORY_SIZE)
        : 0;
    const RssiValue oldest = rssi_history_[oldest_idx];
    return (latest - oldest) >= MOVEMENT_TREND_THRESHOLD_APPROACHING_DB;
}

bool RSSIDetector::is_receding() const noexcept {
    if (samples_count_ < 2) {
        return false;
    }
    const size_t latest_idx = (history_index_ > 0) ? (history_index_ - 1) : 0;
    const RssiValue latest = rssi_history_[latest_idx % RSSI_HISTORY_SIZE];
    const size_t oldest_idx = (samples_count_ >= RSSI_HISTORY_SIZE)
        ? (history_index_ % RSSI_HISTORY_SIZE)
        : 0;
    const RssiValue oldest = rssi_history_[oldest_idx];
    // NOTE: Using APPROACHING threshold for the reversed comparison
    // (oldest - latest >= 3) is equivalent to (latest - oldest <= -3)
    return (oldest - latest) >= MOVEMENT_TREND_THRESHOLD_APPROACHING_DB;
}

bool RSSIDetector::is_static() const noexcept {
    const uint32_t variance = calculate_variance();
    constexpr uint32_t VARIANCE_THRESHOLD = 25;
    return variance < VARIANCE_THRESHOLD;
}

uint32_t RSSIDetector::calculate_variance() const noexcept {
    if (samples_count_ < 2) {
        return 0;
    }

    uint32_t sum_squared_diff = 0;
    const uint32_t count = (samples_count_ > RSSI_HISTORY_SIZE) ? RSSI_HISTORY_SIZE : static_cast<uint32_t>(samples_count_);

    uint32_t sum = 0;
    for (size_t i = 0; i < count; ++i) {
        sum += static_cast<uint32_t>(rssi_history_[i] - RSSI_MIN_DBM);
    }

    const int32_t mean = static_cast<int32_t>((sum / count) + RSSI_MIN_DBM);

    for (size_t i = 0; i < count; ++i) {
        const int32_t diff = static_cast<int32_t>(rssi_history_[i]) - mean;
        sum_squared_diff += static_cast<uint32_t>(diff * diff);
    }

    return sum_squared_diff / count;
}

} // namespace drone_analyzer
