// ui_frequency_manager.cpp - Implementation for frequency management
// Phase 2: Safe implementation with no filesystem dependencies

#include "ui_frequency_manager.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

// FrequencyManager implementation
FrequencyManager::FrequencyManager() : active_frequencies_(0) {
    frequencies_.fill(CustomFrequencyEntry());  // Initialize with defaults
}

int FrequencyManager::find_free_slot() const {
    for (size_t i = 0; i < MAX_FREQUENCIES; ++i) {
        if (frequencies_[i].frequency_hz == 0) {
            return static_cast<int>(i);
        }
    }
    return -1; // No free slot
}

bool FrequencyManager::add_frequency(const CustomFrequencyEntry& entry) {
    if (active_frequencies_ >= MAX_FREQUENCIES) return false;
    if (!is_valid_frequency(entry)) return false;
    if (has_duplicates(entry.frequency_hz)) return false;

    int slot = find_free_slot();
    if (slot < 0) return false;

    frequencies_[slot] = entry;
    active_frequencies_++;
    return true;
}

bool FrequencyManager::remove_frequency(size_t index) {
    if (index >= MAX_FREQUENCIES || frequencies_[index].frequency_hz == 0) {
        return false;
    }

    frequencies_[index] = CustomFrequencyEntry();  // Reset to default (empty)
    active_frequencies_--;
    return true;
}

bool FrequencyManager::toggle_frequency(size_t index) {
    if (index >= MAX_FREQUENCIES || frequencies_[index].frequency_hz == 0) {
        return false;
    }

    frequencies_[index].enabled = !frequencies_[index].enabled;
    return true;
}

const CustomFrequencyEntry* FrequencyManager::get_frequency(size_t index) const {
    if (index >= MAX_FREQUENCIES || frequencies_[index].frequency_hz == 0) {
        return nullptr;
    }

    return &frequencies_[index];
}

bool FrequencyManager::is_valid_frequency(const CustomFrequencyEntry& entry) const {
    // Basic validation for embedded safety
    if (entry.frequency_hz < 100000000U || entry.frequency_hz > 6000000000U) {
        return false;  // Must be in UAV frequency range (100MHz - 6GHz)
    }

    if (entry.rssi_threshold < -120 || entry.rssi_threshold > -20) {
        return false;  // Realistic RSSI range
    }

    if (entry.bandwidth_hz == 0 || entry.bandwidth_hz > 50000000U) {
        return false;  // Reasonable bandwidth
    }

    return entry.frequency_hz > 0;  // Basic non-zero check
}

bool FrequencyManager::has_duplicates(uint32_t frequency_hz) const {
    if (frequency_hz == 0) return false;  // Zero frequency is allowed (empty slot)

    for (const auto& freq : frequencies_) {
        if (freq.frequency_hz == frequency_hz) {
            return true;
        }
    }
    return false;
}

void FrequencyManager::add_default_frequencies() {
    // Add some safe defaults for testing
    CustomFrequencyEntry defaults[] = {
        {920000000U, DroneType::DJI_MAVIC, ThreatLevel::HIGH, -80, {}, 6000000, 0, true},
        {2420000000U, DroneType::DJI_PHANTOM, ThreatLevel::MEDIUM, -85, {}, 6000000, 0, true},
        {5658000000U, DroneType::FPV_RACING, ThreatLevel::MEDIUM, -90, {}, 6000000, 0, true}
    };

    for (auto& entry : defaults) {
        constexpr char name_dji1[] = "DJI-920MHz";
        constexpr char name_dji2[] = "DJI-2.4GHz";
        constexpr char name_fpv[] = "FPV-5.6GHz";

        const char* names[] = {name_dji1, name_dji2, name_fpv};
        static size_t name_idx = 0;

        if (name_idx < 3) {
            entry.set_name(names[name_idx]);
            name_idx++;
        }

        add_frequency(entry);
    }
}

} // namespace ui::external_app::enhanced_drone_analyzer
