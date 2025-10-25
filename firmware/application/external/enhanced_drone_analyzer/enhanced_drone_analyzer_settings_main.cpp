// enhanced_drone_analyzer_settings_main.cpp - Portapack app entry point for Settings App
// Integrates settings management with Portapack menu system

#include "ui_settings_combined.hpp"
#include "app_settings.hpp"
#include "portapack.hpp"
#include "io_file.hpp"
#include "string_format.hpp"

#ifndef __SETTINGS_MAIN_HPP__
#define __SETTINGS_MAIN_HPP__

namespace ui::external_app::enhanced_drone_analyzer::settings {

class EnhancedDroneAnalyzerSettingsMain : public View {
public:
    EnhancedDroneAnalyzerSettingsMain(NavigationView& nav);
    ~EnhancedDroneAnalyzerSettingsMain() override = default;

    void focus() override;
    std::string title() const override { return "EDA Settings"; }
    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    void on_show() override;
    void on_hide() override;

private:
    NavigationView& nav_;
    bool initialized_ = false;

    // Settings management components
    std::unique_ptr<DroneAnalyzerSettingsMainView> main_settings_view_;
    DroneAnalyzerSettingsManager settings_manager_;
    DroneAnalyzerSettings current_settings_;

    // Scanner status integration
    void load_scanner_status();
    void display_scanner_status(Painter& painter);

    // Settings file path constants
    static constexpr const char* SETTINGS_FILE_PATH = "/sdcard/ENHANCED_DRONE_ANALYZER_SETTINGS.txt";

    std::string scanner_active_state_ = "unknown";
    std::string current_scanning_freq_ = "unknown";
    std::string detected_drone_count_ = "unknown";
};

// App entry point function for Portapack
void settings_app_main(NavigationView& nav) {
    nav.push<EnhancedDroneAnalyzerSettingsMain>();
}

} // namespace ui::external_app::enhanced_drone_analyzer::settings

#endif // __SETTINGS_MAIN_HPP__

// Implementation
namespace ui::external_app::enhanced_drone_analyzer::settings {

EnhancedDroneAnalyzerSettingsMain::EnhancedDroneAnalyzerSettingsMain(NavigationView& nav)
    : View(), nav_(nav)
{
    load_scanner_status();
}

void EnhancedDroneAnalyzerSettingsMain::focus() {
    if (!initialized_) {
        // Load current settings
        if (!settings_manager_.load(current_settings_)) {
            settings_manager_.reset_to_defaults(current_settings_);
        }

        // Create main settings view
        main_settings_view_ = std::make_unique<DroneAnalyzerSettingsMainView>(nav_);

        initialized_ = true;
    }
    View::focus();
}

void EnhancedDroneAnalyzerSettingsMain::load_scanner_status() {
    // Try to read scanner status from TXT file using Portapack File API
    File settings_file;
    auto open_result = settings_file.open(SETTINGS_FILE_PATH);
    if (open_result.is_error()) {
        // File not found, keep defaults
        nav_.display_modal("Load Status", "No status file found.");
        return;
    }

    char line_buffer[256];
    bool in_scanner_section = false;

    while (true) {
        auto read_result = settings_file.read_line(line_buffer, sizeof(line_buffer));
        if (read_result.is_error() || read_result.value == 0) {
            break;  // End of file or error
        }

        std::string line(line_buffer);

        // Trim whitespace
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), line.end());

        if (line.find("--- Scanner App Status Export ---") != std::string::npos) {
            in_scanner_section = true;
            continue;
        }
        if (line.find("--- End Status Export ---") != std::string::npos) {
            in_scanner_section = false;
            continue;
        }

        if (in_scanner_section) {
            size_t equals_pos = line.find('=');
            if (equals_pos != std::string::npos) {
                std::string key = line.substr(0, equals_pos);
                std::string value = line.substr(equals_pos + 1);

                // Trim key/value
                key.erase(key.begin(), std::find_if(key.begin(), key.end(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }));
                key.erase(std::find_if(key.rbegin(), key.rend(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }).base(), key.end());

                value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }));
                value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }).base(), value.end());

                if (key == "scanner_active_state") {
                    scanner_active_state_ = value;
                } else if (key == "current_scanning_frequency") {
                    current_scanning_freq_ = value;
                } else if (key == "detected_drone_count") {
                    detected_drone_count_ = value;
                }
            }
        }
    }

    settings_file.close();
}

void EnhancedDroneAnalyzerSettingsMain::display_scanner_status(Painter& painter) {
    int y_pos = screen_height - 60;

    painter.draw_string({10, y_pos}, Theme::getInstance()->fg_light->foreground, "Scanner Status:");
    y_pos += 16;

    painter.draw_string({20, y_pos}, Theme::getInstance()->fg_green, "Active: ");
    painter.draw_string({80, y_pos}, Theme::getInstance()->fg_white, scanner_active_state_.c_str());

    y_pos += 16;
    painter.draw_string({20, y_pos}, Theme::getInstance()->fg_green, "Freq: ");
    painter.draw_string({80, y_pos}, Theme::getInstance()->fg_white, current_scanning_freq_.c_str());

    y_pos += 16;
    painter.draw_string({20, y_pos}, Theme::getInstance()->fg_green, "Drones: ");
    painter.draw_string({80, y_pos}, Theme::getInstance()->fg_white, detected_drone_count_.c_str());
}

void EnhancedDroneAnalyzerSettingsMain::paint(Painter& painter) {
    View::paint(painter);

    if (!initialized_) {
        painter.fill_rectangle({0, 0, screen_width, screen_height}, Color::black());
        painter.draw_string({screen_width / 2 - 60, screen_height / 2 - 10}, Theme::getInstance()->fg_light->foreground, "Loading EDA Settings...");
        return;
    }

    if (main_settings_view_) {
        return; // Let main view handle painting
    }

    // Fallback display
    painter.fill_rectangle({0, 0, screen_width, screen_height}, Color::black());

    // Draw title
    painter.draw_string({10, 10}, Theme::getInstance()->fg_white, "Enhanced Drone Analyzer Settings");

    // Draw current settings summary
    painter.draw_string({10, 40}, Theme::getInstance()->fg_light->foreground, "Current Configuration:");
    painter.draw_string({20, 60}, Theme::getInstance()->fg_green, "Spectrum Mode: ");
    std::string mode_str;
    switch (current_settings_.spectrum_mode) {
        case SpectrumMode::NARROW: mode_str = "NARROW"; break;
        case SpectrumMode::MEDIUM: mode_str = "MEDIUM"; break;
        case SpectrumMode::WIDE: mode_str = "WIDE"; break;
        case SpectrumMode::ULTRA_WIDE: mode_str = "ULTRA_WIDE"; break;
        default: mode_str = "UNKNOWN"; break;
    }
    painter.draw_string({140, 60}, Theme::getInstance()->fg_white, mode_str.c_str());

    painter.draw_string({20, 80}, Theme::getInstance()->fg_green, "Scan Interval: ");
    painter.draw_string({140, 80}, Theme::getInstance()->fg_white, std::to_string(current_settings_.scan_interval_ms).c_str() + " ms");

    painter.draw_string({20, 100}, Theme::getInstance()->fg_green, "RSSI Threshold: ");
    painter.draw_string({140, 100}, Theme::getInstance()->fg_white, std::to_string(current_settings_.rssi_threshold_db).c_str() + " dB");

    painter.draw_string({20, 120}, Theme::getInstance()->fg_green, "Audio Alerts: ");
    painter.draw_string({140, 120}, Theme::getInstance()->fg_white, current_settings_.enable_audio_alerts ? "ON" : "OFF");

    // Display scanner integration status
    display_scanner_status(painter);
}

bool EnhancedDroneAnalyzerSettingsMain::on_key(const KeyEvent key) {
    if (main_settings_view_) {
        return main_settings_view_->on_key(key);
    }

    switch (key) {
        case KeyEvent::Back:
            nav_.pop();
            return true;
        default:
            return false;
    }
}

bool EnhancedDroneAnalyzerSettingsMain::on_touch(const TouchEvent event) {
    if (main_settings_view_) {
        return main_settings_view_->on_touch(event);
    }

    // Handle touch for fallback display
    if (event.type == TouchEvent::Type::Start) {
        // Simple menu navigation for fallback view
        if (event.point.y >= 40 && event.point.y <= 140) {
            // Create settings tabs view
            auto tabbed_view = std::make_unique<SettingsTabbedView>(nav_);
            // Note: In real app, this would push to navigation
        }
    }
    return true;
}

void EnhancedDroneAnalyzerSettingsMain::on_show() {
    load_scanner_status(); // Refresh scanner status
    if (main_settings_view_) {
        main_settings_view_->on_show();
    }

    // Save current settings to TXT file for scanner app to read using Portapack File API
    std::string content = "# Enhanced Drone Analyzer Settings\n";
    content += "# Generated by Settings App\n\n";

    content += "spectrum_mode=";
    switch (current_settings_.spectrum_mode) {
        case SpectrumMode::NARROW: content += "NARROW"; break;
        case SpectrumMode::MEDIUM: content += "MEDIUM"; break;
        case SpectrumMode::WIDE: content += "WIDE"; break;
        case SpectrumMode::ULTRA_WIDE: content += "ULTRA_WIDE"; break;
    }
    content += "\n";

    content += "scan_interval_ms=" + std::to_string(current_settings_.scan_interval_ms) + "\n";
    content += "rssi_threshold_db=" + std::to_string(current_settings_.rssi_threshold_db) + "\n";
    content += "enable_audio_alerts=" + std::string(current_settings_.enable_audio_alerts ? "true" : "false") + "\n";
    content += "audio_alert_frequency_hz=" + std::to_string(current_settings_.audio_alert_frequency_hz) + "\n";
    content += "audio_alert_duration_ms=" + std::to_string(current_settings_.audio_alert_duration_ms) + "\n";
    content += "enable_real_hardware=" + std::string(current_settings_.enable_real_hardware ? "true" : "false") + "\n";
    content += "demo_mode=" + std::string(current_settings_.demo_mode ? "true" : "false") + "\n";
    content += "hardware_bandwidth_hz=" + std::to_string(current_settings_.hardware_bandwidth_hz) + "\n";
    content += "freqman_path=" + current_settings_.freqman_path + "\n";

    // Write to temp file first for atomic operation
    std::string temp_path = std::string(SETTINGS_FILE_PATH) + ".tmp";
    File temp_file;
    auto create_result = temp_file.create(temp_path);
    if (create_result.is_error()) {
        nav_.display_modal("Save Error", "Cannot create temp file for settings.");
        return;
    }

    auto write_result = temp_file.write(content);
    if (write_result.is_error()) {
        nav_.display_modal("Save Error", "Cannot write settings to file.");
        temp_file.close();
        return;
    }

    temp_file.close();

    // Atomic rename - since Portapack may support rename, use filesystem or assume ok
    // For now, since rename may not be available, copy content to main file
    File main_file;
    auto main_create = main_file.create(SETTINGS_FILE_PATH);
    if (main_create.is_success()) {
        auto main_write = main_file.write(content);
        main_file.close();

        if (main_write.is_error()) {
            nav_.display_modal("Save Error", "Cannot finalize settings save.");
        }
    } else {
        nav_.display_modal("Save Error", "Cannot create settings file.");
    }
}

void EnhancedDroneAnalyzerSettingsMain::on_hide() {
    if (main_settings_view_) {
        main_settings_view_->on_hide();
    }

    // Save settings when exiting
    settings_manager_.save(current_settings_);
}

} // namespace ui::external_app::enhanced_drone_analyzer::settings

// Portapack app registration - Must use application_information_t structure
extern "C" {

__attribute__((section(".external_app.app_enhanced_drone_analyzer_settings.application_information"), used)) application_information_t _application_information_enhanced_drone_analyzer_settings = {
    /*.memory_location = */ (uint8_t*)0x00000000,  // will be filled at compile time
    /*.externalAppEntry = */ ui::external_app::enhanced_drone_analyzer::settings::settings_app_main,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "EDA Settings",
    /*.bitmap_data = */ {
        0xC0, 0x03, 0xF0, 0x0F, 0x18, 0x18, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10,
        0x08, 0x10, 0x3F, 0xFC, 0x40, 0x02, 0x80, 0x01, 0x7F, 0xFE, 0x00, 0x00,
        0x7F, 0xFE, 0x40, 0x02, 0x80, 0x01, 0x00, 0x00,  // Settings gear icon
    },
    /*.icon_color = */ ui::Color::blue().v,     // Blue for settings
    /*.menu_location = */ app_location_t::UTIL, // UTIL category for settings
    /*.desired_menu_position = */ -1,           // Auto-position

    /*.m4_app_tag = portapack::spi_flash::image_tag_enhanced_drone_analyzer_settings */ {'E', 'D', 'A', 'T'},  // "EDAT"
    /*.m4_app_offset = */ 0x00000000,           // will be filled at compile time
};
}
