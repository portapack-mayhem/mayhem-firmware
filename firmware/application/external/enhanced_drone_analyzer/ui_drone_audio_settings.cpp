// ui_drone_audio_settings.cpp
// Audio settings UI implementation for Enhanced Drone Analyzer

#include "ui_drone_audio_settings.hpp"
#include "ui_drone_audio_settings_about.hpp"
#include "ui_drone_audio.hpp"  // DECONFLICT: Added missing include
#include "ui/ui_text_entry.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

// AudioManager implementation (consolidated audio system)
AudioManager::AudioManager()
    : audio_enabled_(true), test_threat_level_(ThreatLevel::MEDIUM), last_beep_time_(0)
{
    initialize_audio_device();
}

AudioManager::~AudioManager() {
    cleanup_audio_device();
}

void AudioManager::initialize_audio_device() {
    // Audio device initialization - ensure audio output is ready
}

void AudioManager::cleanup_audio_device() {
    audio::output::stop();
}

void AudioManager::play_detection_beep(ThreatLevel level) {
    if (!audio_enabled_) return;

    // Bounds checking (critical safety fix)
    if (level < ThreatLevel::NONE || level > ThreatLevel::CRITICAL) {
        level = test_threat_level_;  // Fallback to test level
    }

    uint16_t frequency = get_beep_frequency(level);

    // Setup audio like in Looking Glass and Recon
    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();
    chThdSleepMilliseconds(10);

    baseband::request_audio_beep(frequency, 24000, 200);

    last_beep_time_ = chTimeNow();
    chThdSleepMilliseconds(250);
    audio::output::stop();
}

void AudioManager::play_sos_signal() {
    if (!audio_enabled_) return;

    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();
    chThdSleepMilliseconds(10);

    const uint16_t SOS_FREQ = 1500;
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

void AudioManager::stop_audio() {
    audio::output::stop();
}

uint16_t AudioManager::get_beep_frequency(ThreatLevel level) const {
    switch (level) {
        case ThreatLevel::LOW: return 1000;
        case ThreatLevel::MEDIUM: return 1200;
        case ThreatLevel::HIGH: return 1600;
        case ThreatLevel::CRITICAL: return 2000;
        case ThreatLevel::NONE:
        default: return 800;
    }
}

void AudioManager::request_audio_beep(uint16_t frequency, uint32_t duration_ms) {
    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();
    chThdSleepMilliseconds(10);

    baseband::request_audio_beep(frequency, 24000, duration_ms);

    chThdSleepMilliseconds(duration_ms + 50);
    audio::output::stop();
}

DroneAudioSettingsView::DroneAudioSettingsView(
    NavigationView& nav,
    DroneAudioSettings& settings,
    DroneAudioController& audio_controller)   // FIX: Updated parameter type
    : nav_(nav), settings_(settings), audio_controller_(audio_controller) {  // FIX: Updated member name

    // Add UI components
    add_children({
        &text_description_,
        &checkbox_audio_enabled_,
        &field_test_threat_level_,
        &text_test_instructions_,
        &button_test_audio_,
        &button_about_  // Add about button
    });

    // Initialize checkboxes and fields
    checkbox_audio_enabled_.set_value(settings.audio_enabled);

    // Set up event handlers
    checkbox_audio_enabled_.on_select = [this]() {
        on_audio_enabled_changed();
    };

    button_test_audio_.on_select = [this]() {
        on_test_audio();
    };

    button_about_.on_select = [this]() {
        nav_.push<AuthorContactView>();
    };

    field_test_threat_level_.set_value(static_cast<int>(settings.test_threat_level));

    // Initialize with current settings
    update_ui_from_settings();
}

void DroneAudioSettingsView::focus() {
    checkbox_audio_enabled_.focus();
}

void DroneAudioSettingsView::on_audio_enabled_changed() {
    settings_.audio_enabled = checkbox_audio_enabled_.value();

    if (on_settings_changed_) {
        on_settings_changed_();
    }
}

void DroneAudioSettingsView::on_test_audio() {
    // Test audio with selected threat level
    ThreatLevel test_level = static_cast<ThreatLevel>(field_test_threat_level_.get_value());

    // VALIDATE: Add bounds checking for threat level (CRITICAL FIX)
    if (test_level < ThreatLevel::NONE || test_level > ThreatLevel::CRITICAL) {
        test_level = ThreatLevel::MEDIUM;  // Safe default
    }

    if (settings_.audio_enabled) {
        // FIX: Call correct method name (CRITICAL FIX)
        audio_controller_.play_detection_beep(test_level);
    }
}

void DroneAudioSettingsView::on_test_threat_level_changed() {
    // Update test threat level
    settings_.test_threat_level = static_cast<ThreatLevel>(field_test_threat_level_.get_value());
}

void DroneAudioSettingsView::update_ui_from_settings() {
    // Update UI to reflect current settings
    checkbox_audio_enabled_.set_value(settings_.audio_enabled);
    field_test_threat_level_.set_value(static_cast<int>(settings_.test_threat_level));
}

} // namespace ui::external_app::enhanced_drone_analyzer
