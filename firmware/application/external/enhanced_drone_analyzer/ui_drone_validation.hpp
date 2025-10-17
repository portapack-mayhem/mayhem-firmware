// ui_drone_validation.hpp
// Drone detection validation utilities for Enhanced Drone Analyzer
// Module: Provides validation functions following Level/Recon embedded patterns

#ifndef __UI_DRONE_VALIDATION_HPP__
#define __UI_DRONE_VALIDATION_HPP__

#include "ui_drone_types.hpp"  // ThreatLevel enum
#include <cstdint>

namespace ui::external_app::enhanced_drone_analyzer {

// EMBEDDED VALIDATION CLASS - Following Level/Recon direct patterns
class SimpleDroneValidation {
public:
    // DIRECT EMBEDDED PATTERN: RSSI validation like Level app
    static bool validate_rssi_signal(int32_t rssi_db, ThreatLevel threat) {
        // Step 1: Hardware range validation (like Level app constraints)
        if (rssi_db < -120 || rssi_db > -10) {
            return false;  // Outside Portapack hardware range
        }

        // Step 2: Threat-based threshold validation (following Recon patterns)
        int8_t detection_threshold;
        switch (threat) {
            case ThreatLevel::CRITICAL:
                detection_threshold = -75;  // Very sensitive for critical threats
                break;
            case ThreatLevel::HIGH:
                detection_threshold = -80;  // Sensitive for high threats
                break;
            case ThreatLevel::MEDIUM:
                detection_threshold = -85;  // Moderate sensitivity
                break;
            case ThreatLevel::LOW:
                detection_threshold = -90;  // Standard civilian sensitivity
                break;
            case ThreatLevel::NONE:
            default:
                detection_threshold = -95;  // Very relaxed for none/unknown
                break;
        }

        return rssi_db >= detection_threshold;
    }

    // ADDITIONAL: Signal strength classification (following Scanner patterns)
    static ThreatLevel classify_signal_strength(int32_t rssi_db) {
        if (rssi_db >= -75) return ThreatLevel::CRITICAL;
        if (rssi_db >= -80) return ThreatLevel::HIGH;
        if (rssi_db >= -85) return ThreatLevel::MEDIUM;
        if (rssi_db >= -90) return ThreatLevel::LOW;
        return ThreatLevel::NONE;
    }

    // ADDITIONAL: Frequency range validation (following Recon safety checks)
    static bool validate_frequency_range(rf::Frequency freq_hz) {
        // Portapack hardware limits (following radio.hpp constraints)
        return freq_hz >= 50000000 && freq_hz <= 6000000000UL;
    }

private:
    SimpleDroneValidation() = delete;  // Static utility class
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_VALIDATION_HPP__
