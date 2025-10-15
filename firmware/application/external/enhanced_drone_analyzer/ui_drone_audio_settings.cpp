// ui_drone_audio_settings.cpp
// Audio settings UI implementation for Enhanced Drone Analyzer

#include "ui_drone_audio_settings.hpp"
#include "ui/ui_text_entry.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

DroneAudioSettingsView::DroneAudioSettingsView(
    NavigationView& nav,
    DroneAudioSettings& settings,
    DroneAudioAlert& audio_alert)
    : nav_(nav), settings_(settings), audio_alert_(audio_alert) {

    // Add UI components
    add_children({
        &text_description_,
        &checkbox_audio_enabled_,
        &field_test_threat_level_,
        &text_test_instructions_,
        &button_test_audio_
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

    if (settings_.audio_enabled) {
        audio_alert_.play_alert(test_level);
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
