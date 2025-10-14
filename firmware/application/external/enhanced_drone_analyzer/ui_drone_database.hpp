// ui_drone_database.hpp
// Drone frequency database for Enhanced Drone Analyzer
// Step 3.1 of modular refactoring - add frequency database

#ifndef __UI_DRONE_DATABASE_HPP__
#define __UI_DRONE_DATABASE_HPP__

// INCLUDES (minimal for embedded compatibility)
#include "ui_drone_types.hpp"  // Unified types
#include <array>
#include <string>

// FREQUENCY DATABASE ENTRY
struct DroneFrequencyEntry {
    uint32_t frequency_hz;
    DroneType drone_type;
    ThreatLevel threat_level;
    int32_t rssi_threshold_db;
    const char* name;
    uint32_t bandwidth_hz;  // Scanning bandwidth

    // Check if frequency is valid
    bool is_valid() const {
        return frequency_hz > 0 && frequency_hz < 10000000000UL;  // 0 to 10GHz
    }
};

// FREQUENCY DATABASE CLASS WITH CRUD OPERATIONS
class DroneFrequencyDatabase {
public:
    static constexpr size_t MAX_ENTRIES = 64;  // Enough for common drone frequencies

    // Iterator support for easier usage
    class iterator {
    public:
        iterator(DroneFrequencyDatabase& db, size_t index = 0) : db_{db}, index_{index} {}
        iterator& operator++() { ++index_; return *this; }
        bool operator==(const iterator& other) const { return index_ == other.index_; }
        bool operator!=(const iterator& other) const { return !(*this == other); }
        const DroneFrequencyEntry& operator*() const { return db_.entries_[index_]; }

    private:
        DroneFrequencyDatabase& db_;
        size_t index_;
    };

    DroneFrequencyDatabase();

    // CRUD OPERATIONS - Full support for adding/removing/modifying frequencies
    bool add_entry(const DroneFrequencyEntry& entry);        // Add new frequency entry
    bool update_entry(size_t index, const DroneFrequencyEntry& entry); // Update existing entry
    bool remove_entry(size_t index);                              // Remove entry by index
    bool remove_entry_by_frequency(uint32_t frequency_hz);       // Remove entry by frequency
    void clear_database();                                     // Remove all entries

    // Get frequency by index
    const DroneFrequencyEntry* get_entry(size_t index) const;

    // Number of active entries
    size_t size() const { return active_entries_; }
    size_t max_size() const { return MAX_ENTRIES; }
    bool is_empty() const { return active_entries_ == 0; }
    bool is_full() const { return active_entries_ == MAX_ENTRIES; }

    // Lookup frequency in database
    const DroneFrequencyEntry* lookup_frequency(uint32_t frequency_hz) const;

    // Check if frequency contains known threats
    ThreatLevel get_threat_level(uint32_t frequency_hz) const;

    // Iterator support
    iterator begin() { return iterator(*this, 0); }
    iterator end() { return iterator(*this, active_entries_); }

    // Validation
    bool validate_entry(const DroneFrequencyEntry& entry) const;
    bool entry_exists(uint32_t frequency_hz) const;

    // Database management
    void initialize_database();  // Load default entries
    void reset_to_defaults();     // Reset to initial default database

private:
    std::array<DroneFrequencyEntry, MAX_ENTRIES> entries_;
    size_t active_entries_;

    // Helper methods
    void compact_entries();  // Remove gaps after deletion
    size_t find_entry_index(uint32_t frequency_hz) const;
};

// CONSTEXPR DATABASE (compile-time initialization for embedded systems)
const DroneFrequencyEntry ENHANCED_DRONE_FREQUENCY_DATABASE[] = {
    // DJI MINI/MINI SE (2400-2483.5 MHz)
    {2427000000U, DroneType::DJI_MINI, ThreatLevel::MEDIUM, -70, "DJI Mini 2.4GHz", 5000000},
    {2450000000U, DroneType::DJI_MINI, ThreatLevel::MEDIUM, -70, "DJI Mini Main", 5000000},

    // DJI MAVIC (5150-5350 MHz)
    {5150000000U, DroneType::DJI_MAVIC, ThreatLevel::HIGH, -75, "DJI Mavic Video", 10000000},
    {5250000000U, DroneType::DJI_MAVIC, ThreatLevel::HIGH, -75, "DJI Mavic Control", 10000000},

    // DJI PHANTOM (2400-2483.5 MHz + 5150-5885 MHz)
    {2412000000U, DroneType::DJI_PHANTOM, ThreatLevel::HIGH, -70, "DJI Phantom 2.4GHz", 5000000},
    {5745000000U, DroneType::DJI_PHANTOM, ThreatLevel::HIGH, -75, "DJI Phantom 5.7GHz", 10000000},

    // FPV DRONES (800MHz-900MHz, 1.2GHz-1.3GHz, 2.4GHz)
    {800000000U, DroneType::FPV_RACING, ThreatLevel::MEDIUM, -65, "FPV 800MHz Low", 5000000},
    {850000000U, DroneType::FPV_RACING, ThreatLevel::MEDIUM, -65, "FPV 800MHz High", 5000000},
    {1200000000U, DroneType::FPV_RACING, ThreatLevel::HIGH, -70, "FPV 1.2GHz", 10000000},
    {1300000000U, DroneType::FPV_RACING, ThreatLevel::HIGH, -70, "FPV 1.3GHz", 10000000},
    {2400000000U, DroneType::FPV_RACING, ThreatLevel::MEDIUM, -70, "FPV 2.4GHz", 5000000},

    // RUSSIAN MILITARY DRONES
    {2400000000U, DroneType::ORLAN_10, ThreatLevel::CRITICAL, -80, "Orlan-10 Control", 20000000},
    {5800000000U, DroneType::ORLAN_10, ThreatLevel::CRITICAL, -80, "Orlan-10 Video", 20000000},

    {2400000000U, DroneType::LANCET, ThreatLevel::CRITICAL, -85, "Lancet Control", 20000000},
    {5950000000U, DroneType::LANCET, ThreatLevel::CRITICAL, -85, "Lancet Video", 20000000},

    {16000000000ULL, DroneType::SHAHED_136, ThreatLevel::CRITICAL, -90, "Shahed-136 Video", 50000000},
    {2400000000U, DroneType::SHAHED_136, ThreatLevel::CRITICAL, -85, "Shahed-136 Control", 20000000},

    {1450000000U, DroneType::BAYRAKTAR_TB2, ThreatLevel::CRITICAL, -90, "Bayraktar TB2 Control", 50000000},

    // RUSSIAN EW STATIONS (jam UAV signals)
    {1000000000U, DroneType::RUSSIAN_EW_STATION, ThreatLevel::CRITICAL, -95, "EW Station 1GHz", 500000000},
    {2000000000U, DroneType::RUSSIAN_EW_STATION, ThreatLevel::CRITICAL, -95, "EW Station 2GHz", 1000000000},
    {5000000000U, DroneType::RUSSIAN_EW_STATION, ThreatLevel::CRITICAL, -95, "EW Station 5GHz", 2000000000},

    // FIBER OPTIC DRONES (satellite link backup)
    {14000000000ULL, DroneType::FIBER_OPTIC_DRONE, ThreatLevel::CRITICAL, -95, "Fiber Optic UAV Ku", 500000000},

    // RUSSIAN CODENAMES (АНОНИМНЫЕ СИГНАЛЫ)
    {400000000U, DroneType::SVOY, ThreatLevel::LOW, -50, "СВОЙ", 5000000},
    {450000000U, DroneType::CHUZHOY, ThreatLevel::HIGH, -50, "ЧУЖОЙ", 5000000},
    {500000000U, DroneType::PIDR, ThreatLevel::CRITICAL, -55, "ПИДР", 5000000},
    {600000000U, DroneType::HUYLO, ThreatLevel::CRITICAL, -60, "ХУЙЛО", 5000000},
    {700000000U, DroneType::HUYLINA, ThreatLevel::CRITICAL, -65, "ХУЙЛИНА", 5000000},
    {800000000U, DroneType::PVD, ThreatLevel::CRITICAL, -70, "ПВД", 5000000},
    {900000000U, DroneType::VSU, ThreatLevel::CRITICAL, -75, "ВСУ УГРОЗА", 5000000},
    {1050000000U, DroneType::BRAT, ThreatLevel::MEDIUM, -70, "БРАТ", 5000000},
    {1100000000U, DroneType::KROT, ThreatLevel::HIGH, -75, "КРОТ-ПРЕДАТЕЛЬ", 5000000},
    {1150000000U, DroneType::PLENNYY, ThreatLevel::LOW, -65, "ПЛЕННЫЙ", 5000000},
    {1200000000U, DroneType::NE_PLENNYY, ThreatLevel::LOW, -65, "НЕ ПЛЕННЫЙ", 5000000},

    // SPECIAL CODES (emergency response signals)
    {300000000U, DroneType::CODE_300, ThreatLevel::HIGH, -40, "КОД-300 КРАЙНЯЯ ОПАСНОСТЬ", 2000000},
    {200000000U, DroneType::CODE_200, ThreatLevel::MEDIUM, -40, "КОД-200 ОПАСНОСТЬ", 2000000},
    {500000000U, DroneType::CODE_500, ThreatLevel::CRITICAL, -45, "КОД-500 ПОМОГИТЕ", 2000000}
};

#endif // __UI_DRONE_DATABASE_HPP__
