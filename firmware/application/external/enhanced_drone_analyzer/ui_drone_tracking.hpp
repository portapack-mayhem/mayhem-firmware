// ui_drone_tracking.hpp
// Drone tracking and threat management

#ifndef __UI_DRONE_TRACKING_HPP__
#define __UI_DRONE_TRACKING_HPP__

#include "ui.hpp"
#include <array>

namespace ui::external_app::enhanced_drone_analyzer {

enum class MovementTrend : uint8_t {
    STATIC = 0,
    APPROACHING = 1,
    RECEDING = 2,
    UNKNOWN = 3
};

struct TrackedDrone {
    uint32_t frequency;
    int16_t last_rssi;
    systime_t last_seen;
    uint8_t drone_type;
    uint8_t threat_level;
    uint8_t update_count;

    // IMPROVED: Ring buffer from V0 concept - no heap allocation, efficient RSSI history
    static constexpr size_t MAX_RSSI_HISTORY = 8;  // Optimized for embedded RAM
    int16_t rssi_buffer_[MAX_RSSI_HISTORY];        // Fixed-size ring buffer
    uint16_t head_;                                // Ring buffer head pointer
    uint16_t size_;                                // Current buffer size

    TrackedDrone() : frequency(0), last_rssi(-120), last_seen(0), drone_type(0),
                     threat_level(0), update_count(0), head_(0), size_(0) {
        memset(rssi_buffer_, -120, sizeof(rssi_buffer_));  // Initialize with noise floor
    }

    // ENHANCED: Ring buffer-based RSSI tracking (V0 concept, improved implementation)
    void add_rssi(int16_t rssi, systime_t time) {
        // Add to ring buffer (circular)
        if (size_ < MAX_RSSI_HISTORY) {
            rssi_buffer_[size_] = rssi;
            size_++;
        } else {
            // Overwrite oldest value
            rssi_buffer_[head_] = rssi;
            head_ = (head_ + 1) % MAX_RSSI_HISTORY;
        }

        last_rssi = rssi;
        last_seen = time;
        if (update_count < 255) update_count++;
    }

    // IMPROVED: Trend analysis using ring buffer data
    MovementTrend get_trend() const {
        if (size_ < 2) return MovementTrend::UNKNOWN;

        // Use last 3 samples for trend analysis
        size_t samples_needed = std::min(static_cast<size_t>(3), size_);
        int16_t recent_rssi = 0, older_rssi = 0;

        // Extract recent RSSI (newest samples)
        size_t recent_idx = (head_ + MAX_RSSI_HISTORY - 1) % MAX_RSSI_HISTORY;
        recent_rssi = rssi_buffer_[recent_idx];

        // Extract older RSSI
        size_t older_idx = (head_ + MAX_RSSI_HISTORY - samples_needed / 2) % MAX_RSSI_HISTORY;
        older_rssi = rssi_buffer_[older_idx];

        const int16_t TREND_THRESHOLD_DB = 3;
        int16_t diff = recent_rssi - older_rssi;

        if (diff > TREND_THRESHOLD_DB) return MovementTrend::APPROACHING;
        if (diff < -TREND_THRESHOLD_DB) return MovementTrend::RECEDING;
        return MovementTrend::STATIC;
    }

    // CLEAN: Should cleanup method using ring buffer data
    bool should_cleanup(int32_t threshold) const {
        if (size_ == 0) return false;

        // Check if all recent RSSI values are below threshold
        for (uint16_t i = 0; i < size_; ++i) {
            size_t buffer_idx = (head_ + MAX_RSSI_HISTORY - 1 - i) % MAX_RSSI_HISTORY;
            if (rssi_buffer_[buffer_idx] > threshold) {
                return false;  // Strong signal detected - keep tracking
            }
        }
        return true;  // All RSSI below threshold - can cleanup
    }
};

class DroneTracker {
private:
    static constexpr size_t MAX_TRACKED_DRONES = 8;
    TrackedDrone tracked_drones_[MAX_TRACKED_DRONES];
    size_t tracked_drones_count_;

    size_t approaching_count_;
    size_t receding_count_;
    size_t static_count_;

public:
    DroneTracker();

    // Drone management
    void update_tracked_drone(DroneType type, rf::Frequency frequency,
                             int32_t rssi, ThreatLevel threat_level);
    void remove_stale_drones();

    // Trend analysis
    void update_tracking_counts();
    void update_trends_compact_display(Text& text_field);

    // Statistics
    size_t get_approaching_count() const { return approaching_count_; }
    size_t get_receding_count() const { return receding_count_; }
    size_t get_static_count() const { return static_count_; }
    size_t get_tracked_count() const { return tracked_drones_count_; }

    // Access
    const TrackedDrone* get_tracked_drone(size_t index) const;
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_TRACKING_HPP__
