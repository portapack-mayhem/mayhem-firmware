// ui_drone_settings_view.hpp - Settings dialog for Enhanced Drone Analyzer

#ifndef __UI_DRONE_SETTINGS_VIEW_HPP__
#define __UI_DRONE_SETTINGS_VIEW_HPP__

#include "ui.hpp"
#include "ui_tabview.hpp"
#include "ui_button.hpp"
#include "ui_checkbox.hpp"
#include "ui_number_field.hpp"
#include "ui_options_field.hpp"
#include "ui_text.hpp"

#include "ui_drone_settings.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

// Settings dialog view with tabbed interface
class DroneSettingsView : public View {
public:
    DroneSettingsView(NavigationView& nav, DroneAnalyzerSettings& settings);

    std::string title() const override { return "Drone Analyzer Settings"; };
    void focus() override { tab_view.focus(); }

private:
    NavigationView& nav_;
    DroneAnalyzerSettings& settings_;
    DroneAnalyzerSettings backup_settings_;  // For cancel functionality

    // Main UI components
    TabView tab_view{
        {0, 0}, {screen_width, screen_height - 3 * 16},
        {"Spectrum", "Detection", "Audio", "Display"}
    };

    Button button_save{
        {UI_POS_X_CENTER(6), screen_height - 2 * 16, 6 * 8, 2 * 16},
        "Save"
    };

    Button button_cancel{
        {UI_POS_X_CENTER(6) + 6 * 8 + 4, screen_height - 2 * 16, 6 * 8, 2 * 16},
        "Cancel"
    };

    Button button_reset{
        {UI_POS_X_CENTER(6), screen_height - 16, 6 * 8, 16},
        "Reset"
    };

    // ===== SPECTRUM TAB =====
    Text text_spectrum_title{{8, 2 * 16}, "Spectrum Analysis", Theme::getInstance()->fg_light->foreground};

    Text text_spectrum_mode_label{{8, 3 * 16 + 8}, "Mode:", Theme::getInstance()->fg_light->foreground};
    OptionsField field_spectrum_mode{
        {8 * 5, 3 * 16 + 8}, 10,
        {
            {"ULTRA NARROW", static_cast<int>(SpectrumMode::ULTRA_NARROW)},
            {"NARROW", static_cast<int>(SpectrumMode::NARROW)},
            {"MEDIUM", static_cast<int>(SpectrumMode::MEDIUM)},
            {"WIDE", static_cast<int>(SpectrumMode::WIDE)},
            {"ULTRA WIDE", static_cast<int>(SpectrumMode::ULTRA_WIDE)}
        }
    };

    Text text_scan_interval_label{{8, 4 * 16 + 8}, "Scan Int.:", Theme::getInstance()->fg_light->foreground};
    NumberField field_scan_interval{
        {8 * 9, 4 * 16 + 8}, 4, {200, 5000}, 100, ' '
    };

    Text text_rssi_threshold_label{{8, 5 * 16 + 8}, "RSSI Thresh.:", Theme::getInstance()->fg_light->foreground};
    NumberField field_rssi_threshold{
        {8 * 12, 5 * 16 + 8}, 3, {-120, -20}, 1, ' '
    };

    Text text_rssi_alpha_label{{8, 6 * 16 + 8}, "RSSI Smooth:", Theme::getInstance()->fg_light->foreground};
    NumberField field_rssi_alpha{
        {8 * 12, 6 * 16 + 8}, 2, {1, 9}, 1, ' '  // Values 0.1 to 0.9
    };

    // ===== DETECTION TAB =====
    Text text_detection_title{{8, 2 * 16}, "Signal Detection", Theme::getInstance()->fg_light->foreground};

    Text text_min_detections_label{{8, 3 * 16 + 8}, "Min Detections:", Theme::getInstance()->fg_light->foreground};
    NumberField field_min_detections{
        {8 * 14, 3 * 16 + 8}, 2, {1, 10}, 1, ' '
    };

    Text text_hysteresis_label{{8, 4 * 16 + 8}, "Hysteresis:", Theme::getInstance()->fg_light->foreground};
    NumberField field_hysteresis{
        {8 * 11, 4 * 16 + 8}, 2, {1, 20}, 1, ' '
    };

    Text text_trend_threshold_label{{8, 5 * 16 + 8}, "Trend Thresh.:", Theme::getInstance()->fg_light->foreground};
    NumberField field_trend_threshold{
        {8 * 14, 5 * 16 + 8}, 2, {1, 15}, 1, ' '
    };

    Text text_reset_interval_label{{8, 6 * 16 + 8}, "Reset Int.:", Theme::getInstance()->fg_light->foreground};
    NumberField field_reset_interval{
        {8 * 10, 6 * 16 + 8}, 3, {25, 500}, 25, ' '
    };

    // ===== AUDIO TAB =====
    Text text_audio_title{{8, 2 * 16}, "Audio Alerts", Theme::getInstance()->fg_light->foreground};

    Text text_alert_freq_label{{8, 3 * 16 + 8}, "Alert Freq.:", Theme::getInstance()->fg_light->foreground};
    NumberField field_alert_freq{
        {8 * 11, 3 * 16 + 8}, 4, {400, 3000}, 50, ' '
    };

    Text text_beep_duration_label{{8, 4 * 16 + 8}, "Beep Duration:", Theme::getInstance()->fg_light->foreground};
    NumberField field_beep_duration{
        {8 * 14, 4 * 16 + 8}, 3, {50, 1000}, 50, ' '
    };

    Text text_alert_squelch_label{{8, 5 * 16 + 8}, "Alert Squelch:", Theme::getInstance()->fg_light->foreground};
    NumberField field_alert_squelch{
        {8 * 14, 5 * 16 + 8}, 3, {-100, -20}, 5, ' '
    };

    // ===== DISPLAY TAB =====
    Text text_display_title{{8, 2 * 16}, "Display Settings", Theme::getInstance()->fg_light->foreground};

    Checkbox check_show_graph{
        {8, 3 * 16 + 8},
        16,
        "Show frequency chart"
    };

    Checkbox check_show_trends{
        {8, 4 * 16 + 8},
        16,
        "Show movement trends"
    };

    Text text_max_drones_label{{8, 5 * 16 + 8}, "Max display drones:", Theme::getInstance()->fg_light->foreground};
    NumberField field_max_drones{
        {8 * 17, 5 * 16 + 8}, 1, {1, 5}, 1, ' '
    };

    Text text_color_scheme_label{{8, 6 * 16 + 8}, "Color scheme:", Theme::getInstance()->fg_light->foreground};
    OptionsField field_color_scheme{
        {8 * 13, 6 * 16 + 8}, 15,
        {
            {"Standard", 0},
            {"High contrast", 1}
        }
    };

    // Event handlers
    void setup_ui_elements();
    void load_settings_to_ui();
    void save_ui_to_settings();
    void validate_settings();
    void show_validation_error(const std::string& message);
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_SETTINGS_VIEW_HPP__
