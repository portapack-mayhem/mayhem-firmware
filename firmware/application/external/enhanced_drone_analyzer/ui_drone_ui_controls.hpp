// ui_drone_ui_controls.hpp
// UI controls and event handlers for Enhanced Drone Analyzer

#ifndef __UI_DRONE_UI_CONTROLS_HPP__
#define __UI_DRONE_UI_CONTROLS_HPP__

#include "ui.hpp"
#include "ui/ui_button.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

class DroneUIControls {
private:
    Button& button_start_;
    Button& button_stop_;
    Button& button_save_freq_;
    Button& button_load_file_;
    Button& button_mode_;
    Button& button_audio_;
    Button& button_settings_;
    Button& button_advanced_;
    Button& button_frequency_warning_;

public:
    DroneUIControls(Button& start, Button& stop, Button& save, Button& load,
                    Button& mode, Button& audio, Button& settings, Button& advanced,
                    Button& warning);

    // Button event handler setup
    void setup_button_handlers();

    // Add pointer to main view for event delegation
    void set_view_reference(class EnhancedDroneSpectrumAnalyzerView* view) { view_ = view; }

    // Individual button handlers
    void on_start_scan_handler(Button& button);
    void on_stop_scan_handler(Button& button);
    void on_save_frequency_handler(Button& button);
    void on_load_frequency_file_handler(Button& button);
    void on_toggle_mode_handler(Button& button);
    void on_audio_toggle_handler(Button& button);
    void on_open_settings_handler(Button& button);
    void on_advanced_settings_handler(Button& button);
    void on_frequency_warning_handler(Button& button);

private:
    class EnhancedDroneSpectrumAnalyzerView* view_;
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_UI_CONTROLS_HPP__
