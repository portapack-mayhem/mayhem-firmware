// ui_drone_ui.hpp
// UI components and event handling for Enhanced Drone Analyzer
// Module 4 of 4: Handles all user interface elements and interactions

#ifndef __UI_DRONE_UI_HPP__
#define __UI_DRONE_UI_HPP__

#include "ui.hpp"                       // UI base classes
#include "ui_button.hpp"                // Button UI components
#include "ui_text.hpp"                  // Text display components
#include "ui_progress_bar.hpp"          // Progress bar UI
#include "ui_navigation.hpp"            // Navigation system
#include "ui_drone_types.hpp"           // DroneType enum

#include "freqman_db.hpp"               // Frequency database
#include "ui_drone_hardware.hpp"        // Hardware controller
#include "ui_drone_scanner.hpp"         // Scanner controller
#include "ui_drone_audio.hpp"           // Audio controller

#include <memory>
#include <string>

namespace ui::external_app::enhanced_drone_analyzer {

class DroneDisplayController {
public:
    DroneDisplayController(NavigationView& nav);
    ~DroneDisplayController() = default;

    // Core UI components access
    BigFrequencyDisplay& big_display() { return big_display_; }
    ProgressBar& scanning_progress() { return scanning_progress_; }

    // Update UI from scanner state
    void update_detection_display(const DroneScanner& scanner);
    void update_trends_display(size_t approaching, size_t static_count, size_t receding);

    // Navigation access
    NavigationView& nav() { return nav_; }

    // UI state management
    void set_scanning_status(bool active, const std::string& message);
    void set_frequency_display(rf::Frequency freq);

    // Prevent copying
    DroneDisplayController(const DroneDisplayController&) = delete;
    DroneDisplayController& operator=(const DroneDisplayController&) = delete;

private:
    NavigationView& nav_;

    // Core UI components (moved from main class)
    BigFrequencyDisplay big_display_{{0, 24, 240, 32}};
    ProgressBar scanning_progress_{{0, 64, 240, 16}};

    Text text_threat_summary_{{0, 84, 240, 16}, "THREAT: NONE | ▲0 ■0 ▼0"};
    Text text_status_info_{{0, 100, 240, 16}, "Ready - Enhanced Drone Analyzer"};
    Text text_scanner_stats_{{0, 116, 240, 16}, "Detections: 0 | Scan Freq: N/A"};

    Text text_database_info_{{0, 132, 240, 16}, "Database: Ready"};
    Text text_trends_compact_{{0, 148, 240, 16}, "Trends: ▲0 ■0 ▼0"};

    // Additional text fields
    Text text_status_{{0, 164, 240, 16}, "Status: Ready"};
    Text text_threat_level_{{0, 210, 240, 16}, "THREAT: NONE"};
    Text text_info_{{0, 226, 240, 16}, "Enhanced Drone Analyzer v0.2"};
    Text text_copyright_{{0, 242, 240, 16}, "© 2025 M.S. Kuznetsov"};

    // UI utilities moved from main
    Color get_threat_level_color(ThreatLevel level) const;
    const char* get_threat_level_name(ThreatLevel level) const;
};

class DroneUIController {
public:
    DroneUIController(NavigationView& nav,
                     DroneHardwareController& hardware,
                     DroneScanner& scanner,
                     DroneAudioController& audio);
    ~DroneUIController() = default;

    // Main UI view that combines all controllers
    EnhancedDroneSpectrumAnalyzerView* create_main_view();

    // UI event handlers
    void on_start_scan();
    void on_stop_scan();
    void on_toggle_mode();

    // Menu and advanced functions
    void show_menu();
    void on_load_frequency_file();
    void on_save_frequency();
    void on_audio_toggle();
    void on_advanced_settings();

    // UI state management
    bool is_scanning() const { return scanning_active_; }

    // Prevent copying
    DroneUIController(const DroneUIController&) = delete;
    DroneUIController& operator=(const DroneUIController&) = delete;

private:
    NavigationView& nav_;
    DroneHardwareController& hardware_;
    DroneScanner& scanner_;
    DroneAudioController& audio_;

    // UI state
    bool scanning_active_;
    std::unique_ptr<DroneDisplayController> display_controller_;

    // Menu button actions
    void on_manage_frequencies();
    void on_create_new_database();
    void on_frequency_warning();
};

// Main UI view class that ties everything together
class EnhancedDroneSpectrumAnalyzerView : public View {
public:
    EnhancedDroneSpectrumAnalyzerView(NavigationView& nav);
    ~EnhancedDroneSpectrumAnalyzerView() override = default;

    // View interface
    void focus() override;
    std::string title() const override { return "Enhanced Drone Analyzer"; }
    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    void on_show() override;
    void on_hide() override;

private:
    NavigationView& nav_;

    // Controller instances
    std::unique_ptr<DroneHardwareController> hardware_;
    std::unique_ptr<DroneScanner> scanner_;
    std::unique_ptr<DroneAudioController> audio_;
    std::unique_ptr<DroneUIController> ui_controller_;

    // Main UI buttons (simplified)
    Button button_start_{{0, 0}, "START/STOP"};
    Button button_menu_{{120, 0}, "settings"};

    // Scanning thread coordination
    void start_scanning_thread();
    void stop_scanning_thread();

    // UI event handlers
    bool handle_start_stop_button();
    bool handle_menu_button();

    // Prevent copying
    EnhancedDroneSpectrumAnalyzerView(const EnhancedDroneSpectrumAnalyzerView&) = delete;
    EnhancedDroneSpectrumAnalyzerView& operator=(const EnhancedDroneSpectrumAnalyzerView&) = delete;
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_UI_HPP__
