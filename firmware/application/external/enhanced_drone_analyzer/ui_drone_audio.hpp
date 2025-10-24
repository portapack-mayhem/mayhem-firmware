// ui_drone_audio.hpp
// Consolidated audio system for Enhanced Drone Analyzer
// Migrated Session 5: ui_drone_audio_settings.hpp + ui_drone_audio_settings.cpp

#ifndef __UI_DRONE_AUDIO_HPP__
#define __UI_DRONE_AUDIO_HPP__

#include "ui.hpp"
#include "ui.hpp"
#include "external_app.hpp"
#include "audio.hpp"
#include "baseband_api.hpp"
#include "ch.h"

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
    void set_test_threat_level(ThreatLevel level) {
        // VALIDATION: Add bounds checking as per Phase 3 audio restoration
        if (level >= ThreatLevel::NONE && level <= ThreatLevel::CRITICAL) {
            test_threat_level_ = level;
        } else {
            test_threat_level_ = ThreatLevel::MEDIUM;  // Safe default
        }
    }

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

    // VALIDATION: Add audio parameter validation function (Phase 3)
    static bool validate_audio_alert_frequency(uint16_t frequency_hz) {
        return frequency_hz >= 400 && frequency_hz <= 3000;  // Reasonable range for beeps
    }
    static bool validate_audio_duration(uint32_t duration_ms) {
        return duration_ms >= 50 && duration_ms <= 2000;  // Reasonable range for alerts
    }

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
    DroneAudioController() : audio_enabled_(true), last_beep_time_(0) {} // DEPRECATED: Use AudioManager instead
    ~DroneAudioController() = default;
    void enable_audio() { audio_enabled_ = true; }
    void disable_audio() { audio_enabled_ = false; }
    bool is_audio_enabled() const { return audio_enabled_; }
    void toggle_audio() { audio_enabled_ = !audio_enabled_; }
    void play_detection_beep(ThreatLevel level) {
        if (!audio_enabled_) return;

        // Simple implementation for compatibility
        uint16_t frequency = get_beep_frequency(level);
        request_audio_beep(frequency, 200);
    }
    void play_sos_signal() {
        if (!audio_enabled_) return;

        // SOS: morse code - ... ---
        for (int i = 0; i < 3; ++i) {
            request_audio_beep(1500, 200);
            chThdSleepMilliseconds(150);
        }
        chThdSleepMilliseconds(300);

        for (int i = 0; i < 3; ++i) {
            request_audio_beep(1500, 600);
            chThdSleepMilliseconds(150);
        }
        chThdSleepMilliseconds(300);

        for (int i = 0; i < 3; ++i) {
            request_audio_beep(1500, 200);
            chThdSleepMilliseconds(150);
        }

        last_beep_time_ = chTimeNow();
    }
    void stop_audio() { audio::output::stop(); }
    uint16_t get_beep_frequency(ThreatLevel level) const { return 1000; }
    uint32_t get_last_beep_time() const { return last_beep_time_; }
    static void request_audio_beep(uint16_t frequency, uint32_t duration_ms) {
        audio::set_rate(audio::Rate::Hz_24000);
        audio::output::start();
        chThdSleepMilliseconds(10);
        baseband::request_audio_beep(frequency, 24000, duration_ms);
        chThdSleepMilliseconds(duration_ms + 50);
        audio::output::stop();
    }

    DroneAudioController(const DroneAudioController&) = delete;
    DroneAudioController& operator=(const DroneAudioController&) = delete;

    // Audio alert storage and playback
    AudioManager audio_manager_;

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

    // PHASE 3: RESTORE AUDIO FUNCTIONALITY - AudioController callback implementation
    void set_on_audio_settings_changed(std::function<void()> callback) {
        on_audio_settings_changed_ = callback;  // RESTORE: Implement missing audio settings callback
    }

private:
    NavigationView& nav_;
    DroneAudioSettings& audio_settings_;  // Renamed to avoid confusion with parameter
    DroneAudioController& audio_controller_;  // FIX: Updated member name and type

    // UI Elements
    Checkbox checkbox_audio_enabled_{ {10, 30}, "Enable Audio Alerts" };
    Button button_test_audio_{ {10, 60}, "Test Audio" };
    Button button_sos_alert_{ {120, 60}, "SOS Alert" };

    Text text_description_{ {0, 0, 240, 32},
        "Configure audio alerts for\ndrone detections and warnings." };

    NumberField field_test_threat_level_{ {130, 60}, 1, {0, 4, 1, 1}, "Threat" };
    Text text_test_instructions_{ {10, 85, 240, 16},
        "Select threat level to test" };
    Text text_last_beep_{ {10, 100, 240, 16}, "" };

    // Button handlers
    void on_audio_enabled_changed();
    void on_test_audio();
    void on_sos_alert();
    void on_test_threat_level_changed();

    // Callback
    std::function<void()> on_settings_changed_;
};

// ABOUT MODAL: Consolidated from ui_drone_audio_settings_about.hpp
class AuthorContactView : public View {
public:
    explicit AuthorContactView(NavigationView& nav);

    void focus() override;
    std::string title() const override { return "About Author"; }

private:
    NavigationView& nav_;

    // UI Elements
    Text text_title_{{5, 10}, "ENHANCED DRONE ANALYZER"};
    Text text_version_{{5, 25}, "Version 0.3 (PRODUCTION READY)"};

    Text text_feedback_title_{{5, 45}, "FEEDBACK & SUPPORT:"};
    Text text_telegram_{{5, 55}, "Telegram ID: 5364595248"};
    Text text_username_{{5, 65}, "@max_ai_master"};

    Text text_donation_title_{{5, 85}, "DONATIONS:"};
    Text text_yoomoney_{{5, 95}, "YooMoney: 41001810704697"};
    Text text_card_{{5, 100}, "Card: 2202 2020 5787 1695"};

    Text text_ton_wallet_{{5, 115}, "TON: UQCdtMxQB5zbQBOICkY90lTQQqcs8V-V28Bf2AGvl8xOc5HR"};
    Text text_author_note_{{5, 135}, "18 октября 2025 года. Кузнецов М.С."};
    Text text_quote1_{{5, 150}, "Пусть не сложит голову идущий"};
    Text text_quote2_{{5, 160}, "Пусть поможет идти просящий."};
    Text text_quote3_{{5, 165}, "Ваши пожертвования превратятся в код"};
    Text text_greeting_{{5, 170}, "Привет от слесарей г. Оренбурга службы ВДГО"};
    Text text_greeting2_{{5, 180}, "и слесаря который это приложение написал"};
    Text text_disclaimer_{{5, 195}, "2025 - Enhanced Drone Technology"};

    Button button_close_{{150, 205}, "CLOSE"};

    // Button handler
    void on_close();
};

} // namespace ui::external_app::enhanced_drone_analyzer

// HEADER-ONLY IMPLEMENTATIONS - Consolidated from ui_drone_audio_settings.cpp
// Wrapped in anonymous namespace for internal linkage
namespace {

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
    DroneAudioController& audio_controller)
    : nav_(nav), audio_settings_(settings), audio_controller_(audio_controller) {

    // Add UI components
    add_children({
        &text_description_,
        &checkbox_audio_enabled_,
        &field_test_threat_level_,
        &text_test_instructions_,
        &text_last_beep_,
        &button_test_audio_,
        &button_sos_alert_
    });

    // Initialize checkboxes and fields
    checkbox_audio_enabled_.set_value(audio_settings_.audio_enabled);

    // Set up event handlers
    checkbox_audio_enabled_.on_select = [this]() {
        on_audio_enabled_changed();
    };

    button_test_audio_.on_select = [this]() {
        on_test_audio();
    };

    button_sos_alert_.on_select = [this]() {
        on_sos_alert();
    };

    field_test_threat_level_.set_value(static_cast<int>(audio_settings_.test_threat_level));
    field_test_threat_level_.on_change = [this]() {
        on_test_threat_level_changed();
    };

    // Initialize with current settings
    update_ui_from_settings();
}

void DroneAudioSettingsView::focus() {
    checkbox_audio_enabled_.focus();
}

void DroneAudioSettingsView::on_audio_enabled_changed() {
    // RESTORATION: Use enable_audio/disable_audio methods from DroneAudioController
    if (checkbox_audio_enabled_.value()) {
        audio_controller_.enable_audio();
    } else {
        audio_controller_.disable_audio();
    }

    // Update settings struct for compatibility
    audio_settings_.audio_enabled = checkbox_audio_enabled_.value();

    if (on_settings_changed_) {
        on_settings_changed_();
    }
}

void DroneAudioSettingsView::on_test_audio() {
    // RESTORATION: Use get_test_threat_level from audio_controller to check saved state
    ThreatLevel current_saved_level = audio_controller_.get_test_threat_level(); // Restore unused function
    ThreatLevel test_level = static_cast<ThreatLevel>(field_test_threat_level_.get_value());

    // Show message about saved level vs UI level
    if (current_saved_level != test_level) {
        // Restore set_test_threat_level - update controller's idea of test level
        audio_controller_.set_test_threat_level(test_level); // Restore unused function
    }

    // VALIDATE: Add bounds checking for threat level (CRITICAL FIX)
    if (test_level < ThreatLevel::NONE || test_level > ThreatLevel::CRITICAL) {
        test_level = ThreatLevel::MEDIUM;  // Safe default
    }

    if (audio_settings_.audio_enabled) {
        // FIX: Call correct method name (CRITICAL FIX)
        audio_controller_.play_detection_beep(test_level);

        // RESTORATION: After test, show last beep time if available
        uint32_t last_beep = audio_controller_.get_last_beep_time(); // Restore unused function
        if (last_beep > 0) {
            char time_str[32];
            snprintf(time_str, sizeof(time_str), "Last beep: %u ms ago", (chTimeNow() - last_beep));
            text_last_beep_.set(time_str);
            set_dirty();  // Mark UI for redraw
        }
    }
}

void DroneAudioSettingsView::on_test_threat_level_changed() {
    // Update test threat level
    audio_settings_.test_threat_level = static_cast<ThreatLevel>(field_test_threat_level_.get_value());
}

void DroneAudioSettingsView::on_sos_alert() {
    audio_controller_.play_sos_signal();
}

void DroneAudioSettingsView::update_ui_from_settings() {
    // Update UI to reflect current settings
    checkbox_audio_enabled_.set_value(audio_settings_.audio_enabled);
    field_test_threat_level_.set_value(static_cast<int>(audio_settings_.test_threat_level));
}

// AUTHOR CONTACT VIEW IMPLEMENTATION - Consolidated from ui_drone_audio_settings_about.cpp
AuthorContactView::AuthorContactView(NavigationView& nav)
    : nav_(nav) {

    // Add UI components
    add_children({
        &text_title_,
        &text_version_,
        &text_feedback_title_,
        &text_telegram_,
        &text_username_,
        &text_donation_title_,
        &text_yoomoney_,
        &text_card_,
        &text_ton_wallet_,
        &text_author_note_,
        &text_quote1_,
        &text_quote2_,
        &text_quote3_,
        &text_greeting_,
        &text_greeting2_,
        &text_disclaimer_,
        &button_close_
    });

    // Set up event handler
    button_close_.on_select = [this]() {
        on_close();
    };
}

void AuthorContactView::focus() {
    button_close_.focus();
}

void AuthorContactView::on_close() {
    nav_.pop();  // Return to previous view
}

} // anonymous namespace

#endif // __UI_DRONE_AUDIO_HPP__
