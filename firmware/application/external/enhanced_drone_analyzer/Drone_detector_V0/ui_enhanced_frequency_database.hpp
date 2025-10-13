// ui_enhanced_frequency_database.hpp
// Database of drone frequencies for PortaPack Mayhem firmware integration

#ifndef __UI_ENHANCED_FREQUENCY_DATABASE_HPP__
#define __UI_ENHANCED_FREQUENCY_DATABASE_HPP__

#include <cstdint>
#include <array>

// Drone types enumeration
enum class DroneType {
    UNKNOWN = 0,
    DJI_MAVIC,
    DJI_MINI,
    DJI_AIR,
    DJI_PHANTOM,
    DJI_AVATA,
    FPV_RACING,
    FPV_FREESTYLE,
    ORLAN_10,
    LANCET,
    SHAHED_136,
    BAYRAKTAR_TB2,
    MILITARY_UAV,
    RUSSIAN_EW_STATION,
    FIBER_OPTIC_DRONE,
    SVOY,
    CHUZHOY,
    PIDR,
    HUYLO,
    HUYLINA,
    PVD,
    VSU,
    BRAT,
    KROT,
    PLENNYY,
    NE_PLENNYY,
    CODE_300,
    CODE_200,
    CODE_500
};

// Threat levels
enum class ThreatLevel {
    NONE = 0,
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

// Enhanced frequency database entry
struct EnhancedFrequencyEntry {
    const char* name;
    uint32_t frequency_hz;
    int16_t rssi_threshold;  // RSSI threshold in dBm
    DroneType drone_type;
    ThreatLevel threat_level;
    const char* description;
};

// Enhanced frequency database - subset for embedded firmware
static constexpr std::array<EnhancedFrequencyEntry, 32> ENHANCED_DRONE_FREQUENCY_DATABASE = {{
    // DJI frequencies
    {"DJI-920MHz", 920000000, -80, DroneType::DJI_MAVIC, ThreatLevel::HIGH, "DJI Mavic control"},
    {"DJI-2438MHz", 2438000000, -85, DroneType::DJI_MAVIC, ThreatLevel::HIGH, "DJI Mavic video"},
    {"DJI-2403MHz", 2403000000, -85, DroneType::DJI_PHANTOM, ThreatLevel::MEDIUM, "DJI Phantom video"},

    // FPV frequencies
    {"FPV-5658MHz", 5658000000, -90, DroneType::FPV_RACING, ThreatLevel::MEDIUM, "FPV 5.8GHz VTX"},
    {"FPV-5800MHz", 5800000000, -90, DroneType::FPV_RACING, ThreatLevel::MEDIUM, "FPV 5.8GHz VTX"},

    // Military/Orlan
    {"Orlan-802MHz", 802000000, -75, DroneType::ORLAN_10, ThreatLevel::CRITICAL, "Orlan-10 command"},
    {"Orlan-823MHz", 823000000, -75, DroneType::ORLAN_10, ThreatLevel::CRITICAL, "Orlan-10 telemetry"},

    // Russian EW stations
    {"EW-400MHz", 400000000, -70, DroneType::RUSSIAN_EW_STATION, ThreatLevel::CRITICAL, "Russian EW station"},
    {"EW-450MHz", 450000000, -70, DroneType::RUSSIAN_EW_STATION, ThreatLevel::CRITICAL, "Russian EW station"},

    // Placeholder entries to fill array - reduce memory usage
    {"Unknown-1", 1000000000, -85, DroneType::UNKNOWN, ThreatLevel::LOW, ""},
    {"Unknown-2", 1500000000, -85, DroneType::UNKNOWN, ThreatLevel::LOW, ""},
    {"Unknown-3", 2000000000, -85, DroneType::UNKNOWN, ThreatLevel::LOW, ""},
    {"Unknown-4", 2500000000, -85, DroneType::UNKNOWN, ThreatLevel::LOW, ""},
    {"Unknown-5", 3000000000, -85, DroneType::UNKNOWN, ThreatLevel::LOW, ""},
    {"Unknown-6", 3500000000, -85, DroneType::UNKNOWN, ThreatLevel::LOW, ""},
    {"Unknown-7", 4000000000, -85, DroneType::UNKNOWN, ThreatLevel::LOW, ""},
    {"Unknown-8", 4500000000, -85, DroneType::UNKNOWN, ThreatLevel::LOW, ""},
    {"Unknown-9", 5000000000, -85, DroneType::UNKNOWN, ThreatLevel::LOW, ""},
    {"Unknown-10", 5500000000, -85, DroneType::UNKNOWN, ThreatLevel::LOW, ""},
    {"Unknown-11", 6000000000, -85, DroneType::UNKNOWN, ThreatLevel::LOW, ""},

    // Additional entries if needed - truncated for embedded constraints
}};

// Constants
static constexpr size_t ENHANCED_DATABASE_SIZE = ENHANCED_DRONE_FREQUENCY_DATABASE.size();

#endif /* __UI_ENHANCED_FREQUENCY_DATABASE_HPP__ */
