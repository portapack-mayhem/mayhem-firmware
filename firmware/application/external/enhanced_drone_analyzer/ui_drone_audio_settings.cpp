// ui_drone_audio_settings.cpp
// Implementation of audio settings UI

#include "ui_drone_audio_settings.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

DroneAudioSettingsView::DroneAudioSettingsView(
    NavigationView& nav,
    DroneAudioSettings& settings,
    DroneAudioAlert& audio_alert
) : nav_(nav), settings_(settings), audio_alert_(audio_alert) {
    // Initialize settings
    settings_.audio_enabled = audio_alert_.is_audio_enabled();
    settings_.test_threat_level = ThreatLevel::MEDIUM;

    // Set up UI element initial states
    checkbox_audio_enabled_.set_value(settings_.audio_enabled);
    field_test_threat_level_.set_value(static_cast<int>(settings_.test_threat_level));

    // Set up event handlers
    checkbox_audio_enabled_.on_select = [this]() { on_audio_enabled_changed(); };
    button_test_audio_.on_select = [this]() { on_test_audio(); };

    // Change handler for threat level field
    field_test_threat_level_.on_change = [this]() { on_test_threat_level_changed(); };

    // Add all elements to view
    add_children({
        &text_description_,
        &checkbox_audio_enabled_,
        &button_test_audio_,
        &field_test_threat_level_,
        &text_test_instructions_
    });
}

void DroneAudioSettingsView::focus() {
    checkbox_audio_enabled_.focus();
}

void DroneAudioSettingsView::on_audio_enabled_changed() {
    bool new_value = checkbox_audio_enabled_.value();
    settings_.audio_enabled = new_value;
    audio_alert_.enable_audio(new_value);

    // Trigger callback if set
    if (on_settings_changed_) {
        on_settings_changed_();
    }

    // Show confirmation
    std::string msg = new_value ? "Audio alerts enabled" : "Audio alerts disabled";
    nav_.display_modal("Audio Settings", msg);
}

void DroneAudioSettingsView::on_test_audio() {
    // Use current test threat level for the test beep
    audio_alert_.play_detection_beep(settings_.test_threat_level);

    // Show feedback
    std::string threat_name = [this]() {
        switch (settings_.test_threat_level) {
            case ThreatLevel::CRITICAL: return "CRITICAL";
            case ThreatLevel::HIGH: return "HIGH";
            case ThreatLevel::MEDIUM: return "MEDIUM";
            case ThreatLevel::LOW: return "LOW";
            case ThreatLevel::NONE:
            default: return "NONE";
        }
    }();

    std::string msg = "Testing audio for threat: " + threat_name;
    nav_.display_modal("Audio Test", msg);
}

void DroneAudioSettingsView::on_test_threat_level_changed() {
    int new_value = field_test_threat_level_.value();
    settings_.test_threat_level = static_cast<ThreatLevel>(new_value);

    // No callback needed here - just update the setting
}

} // namespace ui::external_app::enhanced_drone_analyzer
