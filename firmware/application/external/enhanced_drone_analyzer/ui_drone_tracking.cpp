// ui_drone_tracking.cpp
// Implementation of drone tracking and threat management

#include "ui_drone_tracking.hpp"
#include "ui_drone_spectrum_scanner.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

    // PRIVATE METHOD should be private - fixed in header

// DroneTracker implementation
DroneTracker::DroneTracker()
    : tracked_drones_count_(0)
    , approaching_count_(0)
    , receding_count_(0)
    , static_count_(0) {
    // Initialize all drone slots
}

void DroneTracker::update_tracked_drone(DroneType type, rf::Frequency frequency,
                                      int32_t rssi, ThreatLevel threat_level) {
    // Linear search through fixed array
    for (size_t i = 0; i < MAX_TRACKED_DRONES; i++) {
        TrackedDrone& drone = tracked_drones_[i];

        // Check if this slot matches frequency (existing drone)
        if (drone.frequency == static_cast<uint32_t>(frequency) && drone.update_count > 0) {
            // Update existing drone
            drone.add_rssi(static_cast<int16_t>(rssi), chTimeNow());
            drone.drone_type = static_cast<uint8_t>(type);
            drone.threat_level = static_cast<uint8_t>(threat_level);
            update_tracking_counts();
            return;
        }

        // Check if this slot is free
        if (drone.update_count == 0) {
            // Initialize new drone slot
            drone.frequency = static_cast<uint32_t>(frequency);
            drone.drone_type = static_cast<uint8_t>(type);
            drone.threat_level = static_cast<uint8_t>(threat_level);
            drone.add_rssi(static_cast<int16_t>(rssi), chTimeNow());
            tracked_drones_count_++;
            update_tracking_counts();
            return;
        }
    }

    // All slots occupied - replace oldest
    size_t oldest_index = 0;
    systime_t oldest_time = tracked_drones_[0].last_seen;

    for (size_t i = 1; i < MAX_TRACKED_DRONES; i++) {
        if (tracked_drones_[i].last_seen < oldest_time) {
            oldest_time = tracked_drones_[i].last_seen;
            oldest_index = i;
        }
    }

    // Replace oldest drone
    tracked_drones_[oldest_index] = TrackedDrone();
    tracked_drones_[oldest_index].frequency = static_cast<uint32_t>(frequency);
    tracked_drones_[oldest_index].drone_type = static_cast<uint8_t>(type);
    tracked_drones_[oldest_index].threat_level = static_cast<uint8_t>(threat_level);
    tracked_drones_[oldest_index].add_rssi(static_cast<int16_t>(rssi), chTimeNow());
    update_tracking_counts();
}

void DroneTracker::remove_stale_drones() {
    const systime_t STALE_TIMEOUT = 30000; // 30 seconds
    const int32_t CLEANUP_THRESHOLD = -90;  // dBm threshold for cleanup
    systime_t current_time = chTimeNow();

    size_t write_idx = 0;

    for (size_t read_idx = 0; read_idx < MAX_TRACKED_DRONES; read_idx++) {
        TrackedDrone& drone = tracked_drones_[read_idx];

        if (drone.update_count == 0) continue;

        // ENHANCED: Use ring buffer-based cleanup check (V0 concept)
        bool is_stale = (current_time - drone.last_seen) > STALE_TIMEOUT;
        bool should_cleanup = drone.should_cleanup(CLEANUP_THRESHOLD);

        if (!is_stale && !should_cleanup) {
            if (write_idx != read_idx) {
                tracked_drones_[write_idx] = drone;
            }
            write_idx++;
        } else {
            tracked_drones_[read_idx] = TrackedDrone();
        }
    }

    tracked_drones_count_ = write_idx;
    update_tracking_counts();
}

void DroneTracker::update_tracking_counts() {
    approaching_count_ = 0;
    receding_count_ = 0;
    static_count_ = 0;

    for (size_t i = 0; i < MAX_TRACKED_DRONES; i++) {
        const TrackedDrone& drone = tracked_drones_[i];

        if (drone.update_count == 0 || drone.update_count < 2) continue;

        MovementTrend trend = drone.get_trend();
        switch (trend) {
            case MovementTrend::APPROACHING:
                approaching_count_++;
                break;
            case MovementTrend::RECEDING:
                receding_count_++;
                break;
            case MovementTrend::STATIC:
            case MovementTrend::UNKNOWN:
            default:
                static_count_++;
                break;
        }
    }

    // This would be called from the UI class to update display
}

void DroneTracker::update_trends_compact_display(Text& text_field) {
    char trend_buffer[64];
    snprintf(trend_buffer, sizeof(trend_buffer), "Trends: ▲%lu ■%lu ▼%lu",
             approaching_count_, static_count_, receding_count_);
    text_field.set(trend_buffer);
}

const TrackedDrone* DroneTracker::get_tracked_drone(size_t index) const {
    if (index < MAX_TRACKED_DRONES) {
        return &tracked_drones_[index];
    }
    return nullptr;
}

} // namespace ui::external_app::enhanced_drone_analyzer
