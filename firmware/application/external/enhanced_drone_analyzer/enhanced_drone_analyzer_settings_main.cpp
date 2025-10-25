// enhanced_drone_analyzer_settings_main.cpp - Portapack app entry point for Settings App
// Integrates settings management with Portapack menu system

#include "ui_settings_combined.hpp"
#include "app_settings.hpp"
#include "portapack.hpp"

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
    // Try to read scanner status from TXT file
    std::ifstream settings_file(SETTINGS_FILE_PATH);
    if (settings_file.is_open()) {
        std::string line;
        bool in_scanner_section = false;

        while (std::getline(settings_file, line)) {
            if (line.find("--- Scanner App Settings Export ---") != std::string::npos) {
                in_scanner_section = true;
                continue;
            }
            if (line.find("--- End Export ---") != std::string::npos) {
                in_scanner_section = false;
                continue;
            }

            if (in_scanner_section) {
                size_t equals_pos = line.find('=');
                if (equals_pos != std::string::npos) {
                    std::string key = line.substr(0, equals_pos);
                    std::string value = line.substr(equals_pos + 1);

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
    }
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

    // Save current settings to TXT file for scanner app to read
    std::ofstream out_file(SETTINGS_FILE_PATH);
    if (out_file.is_open()) {
        out_file << "# Enhanced Drone Analyzer Settings\n";
        out_file << "# Generated by Settings App\n\n";

        out_file << "spectrum_mode=";
        switch (current_settings_.spectrum_mode) {
            case SpectrumMode::NARROW: out_file << "NARROW"; break;
            case SpectrumMode::MEDIUM: out_file << "MEDIUM"; break;
            case SpectrumMode::WIDE: out_file << "WIDE"; break;
            case SpectrumMode::ULTRA_WIDE: out_file << "ULTRA_WIDE"; break;
        }
        out_file << "\n";

        out_file << "scan_interval_ms=" << current_settings_.scan_interval_ms << "\n";
        out_file << "rssi_threshold_db=" << current_settings_.rssi_threshold_db << "\n";
        out_file << "enable_audio_alerts=" << (current_settings_.enable_audio_alerts ? "true" : "false") << "\n";
        out_file << "audio_alert_frequency_hz=" << current_settings_.audio_alert_frequency_hz << "\n";
        out_file << "audio_alert_duration_ms=" << current_settings_.audio_alert_duration_ms << "\n";
        out_file << "enable_real_hardware=" << (current_settings_.enable_real_hardware ? "true" : "false") << "\n";
        out_file << "demo_mode=" << (current_settings_.demo_mode ? "true" : "false") << "\n";
        out_file << "hardware_bandwidth_hz=" << current_settings_.hardware_bandwidth_hz << "\n";
        out_file << "freqman_path=" << current_settings_.freqman_path << "\n";

        out_file.flush();
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
