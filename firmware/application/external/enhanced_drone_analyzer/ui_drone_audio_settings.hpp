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
    bool audio_enabled = true;             // Audio enabled by default
    ThreatLevel test_threat_level = ThreatLevel::MEDIUM;  // Safe default
};

// CONSOLIDATED: Merge DroneAudioController functionality into a unified AudioManager
class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    // Settings access (replaces DroneAudioSettings struct)
    bool is_audio_enabled() const { return audio_enabled_; }
    void set_audio_enabled(bool enabled) { audio_enabled_ = enabled; }
    ThreatLevel get_test_threat_level() const { return test_threat_level_; }
    void set_test_threat_level(ThreatLevel level) { test_threat_level_ = level; }

    // Audio playback (replaces DroneAudioController methods)
    void play_detection_beep(ThreatLevel level);
    void play_sos_signal();
    void stop_audio();

    // Beep frequency mapping
    uint16_t get_beep_frequency(ThreatLevel level) const;

    // Audio status
    uint32_t get_last_beep_time() const { return last_beep_time_; }

    // Static beep method for external access
    static void request_audio_beep(uint16_t frequency, uint32_t duration_ms);

    // Prevent copying
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

private:
    // Consolidated state (was split between struct and class)
    bool audio_enabled_;           // Enable/disable audio alerts
    ThreatLevel test_threat_level_; // Test audio threat level
    uint32_t last_beep_time_;      // Last beep timestamp

    // Audio device lifecycle
    void initialize_audio_device();
    void cleanup_audio_device();
};

// DEPRECATED: Old DroneAudioController class - keeping for compatibility
// Will be removed after full consolidation
class DroneAudioController {
public:
    DroneAudioController() : audio_enabled_(true), last_beep_time_(0) {}
    ~DroneAudioController() = default;
    void enable_audio() { audio_enabled_ = true; }
    void disable_audio() { audio_enabled_ = false; }
    bool is_audio_enabled() const { return audio_enabled_; }
    void toggle_audio() { audio_enabled_ = !audio_enabled_; }
    void play_detection_beep(ThreatLevel level) { /* DEPRECATED */ }
    void play_sos_signal() { /* DEPRECATED */ }
    void stop_audio() { /* DEPRECATED */ }
    uint16_t get_beep_frequency(ThreatLevel level) const { return 1000; }
    uint32_t get_last_beep_time() const { return last_beep_time_; }
    static void request_audio_beep(uint16_t frequency, uint32_t duration_ms) { /* DEPRECATED */ }

    DroneAudioController(const DroneAudioController&) = delete;
    DroneAudioController& operator=(const DroneAudioController&) = delete;

private:
    bool audio_enabled_;
    uint32_t last_beep_time_;
};

class DroneAudioSettingsView : public View {
public:
    DroneAudioSettingsView(NavigationView& nav, DroneAudioSettings& settings, DroneAudioController& audio_controller);

    void focus() override;
    std::string title() const override { return "Audio Settings"; }

    // Установить callback для обновления настроек в main view
    void set_on_settings_changed(std::function<void()> callback) {
        on_settings_changed_ = callback;
    }

private:
    NavigationView& nav_;
    DroneAudioSettings& settings_;
    DroneAudioController& audio_controller_;  // FIX: Updated member name and type

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

// FIXED: Removed typedef - constructor parameters updated

#endif // __UI_DRONE_AUDIO_SETTINGS_HPP__
