// ui_drone_presets.cpp
// Implementation of drone frequency presets for quick setup in Enhanced Drone Analyzer

#include "ui_drone_presets.hpp"
#include "ui/ui_menu.hpp"
#include "ui/navigation.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

// REAL DRONE PRESETS - Based on actual drone frequency database
// All frequencies converted from MHz/GHz to Hz
const std::vector<DronePreset> DroneFrequencyPresets::s_presets = {

    // ===== CIVILIAN/COMMERCIAL DRONES =====

    // DJI Consumer drones (most common consumer UAVs)
    {"DJI Mini 4 Pro",       2400000000, DroneType::DJI_MINI,   ThreatLevel::NONE,     -75, "DJI Mini 4 Pro"},
    {"DJI Mini 4 Pro (5.8GHz)", 5800000000, DroneType::DJI_MINI,   ThreatLevel::NONE,     -75, "DJI Mini 4 Pro (Video)"},
    {"DJI Mavic 3 Pro",      2400000000, DroneType::DJI_MAVIC,  ThreatLevel::NONE,     -70, "DJI Mavic 3 Pro"},
    {"DJI Mavic 3 Pro (5.8GHz)", 5800000000, DroneType::DJI_MAVIC,  ThreatLevel::NONE,     -70, "DJI Mavic 3 Pro (Video)"},

    // Autel UAVs (commercial competitor to DJI)
    {"Autel EVO Max 4T",     902000000,  DroneType::DJI_MAVIC,  ThreatLevel::NONE,     -72, "Autel EVO Max 4T"},
    {"Autel EVO Max 4T (2.4GHz)", 2400000000, DroneType::DJI_MAVIC,  ThreatLevel::NONE,     -72, "Autel EVO Max 4T (2.4GHz)"},
    {"Autel EVO Max 4T (5.8GHz)", 5800000000, DroneType::DJI_MAVIC,  ThreatLevel::NONE,     -72, "Autel EVO Max 4T (Video)"},

    // Parrot drones (European manufacturer)
    {"Parrot ANAFI USA",     2400000000, DroneType::UNKNOWN,   ThreatLevel::NONE,     -78, "Parrot ANAFI USA"},
    {"Parrot ANAFI USA (Video)", 5800000000, DroneType::UNKNOWN,   ThreatLevel::NONE,     -78, "Parrot ANAFI USA (5.8GHz)"},

    // FPV Racing drones (hobby/sport drones)
    {"Analog FPV (2.4GHz)",  2400000000, DroneType::FPV_RACING, ThreatLevel::NONE,     -80, "Analog FPV Racing"},
    {"Digital FPV (5.8GHz)", 5800000000, DroneType::FPV_RACING, ThreatLevel::NONE,     -75, "Digital FPV Racing"},
    {"Long Range FPV (433MHz)", 433920000,  DroneType::FPV_RACING, ThreatLevel::NONE,     -85, "Long Range FPV"},
    {"Long Range FPV (868MHz)", 868000000,  DroneType::FPV_RACING, ThreatLevel::NONE,     -82, "Long Range FPV (868MHz)"},

    // Skydio autonomous drones
    {"Skydio X10 (5.8GHz)",  5800000000, DroneType::UNKNOWN,   ThreatLevel::NONE,     -75, "Skydio X10"},

    // ===== MILITARY/TACTICAL DRONES =====

    // Russian tactical UAVs (high threat)
    {"Orlan-10 (200MHz)",    200000000,  DroneType::ORLAN_10,  ThreatLevel::HIGH,    -85, "Orlan-10 Tactical"},
    {"Orlan-10 (433MHz)",    433920000,  DroneType::ORLAN_10,  ThreatLevel::HIGH,    -85, "Orlan-10 Tactical"},
    {"Orlan-10 (868MHz)",    868000000,  DroneType::ORLAN_10,  ThreatLevel::HIGH,    -85, "Orlan-10 Tactical"},
    {"Orlan-10 (915MHz)",    915000000,  DroneType::ORLAN_10,  ThreatLevel::HIGH,    -85, "Orlan-10 Tactical"},
    {"Orlan-10 (1.2GHz)",    1200000000, DroneType::ORLAN_10,  ThreatLevel::HIGH,    -80, "Orlan-10 Tactical"},
    {"Orlan-10 (2.4GHz)",    2400000000, DroneType::ORLAN_10,  ThreatLevel::HIGH,    -80, "Orlan-10 Tactical"},
    {"Orlan-10 (Video)",     900000000,  DroneType::ORLAN_10,  ThreatLevel::HIGH,    -82, "Orlan-10 Telemetry"},

    // Russian precision munitions (critical threat)
    {"Lancet-3 (900MHz)",    900000000,  DroneType::LANCET,    ThreatLevel::CRITICAL,-82, "Lancet-3 Loitering"},
    {"Lancet-3 (5.8GHz)",    5800000000, DroneType::LANCET,    ThreatLevel::CRITICAL,-75, "Lancet-3 Loitering"},
    {"Shahed-131/136 (800MHz)", 800000000, DroneType::LANCET,    ThreatLevel::CRITICAL,-85, "Shahed Loitering"},
    {"Shahed-131/136 (2.6GHz)", 2600000000, DroneType::LANCET,    ThreatLevel::CRITICAL,-78, "Shahed Loitering"},

    // Eleron tactical UAV
    {"Eleron-3SV (868MHz)",  868000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "Eleron-3SV Tactical"},
    {"Eleron-3SV (915MHz)",  915000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "Eleron-3SV Tactical"},
    {"Eleron-3SV (1.2GHz)",  1200000000, DroneType::UNKNOWN,   ThreatLevel::HIGH,    -82, "Eleron-3SV Tactical"},

    // Forpost tactical UAV (Ukrainian/Russian variant)
    {"Forpost-R (465MHz)",   465000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "Forpost-R Tactical"},
    {"Forpost-R (5.8GHz)",   5800000000, DroneType::UNKNOWN,   ThreatLevel::HIGH,    -75, "Forpost-R Tactical"},

    // Bayraktar Turkish MALE UAV (very high threat)
    {"Bayraktar TB2 (868MHz)", 868000000,  DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-85, "Bayraktar TB2 MALE"},
    {"Bayraktar TB2 (915MHz)", 915000000,  DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-85, "Bayraktar TB2 MALE"},
    {"Bayraktar TB2 (2.4GHz)", 2400000000, DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-80, "Bayraktar TB2 MALE"},
    {"Bayraktar TB2 (5.8GHz)", 5800000000, DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-75, "Bayraktar TB2 MALE"},

    // Chinese MALE/export UAVs (high threat for regional conflicts)
    {"CH-4B Rainbow (900MHz)", 900000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "CH-4B Rainbow MALE"},
    {"CH-4B Rainbow (4.4GHz)", 4400000000, DroneType::UNKNOWN,   ThreatLevel::HIGH,    -78, "CH-4B Rainbow MALE"},
    {"CH-5 Rainbow (841MHz)",  841000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "CH-5 Rainbow MALE"},
    {"CH-5 Rainbow (1.4GHz)",  1400000000, DroneType::UNKNOWN,   ThreatLevel::HIGH,    -80, "CH-5 Rainbow MALE"},

    // Israeli tactical UAVs
    {"Hermes 450 (UHF)",     600000000,  DroneType::UNKNOWN,   ThreatLevel::HIGH,    -85, "Hermes 450 Tactical"},
    {"Hermes 450 (C-band)",  5000000000, DroneType::UNKNOWN,   ThreatLevel::HIGH,    -75, "Hermes 450 Tactical"},

    // US military drones (maximum threat)
    {"MQ-9 Reaper (UHF)",    300000000,  DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-90, "MQ-9 Reaper HALE"},
    {"MQ-9 Reaper (C-band)", 5500000000, DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-75, "MQ-9 Reaper HALE"},
    {"RQ-4 Global Hawk (UHF)", 300000000, DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-90, "RQ-4 Global Hawk HALE"},
    {"RQ-4 Global Hawk (Ku)", 13000000000,DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-70, "RQ-4 Global Hawk HALE"},

    // Precision munitions
    {"Switchblade 600",      1300000000, DroneType::LANCET,    ThreatLevel::CRITICAL,-82, "Switchblade 600 Munition"},

    // Ground vehicles (lower priority but still tracked)
    {"Marker UGV (2.4GHz)",  2400000000, DroneType::UNKNOWN,   ThreatLevel::MEDIUM,  -80, "Marker UGV"},
    {"Marker UGV (LTE 700)", 700000000,  DroneType::UNKNOWN,   ThreatLevel::MEDIUM,  -85, "Marker UGV"},
    {"Uran-6 UGV",           400000000,  DroneType::UNKNOWN,   ThreatLevel::MEDIUM,  -85, "Uran-6 UGV"},

    // SATCOM frequencies (for long-range drones)
    {"Military SATCOM (UHF)", 300000000,  DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-88, "Military SATCOM UHF"},
    {"Military SATCOM (C-band)", 5500000000, DroneType::UNKNOWN, ThreatLevel::CRITICAL,-75, "Military SATCOM C-band"},
    {"Military SATCOM (Ku)", 13000000000, DroneType::UNKNOWN,   ThreatLevel::CRITICAL,-70, "Military SATCOM Ku-band"},
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
