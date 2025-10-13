// ui_audio_manager.hpp - Audio settings manager for threat notifications
// Phase 3: Audio alerts using Mayhem baseband API

#ifndef __UI_AUDIO_MANAGER_HPP__
#define __UI_AUDIO_MANAGER_HPP__

#include <array>
#include <cstdint>

// Forward declarations for embedded compatibility
enum class ThreatLevel;

namespace ui::external_app::enhanced_drone_analyzer {

// Audio settings manager - compatible with Mayhem baseband API
class AudioSettingsManager {
public:
    AudioSettingsManager();

    // Core audio functionality
    bool beep_enabled() const { return beep_enabled_; }
    void set_beep_enabled(bool enabled) { beep_enabled_ = enabled; }

    // Play beep for specific threat level
    void play_threat_beep(ThreatLevel level);

    // Configuration per threat level (embedded-safe)
    void set_tone(ThreatLevel level, uint16_t frequency_hz, uint16_t duration_ms);
    uint16_t get_frequency(ThreatLevel level) const;
    uint16_t get_duration(ThreatLevel level) const;

    // SOS signal for critical threats
    void play_sos_signal();

    // Status and diagnostics
    bool is_pwm_available() const;  // Check if hardware PWM is ready

private:
    bool beep_enabled_;
    // Embedded constraints: Fixed arrays instead of maps
    static constexpr size_t THREAT_LEVEL_COUNT = 5;  // NONE, LOW, MEDIUM, HIGH, CRITICAL

    std::array<uint16_t, THREAT_LEVEL_COUNT> beep_frequencies_;
    std::array<uint16_t, THREAT_LEVEL_COUNT> beep_durations_;

    // Convert enum to array index safely
    size_t threat_to_index(ThreatLevel level) const;

    // Hardware-dependent beep functions (implemented in .cpp with proper API calls)
    bool hardware_beep(uint16_t frequency_hz, uint16_t duration_ms);
    void hardware_beep_sos();  // Interrupt-based SOS pattern
};

// Global instance for safe memory management
extern AudioSettingsManager* global_audio_manager;

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_AUDIO_MANAGER_HPP__
