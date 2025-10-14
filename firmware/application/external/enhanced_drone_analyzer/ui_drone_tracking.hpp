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
    int16_t prev_rssi;
    systime_t last_seen;
    uint8_t drone_type;
    uint8_t threat_level;
    uint8_t update_count;
    uint8_t trend_history;

    TrackedDrone() : frequency(0), last_rssi(-120), prev_rssi(-120),
                     last_seen(0), drone_type(0), threat_level(0),
                     update_count(0), trend_history(0) {}

    void add_rssi(int16_t rssi, systime_t time);
    MovementTrend get_trend() const;
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
