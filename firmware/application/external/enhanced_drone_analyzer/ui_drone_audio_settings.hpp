// ui_drone_audio_settings.hpp
// Audio settings UI for Enhanced Drone Analyzer

#ifndef __UI_DRONE_AUDIO_SETTINGS_HPP__
#define __UI_DRONE_AUDIO_SETTINGS_HPP__

#include "ui.hpp"
#include "ui/ui_checkbox.hpp"
#include "ui/ui_button.hpp"
#include "ui/navigation.hpp"
#include "external_app.hpp"
#include "ui_drone_audio.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

struct DroneAudioSettings {
    bool audio_enabled;
    ThreatLevel test_threat_level;
};

class DroneAudioSettingsView : public View {
public:
    DroneAudioSettingsView(NavigationView& nav, DroneAudioSettings& settings, DroneAudioAlert& audio_alert);

    void focus() override;
    std::string title() const override { return "Audio Settings"; }

    // Установить callback для обновления настроек в main view
    void set_on_settings_changed(std::function<void()> callback) {
        on_settings_changed_ = callback;
    }

private:
    NavigationView& nav_;
    DroneAudioSettings& settings_;
    DroneAudioAlert& audio_alert_;

    // UI Elements
    Checkbox checkbox_audio_enabled_{ {10, 30}, "Enable Audio Alerts" };
    Button button_test_audio_{ {10, 60}, "Test Audio" };

    Text text_description_{ {0, 0, 240, 32},
        "Configure audio alerts for\ndrone detections and warnings." };

    NumberField field_test_threat_level_{ {130, 60}, 1, {0, 4, 1, 1}, "Threat" };
    Text text_test_instructions_{ {10, 85, 240, 16},
        "Select threat level to test" };
    Button button_about_{ {180, 135}, "ABOUT", 14 };  // Right bottom corner

    // Button handlers
    void on_audio_enabled_changed();
    void on_test_audio();
    void on_test_threat_level_changed();

    // Callback
    std::function<void()> on_settings_changed_;
};

} // namespace ui::external_app::enhanced_drone_analyzer

// FIX: Add typedef for DroneAudioAlert used in constructor
using DroneAudioAlert = DroneAudioController;

#endif // __UI_DRONE_AUDIO_SETTINGS_HPP__
