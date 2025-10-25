// enhanced_drone_analyzer_scanner_main.cpp - Portapack app entry point for Scanner App
// Integrates scanner functionality with Portapack menu system

#include "ui_scanner_combined.hpp"
#include "app_settings.hpp"
#include "portapack.hpp"
#include "event_m0.hpp"

#ifndef __SCANNER_MAIN_HPP__
#define __SCANNER_MAIN_HPP__

namespace ui::external_app::enhanced_drone_analyzer::scanner {

class EnhancedDroneAnalyzerScannerMain : public View {
public:
    EnhancedDroneAnalyzerScannerMain(NavigationView& nav);
    ~EnhancedDroneAnalyzerScannerMain() override = default;

    void focus() override;
    std::string title() const override { return "EDA Scanner"; }
    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    void on_show() override;
    void on_hide() override;

private:
    NavigationView& nav_;
    bool initialized_ = false;

    // Core scanner components
    std::unique_ptr<DroneScanner> scanner_;
    std::unique_ptr<DroneHardwareController> hardware_;
    std::unique_ptr<DroneDisplayController> display_controller_;
    std::unique_ptr<EnhancedDroneSpectrumAnalyzerView> main_view_;

    // Settings integration
    DroneAnalyzerSettings scanner_settings_;
    app_settings::SettingsManager settings_manager_;

    void initialize_components();
    void load_settings();
    void save_settings();

    // Settings file path constants
    static constexpr const char* SETTINGS_FILE_PATH = "/sdcard/ENHANCED_DRONE_ANALYZER_SCANNER_SETTINGS.txt";
};

// App entry point function for Portapack
void scanner_app_main(NavigationView& nav) {
    nav.push<EnhancedDroneAnalyzerScannerMain>();
}

} // namespace ui::external_app::enhanced_drone_analyzer::scanner

#endif // __SCANNER_MAIN_HPP__

// Implementation
namespace ui::external_app::enhanced_drone_analyzer::scanner {

EnhancedDroneAnalyzerScannerMain::EnhancedDroneAnalyzerScannerMain(NavigationView& nav)
    : View(), nav_(nav)
{
}

void EnhancedDroneAnalyzerScannerMain::focus() {
    if (!initialized_) {
        initialize_components();
        initialized_ = true;
    }
    View::focus();
}

void EnhancedDroneAnalyzerScannerMain::initialize_components() {
    load_settings();

    // Initialize scanner app components
    hardware_ = std::make_unique<DroneHardwareController>(scanner_settings_.spectrum_mode);
    scanner_ = std::make_unique<DroneScanner>();
    display_controller_ = std::make_unique<DroneDisplayController>(nav_);

    // Create main scanner view
    main_view_ = std::make_unique<EnhancedDroneSpectrumAnalyzerView>(nav_);
}

void EnhancedDroneAnalyzerScannerMain::load_settings() {
    if (!settings_manager_.load(scanner_settings_)) {
        // Load defaults if file not found
        settings_manager_.reset_to_defaults(scanner_settings_);
    }

    // Try to import settings from Settings app TXT file
    std::ifstream settings_file(DroneAnalyzerSettings::SETTINGS_FILE_PATH);
    if (settings_file.is_open()) {
        std::string line;
        while (std::getline(settings_file, line)) {
            size_t equals_pos = line.find('=');
            if (equals_pos != std::string::npos) {
                std::string key = line.substr(0, equals_pos);
                std::string value = line.substr(equals_pos + 1);

                // Import relevant settings from Settings app
                if (key == "spectrum_mode" && value == "MEDIUM") {
                    scanner_settings_.spectrum_mode = SpectrumMode::MEDIUM;
                } else if (key == "scan_interval_ms") {
                    scanner_settings_.scan_interval_ms = std::stoul(value);
                } else if (key == "rssi_threshold_db") {
                    scanner_settings_.rssi_threshold_db = std::stoi(value);
                } else if (key == "enable_audio_alerts") {
                    scanner_settings_.enable_audio_alerts = (value == "true");
                } else if (key == "audio_alert_frequency_hz") {
                    scanner_settings_.audio_alert_frequency_hz = std::stoul(value);
                } else if (key == "hardware_bandwidth_hz") {
                    scanner_settings_.hardware_bandwidth_hz = std::stoul(value);
                } else if (key == "enable_real_hardware") {
                    scanner_settings_.enable_real_hardware = (value == "true");
                }
            }
        }
    }
}

void EnhancedDroneAnalyzerScannerMain::save_settings() {
    settings_manager_.save(scanner_settings_);

    // Export settings to TXT file for Settings app to read
    std::ofstream out_file(DroneAnalyzerSettings::SETTINGS_FILE_PATH, std::ios::app);
    if (out_file.is_open()) {
        out_file << "\n--- Scanner App Settings Export ---\n";
        out_file << "scanner_active_state=" << (scanner_ && scanner_->is_scanning_active() ? "true" : "false") << "\n";
        out_file << "current_scanning_frequency=";
        if (scanner_) {
            out_file << scanner_->get_current_scanning_frequency();
        }
        out_file << "\n";
        out_file << "detected_drone_count=";
        if (scanner_) {
            out_file << scanner_->getTrackedDroneCount();
        }
        out_file << "\n";
        out_file << "--- End Export ---\n";
    }
}

void EnhancedDroneAnalyzerScannerMain::paint(Painter& painter) {
    View::paint(painter);

    if (!initialized_) {
        painter.fill_rectangle({0, 0, screen_width, screen_height}, Color::black());
        painter.draw_string({screen_width / 2 - 50, screen_height / 2 - 10}, Theme::getInstance()->fg_light->foreground, "Loading EDA Scanner...");
        return;
    }

    if (main_view_) {
        return; // Let main view handle painting
    }

    // Fallback display
    painter.fill_rectangle({0, 0, screen_width, screen_height}, Color::black());
    painter.draw_string({10, 10}, Theme::getInstance()->fg_white, "EDA Scanner App");
    painter.draw_string({10, 30}, Theme::getInstance()->fg_white, "Status: Ready");
}

bool EnhancedDroneAnalyzerScannerMain::on_key(const KeyEvent key) {
    if (main_view_) {
        return main_view_->on_key(key);
    }

    switch (key) {
        case KeyEvent::Back:
            nav_.pop();
            return true;
        default:
            return false;
    }
}

bool EnhancedDroneAnalyzerScannerMain::on_touch(const TouchEvent event) {
    if (main_view_) {
        return main_view_->on_touch(event);
    }
    return false;
}

void EnhancedDroneAnalyzerScannerMain::on_show() {
    save_settings(); // Export current state to TXT file
    if (main_view_) {
        main_view_->on_show();
    }
}

void EnhancedDroneAnalyzerScannerMain::on_hide() {
    if (main_view_) {
        main_view_->on_hide();
    }
    save_settings(); // Save settings on exit
}

} // namespace ui::external_app::enhanced_drone_analyzer::scanner

// Portapack app registration
extern "C" {
    __attribute__((section(".external_apps")))
    const struct {
        const char* name;
        void (*main)(NavigationView& nav);
    } scanner_app_definition = {
        "Enhanced Drone Analyzer Scanner",
        ui::external_app::enhanced_drone_analyzer::scanner::scanner_app_main
    };
}
