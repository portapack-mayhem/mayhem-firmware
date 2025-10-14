// ui_drone_audio.cpp
// Implementation of audio feedback system for drone detection

#include "ui_drone_audio.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

DroneAudioAlert::DroneAudioAlert()
    : audio_enabled_(true)
    , last_beep_time_(0) {
}

DroneAudioAlert::~DroneAudioAlert() {
    stop_audio();
}

void DroneAudioAlert::enable_audio(bool enable) {
    audio_enabled_ = enable;
    if (!enable) {
        stop_audio();
    }
}

void DroneAudioAlert::play_detection_beep(ThreatLevel level) {
    if (!audio_enabled_) return;

    uint16_t frequency = get_beep_frequency(level);

    // Setup audio rate and start output
    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();
    chThdSleepMilliseconds(10);  // Brief stabilization delay

    // Direct baseband API call for beep
    baseband::request_audio_beep(frequency, 24000, 200);

    // Cleanup like other Mayhem apps
    chThdSleepMilliseconds(250);
    audio::output::stop();
}

void DroneAudioAlert::play_sos_signal() {
    if (!audio_enabled_) return;

    // Setup SOS implementation using baseband::request_audio_beep
    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();
    chThdSleepMilliseconds(10);

    // SOS signal for critical threats (...---...)
    const uint16_t SOS_FREQ = 1500;

    // Three short beeps
    for (int i = 0; i < 3; ++i) {
        baseband::request_audio_beep(SOS_FREQ, 24000, 200);
        chThdSleepMilliseconds(150);
    }

    chThdSleepMilliseconds(300);

    // Three long beeps
    for (int i = 0; i < 3; ++i) {
        baseband::request_audio_beep(SOS_FREQ, 24000, 600);
        chThdSleepMilliseconds(150);
    }

    chThdSleepMilliseconds(300);

    // Three short beeps
    for (int i = 0; i < 3; ++i) {
        baseband::request_audio_beep(SOS_FREQ, 24000, 200);
        chThdSleepMilliseconds(150);
    }

    // Cleanup
    chThdSleepMilliseconds(250);
    audio::output::stop();
}

void DroneAudioAlert::stop_audio() {
    audio::output::stop();
    baseband::request_beep_stop();
}

uint16_t DroneAudioAlert::get_beep_frequency(ThreatLevel level) {
    switch (level) {
        case ThreatLevel::CRITICAL: return 2000;
        case ThreatLevel::HIGH: return 1600;
        case ThreatLevel::MEDIUM: return 1200;
        case ThreatLevel::LOW: return 1000;
        case ThreatLevel::NONE:
        default: return 800;
    }
}

void AudioManager::request_beep(uint16_t frequency, uint32_t sample_rate, uint32_t duration_ms) {
    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();
    chThdSleepMilliseconds(5);

    baseband::request_audio_beep(frequency, sample_rate, duration_ms);

    chThdSleepMilliseconds(duration_ms + 50);
    audio::output::stop();
}

void AudioManager::stop_all_audio() {
    audio::output::stop();
    baseband::request_beep_stop();
}

} // namespace ui::external_app::enhanced_drone_analyzer
