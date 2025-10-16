// ui_drone_presets.cpp
// Implementation of drone frequency presets for quick setup in Enhanced Drone Analyzer

#include "ui_drone_presets.hpp"
#include "ui/ui_menu.hpp"
#include "ui/navigation.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

// STATIC PRESET DATA - TO BE REPLACED WITH REAL DRONE FREQUENCIES
// This is placeholder data - user will provide actual frequencies
const std::vector<DronePreset> DroneFrequencyPresets::s_presets = {
    // Example DJI drones
    {"DJI Mini 2",         2450000000, DroneType::DJI_MINI,   ThreatLevel::LOW,     -75, "DJI Mini 2"},
    {"DJI Mini 3 Pro",     2400000000, DroneType::DJI_MINI,   ThreatLevel::LOW,     -70, "DJI Mini 3"},
    {"DJI Mavic Air 2",    2400000000, DroneType::DJI_MAVIC,  ThreatLevel::MEDIUM,  -75, "Mavic Air 2"},

    // Example FPV racing drones
    {"EMAX Tinyhawk",      560000000,  DroneType::FPV_RACING, ThreatLevel::LOW,     -80, "Tinyhawk"},
    {"Eachine Wizard",     580000000,  DroneType::FPV_RACING, ThreatLevel::MEDIUM,  -75, "Wizard"},

    // Example military drones (High Threat)
    {"Orlan-10 Recon",     433920000,  DroneType::ORLAN_10,   ThreatLevel::HIGH,    -85, "Orlan-10"},
    {"Lancet Kamikaze",    433920000,  DroneType::LANCET,     ThreatLevel::CRITICAL,-90, "Lancet"},

    // More examples for demonstration
    {"Phantom 4",          2450000000, DroneType::DJI_MAVIC,  ThreatLevel::MEDIUM,  -75, "Phantom 4"},
    {"Mavic 2",            2450000000, DroneType::DJI_MAVIC,  ThreatLevel::MEDIUM,  -75, "Mavic 2"},
};

// Preset implementation
const std::vector<DronePreset>& DroneFrequencyPresets::get_all_presets() {
    return s_presets;
}

std::vector<DronePreset> DroneFrequencyPresets::get_presets_by_type(DroneType type) {
    std::vector<DronePreset> filtered;
    for (const auto& preset : s_presets) {
        if (preset.drone_type == type) {
            filtered.push_back(preset);
        }
    }
    return filtered;
}

const DronePreset* DroneFrequencyPresets::find_preset_by_name(const std::string& display_name) {
    for (const auto& preset : s_presets) {
        if (preset.display_name == display_name) {
            return &preset;
        }
    }
    return nullptr;
}

std::vector<std::string> DroneFrequencyPresets::get_preset_names() {
    std::vector<std::string> names;
    for (const auto& preset : s_presets) {
        names.push_back(preset.display_name);
    }
    return names;
}

// Preset selector implementation
void DronePresetSelector::show_preset_menu(NavigationView& nav,
                                          std::function<void(const DronePreset&)> on_selected) {
    // Create menu with preset options
    auto preset_names = DroneFrequencyPresets::get_preset_names();

    nav.push<MenuView>(
        "Select Drone Preset",
        Icon::Drone,
        Text::Line{},
        Text::Line{},
        &nav,
        &nav,
        [on_selected](MenuChoice choice) {
            const auto* preset = DroneFrequencyPresets::find_preset_by_name(choice.chosen_text);
            if (preset && on_selected) {
                on_selected(*preset);
            }
        }
    );

    // Add preset options to menu
    const auto* menu_view = reinterpret_cast<MenuView*>(nav.children_.back());
    if (menu_view) {
        for (const auto& name : preset_names) {
            menu_view->add_item({
                name,
                Theme::getInstance()->fg_light->foreground,
                nullptr,  // No bitmap
                [name, on_selected]() {
                    const auto* preset = DroneFrequencyPresets::find_preset_by_name(name);
                    if (preset && on_selected) {
                        on_selected(*preset);
                    }
                }
            });
        }
    }
}

} // namespace ui::external_app::enhanced_drone_analyzer
