// ui_frequency_manager.hpp - Frequency management for enhanced drone analyzer
// Phase 2: Safe frequency customization without hardware conflicts

#ifndef __UI_FREQUENCY_MANAGER_HPP__
#define __UI_FREQUENCY_MANAGER_HPP__

#include <array>
#include <cstdint>
#include <string>
#include <vector>

// Forward declaration for embedded compatibility
enum class DroneType;
enum class ThreatLevel;

namespace ui::external_app::enhanced_drone_analyzer {

// Simplified frequency entry for embedded constraints
struct CustomFrequencyEntry {
    uint32_t frequency_hz;
    DroneType drone_type;
    ThreatLevel threat_level;
    int16_t rssi_threshold;  // dB
    std::array<char, 32> name; // Fixed-size string for embedded
    uint32_t bandwidth_hz;
    int32_t center_offset_hz;
    bool enabled;

    CustomFrequencyEntry() : frequency_hz(0), drone_type(DroneType::UNKNOWN),
                           threat_level(ThreatLevel::LOW), rssi_threshold(-80),
                           bandwidth_hz(6000000), center_offset_hz(0), enabled(true) {
        name.fill('\0');
    }

    // Safe string copying
    void set_name(const char* new_name) {
        if (!new_name) return;
        size_t len = 0;
        while (new_name[len] && len < name.size() - 1) {
            name[len] = new_name[len];
            ++len;
        }
        name[len] = '\0'; // Null terminate
    }

    const char* get_name() const { return name.data(); }
};

// Embedded-safe frequency manager - no filesystem operations yet
class FrequencyManager {
public:
    // Embedded constraints
    static constexpr size_t MAX_FREQUENCIES = 16;  // Reduced for RAM safety

    FrequencyManager();

    // Basic operations
    bool add_frequency(const CustomFrequencyEntry& entry);
    bool remove_frequency(size_t index);
    bool toggle_frequency(size_t index);

    // Accessors
    const CustomFrequencyEntry* get_frequency(size_t index) const;
    size_t get_frequency_count() const { return active_frequencies_; }
    size_t get_max_frequencies() const { return MAX_FREQUENCIES; }

    // Simple validation for embedded systems
    bool is_valid_frequency(const CustomFrequencyEntry& entry) const;
    bool has_duplicates(uint32_t frequency_hz) const;

    // Status
    bool is_full() const { return active_frequencies_ >= MAX_FREQUENCIES; }

    // Utility for demo/demo mode
    void add_default_frequencies();

private:
    std::array<CustomFrequencyEntry, MAX_FREQUENCIES> frequencies_;
    size_t active_frequencies_;

    // Helper to find next free slot
    int find_free_slot() const;
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_FREQUENCY_MANAGER_HPP__
