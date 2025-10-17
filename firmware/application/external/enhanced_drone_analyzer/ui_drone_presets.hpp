// ui_drone_presets.hpp
// Drone frequency presets for quick setup in Enhanced Drone Analyzer
// Predefined frequencies and settings for popular drone models

#ifndef __UI_DRONE_PRESETS_HPP__
#define __UI_DRONE_PRESETS_HPP__

#include "ui_drone_types.hpp"
#include <vector>
#include <string>

namespace ui::external_app::enhanced_drone_analyzer {

// Preset structure for drone frequency templates
struct DronePreset {
    std::string display_name;      // Name shown in preset selection menu
    uint32_t frequency_hz;        // Primary frequency
    DroneType drone_type;         // Drone model category
    ThreatLevel threat_level;     // Risk assessment
    int32_t rssi_threshold_db;   // Detection sensitivity
    std::string name_template;    // Generated name template
};

// Collection of known drone frequency presets
// TODO: Replace with actual frequencies provided by user
class DroneFrequencyPresets {
public:
    // Get all available presets for menu display
    static const std::vector<DronePreset>& get_all_presets();

    // Get presets filtered by drone type
    static std::vector<DronePreset> get_presets_by_type(DroneType type);

    // Get preset by display name (for fast lookup)
    static const DronePreset* find_preset_by_name(const std::string& display_name);

    // Get all unique display names for menu construction
    static std::vector<std::string> get_preset_names();

private:
    // STATIC PRESET DATA - TO BE REPLACED WITH USER-PROVIDED FREQUENCIES
    // Example presets (will be updated with real data)
    static const std::vector<DronePreset> s_presets;
};

// Preset selection UI helper class
class DronePresetSelector {
public:
    static void show_preset_menu(NavigationView& nav,
                                std::function<void(const DronePreset&)> on_selected);
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_PRESETS_HPP__
