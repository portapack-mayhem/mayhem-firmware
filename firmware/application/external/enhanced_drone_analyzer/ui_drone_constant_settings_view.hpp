// ui_drone_constant_settings_view.hpp
// Interactive settings dialog for configurable constants in Enhanced Drone Analyzer
// Allows users to customize frequency ranges, detection parameters, and other app constants

#ifndef __UI_DRONE_CONSTANT_SETTINGS_VIEW_HPP__
#define __UI_DRONE_CONSTANT_SETTINGS_VIEW_HPP__

#include "ui.hpp"
#include "ui/ui_button.hpp"
#include "ui/ui_text.hpp"
#include "ui/ui_number_field.hpp"
#include "portapack.hpp"
#include "ui_drone_config.hpp"  // Include configuration header with constants
#include <functional>

namespace ui::external_app::enhanced_drone_analyzer {

// Forward declaration for settings manager
class ConstantSettingsManager {
public:
    static bool load_settings();
    static bool save_settings();
    static void reset_to_defaults();

    // Static variables holding current working constants
    static rf::Frequency current_min_drone_freq;
    static rf::Frequency current_max_drone_freq;
    static uint8_t current_min_detection_count;
    static int32_t current_hysteresis_margin_db;
    static int16_t current_trend_threshold_db;
    static int32_t current_default_rssi_threshold_db;
    static uint32_t current_scan_interval_ms;
    static uint32_t current_stale_timeout_ms;
};

class ConstantSettingsView : public View {
public:
    ConstantSettingsView(NavigationView& nav);
    ~ConstantSettingsView() = default;

    void focus() override;
    std::string title() const override { return "Constant Settings"; }

private:
    NavigationView& nav_;

    // FREQUENCY RANGE SETTINGS
    Text text_freq_range_title_{ {5, 10}, "Frequency Ranges (MHz)" };
    Text text_min_drone_freq_{ {5, 25}, "Min Drone Freq:" };
    NumberField field_min_drone_freq_{ {85, 22}, 5, {400}, 6000, 240 };
    Text text_max_drone_freq_{ {5, 42}, "Max Drone Freq:" };
    NumberField field_max_drone_freq_{ {85, 39}, 5, {400}, 6000, 6000 };

    // DETECTION PARAMETERS
    Text text_detection_title_{ {5, 60}, "Detection Parameters" };
    Text text_min_detection_{ {5, 75}, "Min Detection Count:" };
    NumberField field_min_detection_count_{ {110, 72}, 2, {1}, 10, 3 };
    Text text_hysteresis_{ {5, 92}, "Hysteresis Margin (dB):" };
    NumberField field_hysteresis_margin_{ {130, 89}, 3, {-20}, 20, 5 };
    Text text_trend_threshold_{ {5, 109}, "Trend Threshold (dB):" };
    NumberField field_trend_threshold_{ {120, 106}, 3, {1}, 10, 3 };
    Text text_default_rssi_{ {5, 126}, "Default RSSI Threshold:" };
    NumberField field_default_rssi_threshold_{ {135, 123}, 4, {-120}, -10, -90 };

    // SCANNING PARAMETERS
    Text text_scanning_title_{ {5, 140}, "Scanning Parameters" };
    Text text_scan_interval_{ {5, 155}, "Scan Interval (ms):" };
    NumberField field_scan_interval_{ {95, 152}, 4, {500}, 5000, 1000 };
    Text text_stale_timeout_{ {5, 172}, "Stale Timeout (ms):" };
    NumberField field_stale_timeout_{ {95, 169}, 5, {5000}, 60000, 30000 };

    // ACTION BUTTONS
    Button button_defaults_{ {5, 160}, "Load Defaults" };
    Button button_save_{ {80, 160}, "Save Settings" };
    Button button_ok_{ {165, 160}, "OK" };

    // STATUS TEXT
    Text text_status_{ {5, 185}, 230, "" };

    // Helper methods
    void initialize_field_values();
    void update_status_text(const char* text);
    bool validate_all_fields();
    void apply_current_values();

    // Button callback methods
    void on_load_defaults();
    void on_save_settings();
    void on_ok_pressed();
    void on_cancel();

    // Validation callbacks for individual fields
    void validate_min_freq(int32_t value);
    void validate_max_freq(int32_t value);
    void validate_detection_params();

    // Prevent copying
    ConstantSettingsView(const ConstantSettingsView&) = delete;
    ConstantSettingsView& operator=(const ConstantSettingsView&) = delete;
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_CONSTANT_SETTINGS_VIEW_HPP__
