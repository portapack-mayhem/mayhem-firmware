// ui_drone_database.cpp
// Implementation of drone-specific database management

#include "ui_drone_database.hpp"
#include "string_format.hpp"
#include <algorithm>

namespace ui::external_app::enhanced_drone_analyzer {

DroneFrequencyDatabase::DroneFrequencyDatabase()
    : db_path_({}) {
    // Constructor - path set during open()
}

bool DroneFrequencyDatabase::open(const std::filesystem::path& path, bool create) {
    if (freq_db_.open(path, create)) {
        db_path_ = path;
        sync_from_freqman_db();  // Load existing data and parse drone metadata
        return true;
    }
    return false;
}

void DroneFrequencyDatabase::close() {
    if (freq_db_.is_open()) {
        save();  // Ensure changes are persisted
        freq_db_.close();
        db_path_.clear();
        drone_entries_.clear();
    }
}

bool DroneFrequencyDatabase::save() {
    if (!freq_db_.is_open()) {
        return false;
    }

    sync_to_freqman_db();  // Update FreqmanDB with encoded drone data
    return freq_db_.save();
}

void DroneFrequencyDatabase::clear() {
    freq_db_.clear();
    drone_entries_.clear();
}

size_t DroneFrequencyDatabase::entry_count() const {
    return drone_entries_.size();
}

bool DroneFrequencyDatabase::get_entry_drone(size_t index, DroneFrequencyEntry& entry) const {
    if (index >= drone_entries_.size()) {
        return false;
    }

    entry = drone_entries_[index];
    return true;
}

bool DroneFrequencyDatabase::add_drone_frequency(const DroneFrequencyEntry& entry, bool override_existing) {
    // Check for existing frequency
    auto it = std::find_if(drone_entries_.begin(), drone_entries_.end(),
                          [entry](const DroneFrequencyEntry& e) {
                              return e.frequency_hz == entry.frequency_hz;
                          });

    if (it != drone_entries_.end()) {
        if (!override_existing) {
            return false;  // Frequency already exists
        }
        *it = entry;  // Update existing
    } else {
        drone_entries_.push_back(entry);  // Add new
    }

    return true;
}

bool DroneFrequencyDatabase::remove_frequency(rf::Frequency frequency) {
    auto it = std::remove_if(drone_entries_.begin(), drone_entries_.end(),
                            [frequency](const DroneFrequencyEntry& e) {
                                return e.frequency_hz == frequency;
                            });

    if (it != drone_entries_.end()) {
        drone_entries_.erase(it, drone_entries_.end());
        return true;
    }

    return false;
}

const DroneFrequencyEntry* DroneFrequencyDatabase::lookup_frequency(rf::Frequency frequency) const {
    auto it = std::find_if(drone_entries_.begin(), drone_entries_.end(),
                          [frequency](const DroneFrequencyEntry& e) {
                              return e.frequency_hz == frequency;
                          });

    if (it != drone_entries_.end()) {
        return &(*it);
    }

    return nullptr;
}

std::vector<freqman_entry> DroneFrequencyDatabase::get_freqman_entries() const {
    std::vector<freqman_entry> entries;

    for (const auto& drone_entry : drone_entries_) {
        freqman_entry entry{
            .frequency_a = static_cast<rf::Frequency>(drone_entry.frequency_hz),
            .frequency_b = static_cast<rf::Frequency>(drone_entry.frequency_hz),
            .type = freqman_type::Single,
            .modulation = freqman_index_t(1),  // NFM default for drones
            .bandwidth = freqman_index_t(3),   // 25kHz default
            .step = freqman_index_t(5),        // 25kHz step
            .description = encode_drone_data(drone_entry),
            .tonal = ""  // Not used for drones
        };
        entries.push_back(entry);
    }

    return entries;
}

void DroneFrequencyDatabase::sync_from_freqman_db() {
    drone_entries_.clear();

    if (!freq_db_.is_open()) {
        return;
    }

    // Load and decode all entries from FreqmanDB
    for (size_t i = 0; i < freq_db_.entry_count(); ++i) {
        auto entry_opt = freq_db_.get_entry(i);
        if (entry_opt) {
            DroneFrequencyEntry drone_entry;
            if (decode_drone_data(entry_opt->description, drone_entry)) {
                // Successfully decoded drone data
                drone_entry.frequency_hz = entry_opt->frequency_a;
                drone_entries_.push_back(drone_entry);
            }
        }
    }
}

void DroneFrequencyDatabase::sync_to_freqman_db() {
    if (!freq_db_.is_open()) {
        return;
    }

    freq_db_.clear();  // Clear existing entries

    // Write all drone entries to FreqmanDB
    auto freqman_entries = get_freqman_entries();
    for (const auto& entry : freqman_entries) {
        freq_db_.append_entry(entry);
    }
}

std::string DroneFrequencyDatabase::encode_drone_data(const DroneFrequencyEntry& entry) const {
    char buffer[128];
    memset(buffer, 0, sizeof(buffer));

    // Format: "DRONE|<type_idx>|<threat_idx>|<rssi_thresh>|<name>"
    int ret = snprintf(buffer, sizeof(buffer) - 1,
                      "DRONE|%u|%u|%d|%s",
                      static_cast<uint8_t>(entry.drone_type),
                      static_cast<uint8_t>(entry.threat_level),
                      entry.rssi_threshold_db,
                      entry.name.c_str());

    if (ret < 0 || ret >= static_cast<int>(sizeof(buffer))) {
        return "DRONE|0|0|-90|UNKNOWN";  // Safe fallback
    }

    return std::string(buffer);
}

bool DroneFrequencyDatabase::decode_drone_data(const std::string& description, DroneFrequencyEntry& entry) const {
    // Check if description starts with "DRONE|"
    if (description.substr(0, 6) != "DRONE|") {
        return false;
    }

    // Parse format: "DRONE|<type_idx>|<threat_idx>|<rssi_thresh>|<name>"
    char type_idx_str[4], threat_idx_str[4], rssi_str[8], name_str[64];
    memset(type_idx_str, 0, sizeof(type_idx_str));
    memset(threat_idx_str, 0, sizeof(threat_idx_str));
    memset(rssi_str, 0, sizeof(rssi_str));
    memset(name_str, 0, sizeof(name_str));

    int ret = sscanf(description.c_str(), "DRONE|%3[^|]|%3[^|]|%7[^|]|%63[^|]",
                     type_idx_str, threat_idx_str, rssi_str, name_str);

    if (ret != 4) {
        return false;  // Failed to parse all required fields
    }

    // Convert and validate
    uint8_t type_idx = static_cast<uint8_t>(atoi(type_idx_str));
    uint8_t threat_idx = static_cast<uint8_t>(atoi(threat_idx_str));
    int8_t rssi_threshold = static_cast<int8_t>(atoi(rssi_str));

    if (type_idx >= static_cast<uint8_t>(DroneType::MAX_TYPES) ||
        threat_idx > static_cast<uint8_t>(ThreatLevel::CRITICAL)) {
        return false;
    }

    // Fill entry
    entry.drone_type = static_cast<DroneType>(type_idx);
    entry.threat_level = static_cast<ThreatLevel>(threat_idx);
    entry.rssi_threshold_db = rssi_threshold;
    entry.name = std::string(name_str);

    return true;
}

} // namespace ui::external_app::enhanced_drone_analyzer
