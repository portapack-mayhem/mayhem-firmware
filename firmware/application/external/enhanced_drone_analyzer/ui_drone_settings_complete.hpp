// ui_drone_settings_complete.hpp - Consolidated settings management for Enhanced Drone Analyzer
// Migration Session 6: ui_drone_settings.hpp + ui_drone_settings.cpp + ui_drone_settings_view.hpp + ui_drone_settings_view.cpp → ui_drone_settings_complete.hpp
// RESTORATION: Resolved git merge conflicts and restored complete functionality

#ifndef __UI_DRONE_SETTINGS_COMPLETE_HPP__
#define __UI_DRONE_SETTINGS_COMPLETE_HPP__

#include "app_settings.hpp"
#include "ui_drone_config.hpp"

#include "ui.hpp"
#include "ui_tabview.hpp"
#include "ui_button.hpp"
#include "ui_checkbox.hpp"
#include "ui_number_field.hpp"
#include "ui_options_field.hpp"
#include "ui_text.hpp"

#include "portapack.hpp"

#include <string>
#include <sstream>

namespace ui::external_app::enhanced_drone_analyzer {

// FIXED: Single consolidated Language enum (removed duplicate from merge conflict)
enum class Language {
    ENGLISH,
    RUSSIAN
};

// Translation system (simplified inline implementation)
class Translator {
public:
    static void set_language(Language lang) { current_language_ = lang; }
    static Language get_language() { return current_language_; }

    static const char* translate(const std::string& key) {
        static const std::map<Language, std::map<std::string, std::string>> translations = {
            {Language::ENGLISH, {
                {"app_title", "Enhanced Drone Analyzer"},
                {"start_stop", "START/STOP"},
                {"load_database", "Load Database"},
                {"audio_settings", "Audio Settings"},
                {"advanced", "Advanced"},
                {"ready", "Ready - Enhanced Drone Analyzer"}
            }},
            {Language::RUSSIAN, {
                {"app_title", "Расширенный Анализатор Дронов"},
                {"start_stop", "СТАРТ/СТОП"},
                {"load_database", "Загрузить Базу"},
                {"audio_settings", "Настройки Аудио"},
                {"advanced", "Дополнительно"},
                {"ready", "Готов - Расширенный Анализатор Дронов"}
            }}
        };

        auto lang_it = translations.find(current_language_);
        if (lang_it != translations.end()) {
            auto str_it = lang_it->second.find(key);
            if (str_it != lang_it->second.end()) {
                return str_it->second.c_str();
            }
        }
        return key.c_str(); // Fallback to key if translation not found
    }

private:
    static Language current_language_;
};

Language Translator::current_language_ = Language::ENGLISH;

// Перегруппированные настройки для лучшей организации UI
struct DroneSpectrumSettings {
    uint32_t min_scan_interval_ms = 1000;  // Скорректировано с 750ms для энергоэффективности
    uint32_t stale_timeout_ms = 30000;
    int32_t default_rssi_threshold = -90;
    float rssi_smoothing_alpha = 0.6f;
};

struct DroneDetectionSettings {
    uint8_t min_detection_count = 5;
    int32_t hysteresis_margin_db = 5;
    int16_t trend_threshold_db = 5;
    uint32_t detection_reset_interval = 100;
};

struct DroneAudioSettings {
    uint16_t alert_frequency_hz = 800;
    uint32_t beep_duration_ms = 100;
    int32_t alert_squelch_db = -80;
};

struct DroneDisplaySettings {
    bool show_rssi_graph = true;
    bool show_trends = true;
    size_t max_display_drones = 3;
    bool color_scheme = 0;
};

struct DroneAnalyzerSettings {
public:
    SpectrumMode spectrum_mode = SpectrumMode::MEDIUM;

    DroneSpectrumSettings spectrum;
    DroneDetectionSettings detection;
    DroneAudioSettings audio;
    DroneDisplaySettings display;

    void reset_to_defaults();

    std::string serialize() const {
        std::stringstream ss;
        ss << "v1:" << static_cast<int>(spectrum_mode) << ":"
           << spectrum.min_scan_interval_ms << ":"
           << spectrum.stale_timeout_ms << ":"
           << spectrum.default_rssi_threshold << ":"
           << spectrum.rssi_smoothing_alpha << ":"
           << static_cast<int>(detection.min_detection_count) << ":"
           << detection.hysteresis_margin_db << ":"
           << detection.trend_threshold_db << ":"
           << detection.detection_reset_interval << ":"
           << audio.alert_frequency_hz << ":"
           << audio.beep_duration_ms << ":"
           << audio.alert_squelch_db << ":"
           << (display.show_rssi_graph ? "1" : "0") << ":"
           << (display.show_trends ? "1" : "0") << ":"
           << display.max_display_drones << ":"
           << (display.color_scheme ? "1" : "0");
        return ss.str();
    }

    bool deserialize(const std::string& data) {
        std::stringstream ss(data);
        std::string token;

        if (!std::getline(ss, token, ':')) return false;
        if (token != "v1") return false;

        int spectrum_mode_int;
        if (!std::getline(ss, token, ':')) return false;
        spectrum_mode_int = std::stoi(token);
        spectrum_mode = static_cast<SpectrumMode>(spectrum_mode_int);

        // Spectrum settings
        if (!std::getline(ss, token, ':')) return false;
        spectrum.min_scan_interval_ms = static_cast<uint32_t>(std::stoul(token));
        if (!std::getline(ss, token, ':')) return false;
        spectrum.stale_timeout_ms = static_cast<uint32_t>(std::stoul(token));
        if (!std::getline(ss, token, ':')) return false;
        spectrum.default_rssi_threshold = std::stoi(token);
        if (!std::getline(ss, token, ':')) return false;
        spectrum.rssi_smoothing_alpha = std::stof(token);

        // Detection settings
        if (!std::getline(ss, token, ':')) return false;
        detection.min_detection_count = static_cast<uint8_t>(std::stoi(token));
        if (!std::getline(ss, token, ':')) return false;
        detection.hysteresis_margin_db = std::stoi(token);
        if (!std::getline(ss, token, ':')) return false;
        detection.trend_threshold_db = std::stoi(token);
        if (!std::getline(ss, token, ':')) return false;
        detection.detection_reset_interval = static_cast<uint32_t>(std::stoul(token));

        // Audio settings
        if (!std::getline(ss, token, ':')) return false;
        audio.alert_frequency_hz = static_cast<uint16_t>(std::stoi(token));
        if (!std::getline(ss, token, ':')) return false;
        audio.beep_duration_ms = static_cast<uint32_t>(std::stoul(token));
        if (!std::getline(ss, token, ':')) return false;
        audio.alert_squelch_db = std::stoi(token);

        // Display settings
        if (!std::getline(ss, token, ':')) return false;
        display.show_rssi_graph = (token == "1");
        if (!std::getline(ss, token, ':')) return false;
        display.show_trends = (token == "1");
        if (!std::getline(ss, token, ':')) return false;
        display.max_display_drones = static_cast<size_t>(std::stoul(token));
        if (!std::getline(ss, token, ':')) return false;
        display.color_scheme = (token == "1");

        return true;
    }
};

// Глобальная функция для получения настроек по умолчанию
const DroneAnalyzerSettings& get_default_settings();

// Settings persistence manager
class DroneSettingsManager {
public:
    static bool load_settings(DroneAnalyzerSettings& settings) {
        app_settings::SettingsManager settings_manager{"eda_main", app_settings::Mode::SETTINGS, {}};
        std::string serialized = "";
        bool success = settings_manager.load() &&
                      settings_manager.get_value("config", serialized) &&
                      settings.deserialize(serialized);
        if (!success) {
            settings = get_default_settings();
        }
        return success;
    }

    static bool save_settings(const DroneAnalyzerSettings& settings) {
        app_settings::SettingsManager settings_manager{"eda_main", app_settings::Mode::SETTINGS, {}};
        std::string serialized = settings.serialize();
        return settings_manager.set_value("config", serialized) && settings_manager.save();
    }

    static void reset_to_defaults(DroneAnalyzerSettings& settings) {
        settings = get_default_settings();
    }
};

// Settings dialog view with tabbed interface
class DroneSettingsView : public View {
public:
    DroneSettingsView(NavigationView& nav, DroneAnalyzerSettings& settings);

    std::string title() const override { return "Drone Analyzer Settings"; };
    void focus() override { tab_view.focus(); }

private:
    NavigationView& nav_;
    DroneAnalyzerSettings& settings_;
    DroneAnalyzerSettings backup_settings_;

    TabView tab_view{
        {0, 0}, {screen_width, screen_height - 3 * 16},
        {"Spectrum", "Detection", "Audio", "Display"}
    };

    Button button_save{
        {screen_width/2 - 40, screen_height - 2 * 16, 80, 2 * 16},
        "Save"
    };

    Button button_cancel{
        {screen_width/2 - 40, screen_height - 16, 80, 16},
        "Cancel"
    };

    // Spectrum tab
    Text text_spectrum_title{{8, 2 * 16}, "Spectrum Analysis"};
    Text text_scan_interval_label{{8, 4 * 16 + 8}, "Scan Int.:", Theme::getInstance()->fg_light->foreground};
    NumberField field_scan_interval{
        {8 * 9, 4 * 16 + 8}, 4, {200, 5000}, 100, ' '
    };

    // Detection tab
    Text text_detection_title{{8, 2 * 16}, "Signal Detection"};
    Text text_min_detections_label{{8, 3 * 16 + 8}, "Min Detections:", Theme::getInstance()->fg_light->foreground};
    NumberField field_min_detections{
        {8 * 14, 3 * 16 + 8}, 2, {1, 10}, 1, ' '
    };

    // Audio tab
    Text text_audio_title{{8, 2 * 16}, "Audio Alerts"};
    Text text_alert_freq_label{{8, 3 * 16 + 8}, "Alert Freq.:", Theme::getInstance()->fg_light->foreground};
    NumberField field_alert_freq{
        {8 * 11, 3 * 16 + 8}, 4, {400, 3000}, 50, ' '
    };

    // Display tab
    Text text_display_title{{8, 2 * 16}, "Display Settings"};

    void setup_ui_elements();
    void load_settings_to_ui();
    void save_ui_to_settings();
    void validate_settings();
    void show_validation_error(const std::string& message);
};

class ConstantSettingsView : public View {
public:
    explicit ConstantSettingsView(NavigationView& nav);
    ~ConstantSettingsView() override = default;

    void focus() override;
    std::string title() const override { return "Constant Settings"; }

private:
    NavigationView& nav_;

    Text text_title_{{8, 8}, "Constant Settings"};
    Text text_min_freq_{{8, 32}, "Min Drone Freq (MHz):"};
    NumberField field_min_drone_freq_{{120, 32}, 4, {400}, 6000, 240};

    Button button_ok_{{screen_width/2 - 20, screen_height - 24, 40, 16}, "OK"};
    void on_ok_pressed();
};

} // namespace ui::external_app::enhanced_drone_analyzer

// HEADER-ONLY IMPLEMENTATIONS - From ui_drone_settings.cpp
namespace {

using namespace ui::external_app::enhanced_drone_analyzer;

void DroneAnalyzerSettings::reset_to_defaults() {
    spectrum_mode = SpectrumMode::MEDIUM;

    spectrum.min_scan_interval_ms = 1000u;
    spectrum.stale_timeout_ms = 30000u;
    spectrum.default_rssi_threshold = -90;
    spectrum.rssi_smoothing_alpha = 0.6f;

    detection.min_detection_count = 5u;
    detection.hysteresis_margin_db = 5;
    detection.trend_threshold_db = 5;
    detection.detection_reset_interval = 100u;

    audio.alert_frequency_hz = 800;
    audio.beep_duration_ms = 100;
    audio.alert_squelch_db = -80;

    display.show_rssi_graph = true;
    display.show_trends = true;
    display.max_display_drones = 3;
    display.color_scheme = 0;
}

std::string DroneAnalyzerSettings::serialize() const {
    std::stringstream ss;
    ss << "v1:" << static_cast<int>(spectrum_mode) << ":"
       << spectrum.min_scan_interval_ms << ":"
       << spectrum.stale_timeout_ms << ":"
       << spectrum.default_rssi_threshold << ":"
       << spectrum.rssi_smoothing_alpha << ":"
       << static_cast<int>(detection.min_detection_count) << ":"
       << detection.hysteresis_margin_db << ":"
       << detection.trend_threshold_db << ":"
       << detection.detection_reset_interval << ":"
       << audio.alert_frequency_hz << ":"
       << audio.beep_duration_ms << ":"
       << audio.alert_squelch_db << ":"
       << (display.show_rssi_graph ? "1" : "0") << ":"
       << (display.show_trends ? "1" : "0") << ":"
       << display.max_display_drones << ":"
       << (display.color_scheme ? "1" : "0");
    return ss.str();
}

bool DroneAnalyzerSettings::deserialize(const std::string& data) {
    std::stringstream ss(data);
    std::string token;

    if (!std::getline(ss, token, ':') || token != "v1") return false;

    int spectrum_mode_int;
    if (!std::getline(ss, token, ':')) return false;
    spectrum_mode_int = std::stoi(token);
    spectrum_mode = static_cast<SpectrumMode>(spectrum_mode_int);

    // Spectrum settings
    if (!std::getline(ss, token, ':')) return false;
    spectrum.min_scan_interval_ms = static_cast<uint32_t>(std::stoul(token));
    if (!std::getline(ss, token, ':')) return false;
    spectrum.stale_timeout_ms = static_cast<uint32_t>(std::stoul(token));
    if (!std::getline(ss, token, ':')) return false;
    spectrum.default_rssi_threshold = std::stoi(token);
    if (!std::getline(ss, token, ':')) return false;
    spectrum.rssi_smoothing_alpha = std::stof(token);

    // Detection settings
    if (!std::getline(ss, token, ':')) return false;
    detection.min_detection_count = static_cast<uint8_t>(std::stoi(token));
    if (!std::getline(ss, token, ':')) return false;
    detection.hysteresis_margin_db = std::stoi(token);
    if (!std::getline(ss, token, ':')) return false;
    detection.trend_threshold_db = std::stoi(token);
    if (!std::getline(ss, token, ':')) return false;
    detection.detection_reset_interval = static_cast<uint32_t>(std::stoul(token));

    // Audio settings
    if (!std::getline(ss, token, ':')) return false;
    audio.alert_frequency_hz = static_cast<uint16_t>(std::stoi(token));
    if (!std::getline(ss, token, ':')) return false;
    audio.beep_duration_ms = static_cast<uint32_t>(std::stoul(token));
    if (!std::getline(ss, token, ':')) return false;
    audio.alert_squelch_db = std::stoi(token);

    // Display settings
    if (!std::getline(ss, token, ':')) return false;
    display.show_rssi_graph = (token == "1");
    if (!std::getline(ss, token, ':')) return false;
    display.show_trends = (token == "1");
    if (!std::getline(ss, token, ':')) return false;
    display.max_display_drones = static_cast<size_t>(std::stoul(token));
    if (!std::getline(ss, token, ':')) return false;
    display.color_scheme = (token == "1");

    return true;
}

const DroneAnalyzerSettings& get_default_settings() {
    static DroneAnalyzerSettings defaults;
    static bool initialized = false;

    if (!initialized) {
        defaults.reset_to_defaults();
        initialized = true;
    }

    return defaults;
}

DroneSettingsView::DroneSettingsView(NavigationView& nav, DroneAnalyzerSettings& settings)
    : nav_(nav), settings_(settings), backup_settings_(settings)
{
    add_children({
        &tab_view,
        &button_save,
        &button_cancel,

        // Spectrum tab
        &text_spectrum_title,
        &text_scan_interval_label, &field_scan_interval,

        // Detection tab
        &text_detection_title,
        &text_min_detections_label, &field_min_detections,

        // Audio tab
        &text_audio_title,
        &text_alert_freq_label, &field_alert_freq,

        // Display tab
        &text_display_title
    });

    setup_ui_elements();
    load_settings_to_ui();
}

void DroneSettingsView::setup_ui_elements() {
    button_save.on_select = [this](Button&) {
        if (validate_settings()) {
            save_ui_to_settings();
            nav_.pop();
        }
    };

    button_cancel.on_select = [this](Button&) {
        settings_ = backup_settings_;
        nav_.pop();
    };
}

void DroneSettingsView::load_settings_to_ui() {
    field_scan_interval.set_value(settings_.spectrum.min_scan_interval_ms);
    field_min_detections.set_value(settings_.detection.min_detection_count);
    field_alert_freq.set_value(settings_.audio.alert_frequency_hz);
}

void DroneSettingsView::save_ui_to_settings() {
    settings_.spectrum.min_scan_interval_ms = field_scan_interval.to_integer();
    settings_.detection.min_detection_count = field_min_detections.to_integer();
    settings_.audio.alert_frequency_hz = field_alert_freq.to_integer();
}

void DroneSettingsView::validate_settings() {
    // Basic validation
    if (field_scan_interval.to_integer() < 200 || field_scan_interval.to_integer() > 5000) {
        show_validation_error("Scan interval must be 200-5000 ms");
        field_scan_interval.focus();
        return;
    }

    if (field_alert_freq.to_integer() < 400 || field_alert_freq.to_integer() > 3000) {
        show_validation_error("Alert frequency must be 400-3000 Hz");
        field_alert_freq.focus();
        return;
    }
}

void DroneSettingsView::show_validation_error(const std::string& message) {
    nav_.display_modal("Validation Error", message);
}

// Constant settings class
class ConstantSettingsManager {
public:
    static bool load_settings() {
        app_settings::SettingsManager settings_manager{"eda_constants", app_settings::Mode::SETTINGS, {
            {"min_drone_freq", reinterpret_cast<void*>(&current_min_drone_freq)},
            {"max_drone_freq", reinterpret_cast<void*>(&current_max_drone_freq)}
        }};
        return settings_manager.load();
    }

    static bool save_settings() {
        app_settings::SettingsManager settings_manager{"eda_constants", app_settings::Mode::SETTINGS, {
            {"min_drone_freq", reinterpret_cast<void*>(&current_min_drone_freq)},
            {"max_drone_freq", reinterpret_cast<void*>(&current_max_drone_freq)}
        }};
        return settings_manager.save();
    }

    static void reset_to_defaults() {
        current_min_drone_freq = 2400000000ULL;
        current_max_drone_freq = 6000000000ULL;
    }

    static rf::Frequency current_min_drone_freq;
    static rf::Frequency current_max_drone_freq;
};

rf::Frequency ConstantSettingsManager::current_min_drone_freq = 2400000000ULL;
rf::Frequency ConstantSettingsManager::current_max_drone_freq = 6000000000ULL;

ConstantSettingsView::ConstantSettingsView(NavigationView& nav)
    : nav_(nav) {
    add_children({
        &text_title_,
        &text_min_freq_,
        &field_min_drone_freq_,
        &button_ok_
    });

    button_ok_.on_select = [this]() { on_ok_pressed(); };

    field_min_drone_freq_.set_value(ConstantSettingsManager::current_min_drone_freq / 1000000);
}

void ConstantSettingsView::focus() {
    field_min_drone_freq_.focus();
}

void ConstantSettingsView::on_ok_pressed() {
    ConstantSettingsManager::current_min_drone_freq = field_min_drone_freq_.get_value() * 1000000ULL;
    nav_.pop();
}

} // anonymous namespace

#endif // __UI_DRONE_SETTINGS_COMPLETE_HPP__
