// ui_drone_database.cpp
// Implementation of CRUD operations for drone frequency database

#include "ui_drone_database.hpp"
#include <algorithm>

namespace ui::external_app::enhanced_drone_analyzer {

DroneFrequencyDatabase::DroneFrequencyDatabase()
    : active_entries_(0) {
    initialize_database();
}

void DroneFrequencyDatabase::initialize_database() {
    // Phase 1: Load known drone frequencies (pattern from Recon freqman_load_options)
    // For now, use compile-time defaults - could be extended to load from file
    reset_to_defaults();
}

void DroneFrequencyDatabase::reset_to_defaults() {
    clear_database();
    // Load from ENHANCED_DRONE_FREQUENCY_DATABASE
    size_t entries_to_load = std::min(
        static_cast<size_t>(sizeof(ENHANCED_DRONE_FREQUENCY_DATABASE) / sizeof(DroneFrequencyEntry)),
        MAX_ENTRIES);

    for (size_t i = 0; i < entries_to_load; ++i) {
        add_entry(ENHANCED_DRONE_FREQUENCY_DATABASE[i]);
    }
}

bool DroneFrequencyDatabase::add_entry(const DroneFrequencyEntry& entry) {
    if (active_entries_ >= MAX_ENTRIES) return false;

    if (!validate_entry(entry)) return false;

    // Check for duplicates
    if (entry_exists(entry.frequency_hz)) return false;

    entries_[active_entries_] = entry;
    active_entries_++;
    return true;
}

bool DroneFrequencyDatabase::update_entry(size_t index, const DroneFrequencyEntry& entry) {
    if (index >= active_entries_) return false;

    if (!validate_entry(entry)) return false;

    entries_[index] = entry;
    return true;
}

bool DroneFrequencyDatabase::remove_entry(size_t index) {
    if (index >= active_entries_) return false;

    // Shift remaining entries left (similar to std::remove_if)
    for (size_t i = index; i < active_entries_ - 1; ++i) {
        entries_[i] = entries_[i + 1];
    }

    active_entries_--;
    return true;
}

bool DroneFrequencyDatabase::remove_entry_by_frequency(uint32_t frequency_hz) {
    for (size_t i = 0; i < active_entries_; ++i) {
        if (entries_[i].frequency_hz == frequency_hz) {
            return remove_entry(i);
        }
    }
    return false;
}

void DroneFrequencyDatabase::clear_database() {
    active_entries_ = 0;
    // Reset all entries to zero state
    std::fill(entries_.begin(), entries_.end(), DroneFrequencyEntry{});
}

const DroneFrequencyEntry* DroneFrequencyDatabase::get_entry(size_t index) const {
    if (index >= active_entries_) return nullptr;
    return &entries_[index];
}

const DroneFrequencyEntry* DroneFrequencyDatabase::lookup_frequency(uint32_t frequency_hz) const {
    for (size_t i = 0; i < active_entries_; ++i) {
        if (entries_[i].frequency_hz == frequency_hz) {
            return &entries_[i];
        }
    }
    return nullptr;
}

ThreatLevel DroneFrequencyDatabase::get_threat_level(uint32_t frequency_hz) const {
    const auto* entry = lookup_frequency(frequency_hz);
    return entry ? entry->threat_level : ThreatLevel::NONE;
}

bool DroneFrequencyDatabase::validate_entry(const DroneFrequencyEntry& entry) const {
    return entry.is_valid() &&
           entry.frequency_hz >= MIN_DRONE_FREQUENCY &&
           entry.frequency_hz <= MAX_DRONE_FREQUENCY;
}

bool DroneFrequencyDatabase::entry_exists(uint32_t frequency_hz) const {
    return lookup_frequency(frequency_hz) != nullptr;
}

size_t DroneFrequencyDatabase::find_entry_index(uint32_t frequency_hz) const {
    for (size_t i = 0; i < active_entries_; ++i) {
        if (entries_[i].frequency_hz == frequency_hz) {
            return i;
        }
    }
    return static_cast<size_t>(-1);
}

} // namespace ui::external_app::enhanced_drone_analyzer
