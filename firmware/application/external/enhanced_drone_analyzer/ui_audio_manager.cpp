// ui_audio_manager.cpp - Audio implementation with Mayhem baseband API
// Phase 3: Safe audio alerts without hardware conflicts

#include "ui_audio_manager.hpp"
#include "baseband_api.hpp"  // Mayhem baseband API for beeper
#include <cstring>

namespace ui::external_app::enhanced_drone_analyzer {

// Global instance for static allocation safety
static AudioSettingsManager global_audio_instance;
AudioSettingsManager* global_audio_manager = &global_audio_instance;

// Default beep configurations (safe frequency ranges)
static constexpr uint16_t DEFAULT_FREQUENCIES[] = {
    800,  // NONE - silent/low
    1000, // LOW - low tone
    1200, // MEDIUM - medium tone
    1600, // HIGH - high tone
    2000  // CRITICAL - very high tone
};

static constexpr uint16_t DEFAULT_DURATIONS[] = {
    100,  // NONE - very short
    200,  // LOW - short
    300,  // MEDIUM - medium
    500,  // HIGH - long
    800   // CRITICAL - very long
};

AudioSettingsManager::AudioSettingsManager() : beep_enabled_(true) {
    // Initialize with safe defaults
    memcpy(beep_frequencies_.data(), DEFAULT_FREQUENCIES, sizeof(DEFAULT_FREQUENCIES));
    memcpy(beep_durations_.data(), DEFAULT_DURATIONS, sizeof(DEFAULT_DURATIONS));
}

size_t AudioSettingsManager::threat_to_index(ThreatLevel level) const {
    switch (level) {
        case ThreatLevel::NONE: return 0;
        case ThreatLevel::LOW: return 1;
        case ThreatLevel::MEDIUM: return 2;
        case ThreatLevel::HIGH: return 3;
        case ThreatLevel::CRITICAL: return 4;
        default: return 0; // Safe fallback
    }
}

void AudioSettingsManager::play_threat_beep(ThreatLevel level) {
    if (!beep_enabled_) return;

    size_t index = threat_to_index(level);
    if (index >= THREAT_LEVEL_COUNT) return;

    uint16_t freq = beep_frequencies_[index];
    uint16_t duration = beep_durations_[index];

    hardware_beep(freq, duration);
}

void AudioSettingsManager::set_tone(ThreatLevel level, uint16_t frequency_hz, uint16_t duration_ms) {
    // Safety checks for embedded audio hardware
    if (frequency_hz < 200 || frequency_hz > 5000) return;  // Realistic PWM range
    if (duration_ms < 50 || duration_ms > 2000) return;     // Reasonable duration

    size_t index = threat_to_index(level);
    if (index >= THREAT_LEVEL_COUNT) return;

    beep_frequencies_[index] = frequency_hz;
    beep_durations_[index] = duration_ms;
}

uint16_t AudioSettingsManager::get_frequency(ThreatLevel level) const {
    size_t index = threat_to_index(level);
    if (index >= THREAT_LEVEL_COUNT) return DEFAULT_FREQUENCIES[0];
    return beep_frequencies_[index];
}

uint16_t AudioSettingsManager::get_duration(ThreatLevel level) const {
    size_t index = threat_to_index(level);
    if (index >= THREAT_LEVEL_COUNT) return DEFAULT_DURATIONS[0];
    return beep_durations_[index];
}

void AudioSettingsManager::play_sos_signal() {
    if (!beep_enabled_) return;

    // SOS pattern: ...---... (3 short, 3 long, 3 short)
    hardware_beep_sos();
}

bool AudioSettingsManager::is_pwm_available() const {
    // TODO: Check if PWM hardware is initialized
    // This is a placeholder - would need to check baseband state
    return true; // Assume available for now
}

// Hardware-dependent implementations (Mayhem baseband compatible)
bool AudioSettingsManager::hardware_beep(uint16_t frequency_hz, uint16_t duration_ms) {
    // Use Mayhem baseband API for beeper
    // This requires proper initialization of baseband subsystem

    if (!is_pwm_available()) return false;

    // Convert to baseband API parameters
    // Note: This is a simplified wrapper - real implementation needs
    // proper baseband::beeper_* function calls from Mayhem firmware

    // Placeholder for now - would be:
    // return baseband::beeper::beep(frequency_hz, duration_ms);

    return true; // Success placeholder
}

void AudioSettingsManager::hardware_beep_sos() {
    // SOS implementation using baseband beeper
    // Pattern: 3 short (200ms) + 3 long (600ms) + 3 short (200ms)

    static constexpr uint16_t SHORT_FREQ = 1500;
    static constexpr uint16_t LONG_FREQ = 1500;
    static constexpr uint16_t SHORT_MS = 200;
    static constexpr uint16_t LONG_MS = 600;
    static constexpr uint16_t PAUSE_MS = 150;

    if (!beep_enabled_) return;

    // Three shorts
    for (int i = 0; i < 3; ++i) {
        hardware_beep(SHORT_FREQ, SHORT_MS);
        chThdSleepMilliseconds(PAUSE_MS);
    }

    // Pause between letter groups
    chThdSleepMilliseconds(PAUSE_MS * 2);

    // Three longs
    for (int i = 0; i < 3; ++i) {
        hardware_beep(LONG_FREQ, LONG_MS);
        chThdSleepMilliseconds(PAUSE_MS);
    }

    // Pause between letter groups
    chThdSleepMilliseconds(PAUSE_MS * 2);

    // Three shorts
    for (int i = 0; i < 3; ++i) {
        hardware_beep(SHORT_FREQ, SHORT_MS);
        chThdSleepMilliseconds(PAUSE_MS);
    }
}

} // namespace ui::external_app::enhanced_drone_analyzer
