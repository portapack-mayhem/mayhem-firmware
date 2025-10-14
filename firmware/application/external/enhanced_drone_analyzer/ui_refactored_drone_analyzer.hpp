// ui_refactored_drone_analyzer.hpp
// Refactored Enhanced Drone Analyzer main class - now composes multiple components

#ifndef __UI_REFACTORED_DRONE_ANALYZER_HPP__
#define __UI_REFACTORED_DRONE_ANALYZER_HPP__

#include "ui.hpp"
#include "ui/ui_button.hpp"
#include "ui/ui_text.hpp"
#include "ui/ui_progress_bar.hpp"
#include "ui/navigation.hpp"
#include "external_app.hpp"
#include "freqman_db.hpp"

// Include our component headers
#include "ui_drone_analyzer_lifecycle.hpp"
#include "ui_drone_spectrum_scanner.hpp"
#include "ui_drone_tracking.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

class RefactoredDroneSpectrumAnalyzerView : public View {
public:
    RefactoredDroneSpectrumAnalyzerView(NavigationView& nav);

    ~RefactoredDroneSpectrumAnalyzerView() override;

    // UI navigation
    void focus() override;
    std::string title() const override { return "Refactored Drone Analyzer"; }
    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;

    // UI event handlers
    void on_start_scan();
    void on_stop_scan();
    void on_toggle_mode();
    void on_show() override;
    void on_hide() override;

    // UI update methods
    void update_detection_display();
    void update_database_display();
    void handle_scan_error(const char* error_msg);

    // Button handlers with proper signatures
    void on_frequency_warning();
    void on_save_frequency();
    void on_load_frequency_file();

private:
    NavigationView& nav_;

    // Component composition - using our modular components
    DroneAnalyzerLifecycle lifecycle_;
    DroneSpectrumScanner spectrum_scanner_;
    DroneTracker drone_tracker_;

    // UI Controls
    Button button_start_{ {0, 0}, "START" };
    Button button_stop_{ {48, 0}, "STOP" };
    Button button_save_freq_{ {96, 0}, "SAVE" };
    Button button_load_file_{ {0, 32}, "LOAD" };
    Button button_advanced_{ {48, 32}, "ADV" };
    Button button_mode_{ {96, 32}, "MODE" };

    // Display components
    ProgressBar scanning_progress_bar_{ {0, 64, 240, 16} };
    Text text_status_{ {0, 84, 240, 16}, "Status: Ready" };
    Text text_detection_info_{ {0, 104, 240, 48}, "No detections" };
    Text text_database_info_{ {0, 156, 240, 32}, "Database: Not loaded" };
    Text text_scanning_info_{ {0, 192, 240, 32}, "Scanning: Idle" };
    Text text_info_{ {0, 208, 240, 32}, "Refactored Modular Version" };
    Text text_version_{ {0, 244, 240, 16}, "© 2025 M.K." };

    // State variables - simplified
    bool is_real_mode_;
    uint32_t total_detections_;
    ThreatLevel max_detected_threat_;

    // Initialize UI components
    void setup_ui_controls();
    void setup_button_handlers();

    // Mode management
    bool is_demo_mode() const { return !is_real_mode_; }
    void switch_to_demo_mode();
    void switch_to_real_mode();

    // Settings
    void on_open_settings();
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_REFACTORED_DRONE_ANALYZER_HPP__
