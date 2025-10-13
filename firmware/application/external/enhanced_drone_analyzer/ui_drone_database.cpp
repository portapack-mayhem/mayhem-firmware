// ui_drone_database.cpp
// Implementation of Drone frequency database with full CRUD operations
// Step 3.2 of modular refactoring - add runtime database management

#include "ui_drone_database.hpp"

// DroneFrequencyDatabase implementation with CRUD support
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

// VALIDATION METHODS
bool DroneFrequencyDatabase::validate_entry(const DroneFrequencyEntry& entry) const {
    // Basic validation
    if (!entry.is_valid()) return false;

    // Check frequency range (50MHz - 10GHz for UAV applications)
    if (entry.frequency_hz < 50000000UL || entry.frequency_hz > 10000000000ULL) return false;

    // Check threat level bounds
    if (static_cast<int>(entry.threat_level) < 0 ||
        static_cast<int>(entry.threat_level) >= static_cast<int>(ThreatLevel::MAX_LEVELS)) return false;

    // Check drone type bounds
    if (static_cast<int>(entry.drone_type) < 0 ||
        static_cast<int>(entry.drone_type) >= static_cast<int>(DroneType::MAX_TYPES)) return false;

    // Check bandwidth is reasonable
    if (entry.bandwidth_hz < 1000 || entry.bandwidth_hz > 100000000) return false; // 1kHz - 100MHz

    // Check RSSI threshold in reasonable range
    if (entry.rssi_threshold_db > -10 || entry.rssi_threshold_db < -120) return false;

    return true;
}

bool DroneFrequencyDatabase::entry_exists(uint32_t frequency_hz) const {
    return find_entry_index(frequency_hz) != SIZE_MAX; // SIZE_MAX indicates not found
}

size_t DroneFrequencyDatabase::find_entry_index(uint32_t frequency_hz) const {
    // Linear search - acceptable for embedded systems with <64 entries
    for (size_t i = 0; i < active_entries_; ++i) {
        // Check if frequency falls within the bandwidth of this entry
        uint32_t half_bandwidth = entries_[i].bandwidth_hz / 2;
        uint32_t min_freq = entries_[i].frequency_hz - half_bandwidth;
        uint32_t max_freq = entries_[i].frequency_hz + half_bandwidth;

        if (frequency_hz >= min_freq && frequency_hz <= max_freq) {
            return i;
        }
    }
    return SIZE_MAX; // Not found
}

// CRUD OPERATIONS

// Add new frequency entry
bool DroneFrequencyDatabase::add_entry(const DroneFrequencyEntry& entry) {
    // Validation
    if (!validate_entry(entry)) return false;
    if (active_entries_ >= MAX_ENTRIES) return false; // Database full
    if (entry_exists(entry.frequency_hz)) return false; // Entry already exists

    // Add the entry
    entries_[active_entries_] = entry;
    active_entries_++;

    return true;
}

// Update existing entry by index
bool DroneFrequencyDatabase::update_entry(size_t index, const DroneFrequencyEntry& entry) {
    // Validation
    if (!validate_entry(entry)) return false;
    if (index >= active_entries_) return false; // Index out of bounds

    // Special check: Don't allow duplicate frequencies unless updating the same entry
    size_t existing_index = find_entry_index(entry.frequency_hz);
    if (existing_index != SIZE_MAX && existing_index != index) return false; // Frequency already used elsewhere

    // Update the entry
    entries_[index] = entry;

    return true;
}

// Remove entry by index
bool DroneFrequencyDatabase::remove_entry(size_t index) {
    if (index >= active_entries_) return false; // Index out of bounds

    // Shift all entries after this index one position left
    for (size_t i = index; i < active_entries_ - 1; ++i) {
        entries_[i] = entries_[i + 1];
    }

    active_entries_--;
    return true;
}

// Remove entry by frequency
bool DroneFrequencyDatabase::remove_entry_by_frequency(uint32_t frequency_hz) {
    size_t index = find_entry_index(frequency_hz);
    if (index == SIZE_MAX) return false; // Entry not found

    return remove_entry(index);
}

// Clear entire database
void DroneFrequencyDatabase::clear_database() {
    active_entries_ = 0;
}

// Reset to default embedded database
void DroneFrequencyDatabase::reset_to_defaults() {
    active_entries_ = 0;
    initialize_database(); // Reload from constants
}

// Compact entries - remove gaps after deletion (if needed in future)
void DroneFrequencyDatabase::compact_entries() {
    // Current implementation keeps entries contiguous, so no need to compact
}

// Legacy methods for backward compatibility

// Simple add_entry method (backward compatible)
void DroneFrequencyDatabase::add_entry(const DroneFrequencyEntry& entry) {
    // This was the old signature - now it returns bool to indicate success
    // For backward compatibility, we'll just call the new method and ignore return value
    add_entry(entry);
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
