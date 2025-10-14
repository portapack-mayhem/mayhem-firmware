// ui_drone_tracking.hpp
// Drone tracking and threat management

#ifndef __UI_DRONE_TRACKING_HPP__
#define __UI_DRONE_TRACKING_HPP__

#include "ui.hpp"
#include "ui_drone_types.hpp"  // For unified types
#include <array>

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
