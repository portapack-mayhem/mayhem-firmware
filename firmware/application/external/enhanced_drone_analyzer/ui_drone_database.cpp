// ui_drone_database.cpp
// Implementation of Drone frequency database
// Step 3.1 of modular refactoring

#include "ui_drone_database.hpp"

// DroneFrequencyDatabase implementation
DroneFrequencyDatabase::DroneFrequencyDatabase() : active_entries_(0) {
    initialize_database();
}

void DroneFrequencyDatabase::initialize_database() {
    // Initialize from the constexpr database
    for (const auto& entry : ENHANCED_DRONE_FREQUENCY_DATABASE) {
        if (entry.is_valid() && active_entries_ < MAX_ENTRIES) {
            entries_[active_entries_] = entry;
            active_entries_++;
        }
    }
}

void DroneFrequencyDatabase::add_entry(const DroneFrequencyEntry& entry) {
    if (entry.is_valid() && active_entries_ < MAX_ENTRIES) {
        entries_[active_entries_] = entry;
        active_entries_++;
    }
}

const DroneFrequencyEntry* DroneFrequencyDatabase::get_entry(size_t index) const {
    if (index < active_entries_) {
        return &entries_[index];
    }
    return nullptr;
}

const DroneFrequencyEntry* DroneFrequencyDatabase::lookup_frequency(uint32_t frequency_hz) const {
    // Simple linear search (acceptable for embedded systems with <64 entries)
    for (size_t i = 0; i < active_entries_; ++i) {
        // Check if frequency falls within the bandwidth of this entry
        uint32_t half_bandwidth = entries_[i].bandwidth_hz / 2;
        uint32_t min_freq = entries_[i].frequency_hz - half_bandwidth;
        uint32_t max_freq = entries_[i].frequency_hz + half_bandwidth;

        if (frequency_hz >= min_freq && frequency_hz <= max_freq) {
            return &entries_[i];
        }
    }
    return nullptr;  // Not found
}

ThreatLevel DroneFrequencyDatabase::get_threat_level(uint32_t frequency_hz) const {
    const DroneFrequencyEntry* entry = lookup_frequency(frequency_hz);
    if (entry) {
        return entry->threat_level;
    }

    // Default to unknown/medium risk for frequencies in UAV ranges
    if (frequency_hz >= 100000000U && frequency_hz <= 6000000000U) {  // 100MHz to 6GHz
        return ThreatLevel::MEDIUM;  // Potentially dangerous range
    }

    return ThreatLevel::NONE;  // Outside known UAV frequency ranges
}
