// ui_drone_audio.cpp
// Audio alerts implementation for Enhanced Drone Analyzer

#include "ui_drone_audio.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

DroneAudioController::DroneAudioController()
    : audio_enabled_(true),     // Audio enabled by default
      last_beep_time_(0)
{
    initialize_audio_device();
}

DroneAudioController::~DroneAudioController() {
    cleanup_audio_device();
}

void DroneAudioController::initialize_audio_device() {
    // Audio device initialization - ensure audio output is ready
    // This is typically handled by the portapack audio system
}

void DroneAudioController::cleanup_audio_device() {
    // Cleanup audio resources
    audio::output::stop();
}

void DroneAudioController::play_detection_beep(ThreatLevel level) {
    if (!audio_enabled_) return;

    uint16_t frequency = get_beep_frequency(level);

    // Setup audio like in Looking Glass and Recon
    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();
    chThdSleepMilliseconds(10);

    // Request audio beep through baseband API (direct call pattern)
    baseband::request_audio_beep(frequency, 24000, 200);

    last_beep_time_ = chTimeNow();

    // Cleanup like other Mayhem apps
    chThdSleepMilliseconds(250);
    audio::output::stop();
}

void DroneAudioController::play_sos_signal() {
    if (!audio_enabled_) return;

    // Setup audio
    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();
    chThdSleepMilliseconds(10);

    const uint16_t SOS_FREQ = 1500;
    // SOS pattern: ...---...
    for (int i = 0; i < 3; ++i) {
        baseband::request_audio_beep(SOS_FREQ, 24000, 200);
        chThdSleepMilliseconds(150);
    }
    chThdSleepMilliseconds(300);

    for (int i = 0; i < 3; ++i) {
        baseband::request_audio_beep(SOS_FREQ, 24000, 600);
        chThdSleepMilliseconds(150);
    }
    chThdSleepMilliseconds(300);

    for (int i = 0; i < 3; ++i) {
        baseband::request_audio_beep(SOS_FREQ, 24000, 200);
        chThdSleepMilliseconds(150);
    }

    last_beep_time_ = chTimeNow();

    chThdSleepMilliseconds(250);
    audio::output::stop();
}

void DroneAudioController::stop_audio() {
    audio::output::stop();
}

uint16_t DroneAudioController::get_beep_frequency(ThreatLevel level) const {
    // V0 style frequencies like in main implementation
    switch (level) {
        case ThreatLevel::LOW: return 1000;
        case ThreatLevel::MEDIUM: return 1200;
        case ThreatLevel::HIGH: return 1600;
        case ThreatLevel::CRITICAL: return 2000;
        case ThreatLevel::NONE:
        default: return 800; // Very low tone for none
    }
}

void DroneAudioController::request_audio_beep(uint16_t frequency, uint32_t duration_ms) {
    // Static method for external access
    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();
    chThdSleepMilliseconds(10);

    baseband::request_audio_beep(frequency, 24000, duration_ms);

    chThdSleepMilliseconds(duration_ms + 50);
    audio::output::stop();
}

} // namespace ui::external_app::enhanced_drone_analyzer
