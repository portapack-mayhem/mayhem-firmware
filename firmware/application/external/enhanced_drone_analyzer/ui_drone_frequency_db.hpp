// ui_drone_frequency_db.hpp
// Drone frequency database integration for Enhanced Drone Analyzer
// Module: Specialized database lookup following Recon/Freqman patterns

#ifndef __UI_DRONE_FREQUENCY_DB_HPP__
#define __UI_DRONE_FREQUENCY_DB_HPP__

#include "ui_drone_types.hpp"        // DroneFrequencyEntry, ThreatLevel, DroneType
#include "freqman_db.hpp"            // FreqmanDB integration

#include <string>

namespace ui::external_app::enhanced_drone_analyzer {

// FORWARD DECLARATION RESOLUTION: DroneFrequencyDatabase class for scanner
class DroneFrequencyDatabase {
public:
    // CONSTRUCTOR: Using FreqmanDB backend (Recon pattern)
    DroneFrequencyDatabase() = default;
    ~DroneFrequencyDatabase() = default;

    // FREQUENCY LOOKUP: Following Recon database access patterns
    const DroneFrequencyEntry* lookup_frequency(rf::Frequency freq_hz) const {
        // INTEGRATE WITH FREQMAN_DB: Look for matching frequency in loaded database
        // For now, return nullptr - actual implementation should query FreqmanDB
        (void)freq_hz; // Suppress unused parameter warning
        return nullptr; // TODO: Implement FreqmanDB integration
    }

    // DRONE TYPE LOOKUP: Get drone info by frequency (following EDA original intent)
    DroneType get_drone_type(rf::Frequency freq_hz) const {
        const auto* entry = lookup_frequency(freq_hz);
        return entry ? entry->get_drone_type() : DroneType::UNKNOWN;
    }

    // THREAT LEVEL LOOKUP: Critical for security (following threat assessment patterns)
    ThreatLevel get_threat_level(rf::Frequency freq_hz) const {
        const auto* entry = lookup_frequency(freq_hz);
        return entry ? entry->get_threat_level() : ThreatLevel::NONE;
    }

    // RSSI THRESHOLD LOOKUP: Per-frequency sensitivity (Recon pattern)
    int32_t get_rssi_threshold(rf::Frequency freq_hz) const {
        const auto* entry = lookup_frequency(freq_hz);
        return entry ? entry->rssi_threshold_db : -90; // Default threshold
    }

    // DATABASE SIZE: For UI display (following Recon statistics)
    size_t size() const {
        // TODO: Return actual database size from FreqmanDB integration
        return 0;
    }

    // VALIDATION: Check if frequency is in known dangerous range
    bool is_known_dangerous_frequency(rf::Frequency freq_hz) const {
        ThreatLevel threat = get_threat_level(freq_hz);
        return threat >= ThreatLevel::HIGH;
    }

private:
    // BACKEND INTEGRATION PLACEHOLDER
    // TODO: Add FreqmanDB member and integration logic
    // Following Recon's frequency loading and database management patterns

    // Prevent copying (embedded constraints)
    DroneFrequencyDatabase(const DroneFrequencyDatabase&) = delete;
    DroneFrequencyDatabase& operator=(const DroneFrequencyDatabase&) = delete;
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_FREQUENCY_DB_HPP__
