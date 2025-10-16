// ui_drone_database.hpp
// Drone-specific database management for Enhanced Drone Analyzer
// Stores drone type, threat level, and RSSI threshold per frequency

#ifndef __UI_DRONE_DATABASE_HPP__
#define __UI_DRONE_DATABASE_HPP__

#include "freqman_db.hpp"        // Base FreqmanDB functionality
#include "ui_drone_types.hpp"    // DroneType, ThreatLevel
#include "log_file.hpp"          // LogFile for persistence

#include <vector>
#include <array>
#include <string>

namespace ui::external_app::enhanced_drone_analyzer {

// Forward declare for circular dependency avoidance
struct DroneFrequencyEntry;

// Drone Database Manager - Extends FreqmanDB with drone-specific data
class DroneFrequencyDatabase {
public:
    DroneFrequencyDatabase();
    ~DroneFrequencyDatabase() = default;

    // Database lifecycle
    bool open(const std::filesystem::path& path, bool create = false);
    bool is_open() const { return freq_db_.is_open(); }
    void close();
    bool save();
    void clear();

    // Entry count access
    size_t entry_count() const;

    // Get frequency entry with drone metadata
    bool get_entry_drone(size_t index, DroneFrequencyEntry& entry) const;

    // Add/update drone frequency entry
    bool add_drone_frequency(const DroneFrequencyEntry& entry, bool override_existing = false);

    // Remove entry by frequency
    bool remove_frequency(rf::Frequency frequency);

    // Lookup drone info by frequency
    const DroneFrequencyEntry* lookup_frequency(rf::Frequency frequency) const;

    // Get all frequencies as basic freqman entries (for scanner compatibility)
    std::vector<freqman_entry> get_freqman_entries() const;

private:
    FreqmanDB freq_db_;  // Core frequency database
    std::filesystem::path db_path_;  // Path to associated file

    // Drone metadata storage (in-memory cache for performance)
    std::vector<DroneFrequencyEntry> drone_entries_;

    // Synchronization helpers
    void sync_from_freqman_db();
    void sync_to_freqman_db();

    // Encode/decode drone data in freqman_entry.description
    // Format: "DRONE|<type>|<threat>|<rssi_thresh>|<name>"
    std::string encode_drone_data(const DroneFrequencyEntry& entry) const;
    bool decode_drone_data(const std::string& description, DroneFrequencyEntry& entry) const;

    // Prevent copying (embedded constraints)
    DroneFrequencyDatabase(const DroneFrequencyDatabase&) = delete;
    DroneFrequencyDatabase& operator=(const DroneFrequencyDatabase&) = delete;
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_DATABASE_HPP__
