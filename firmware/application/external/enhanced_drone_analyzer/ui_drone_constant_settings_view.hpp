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
    // Additional configurable constants
    static float current_rssi_smoothing_alpha;
    static uint32_t current_detection_reset_interval;
    static size_t current_max_displayed_drones;
    static rf::Frequency current_mini_spectrum_min_freq;
    static rf::Frequency current_mini_spectrum_max_freq;
    static uint16_t current_sos_frequency_hz;
    static uint32_t current_beep_duration_ms;
    static float current_rssi_threat_weight;
    static float current_database_threat_weight;

    // Additional hardware and buffer constants
    static size_t current_detection_table_size;
    static size_t current_max_tracked_drones;
    static size_t current_max_database_entries;
    static size_t current_spectrum_buffer_size;
    static rf::Frequency current_ism_center_freq;
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
    Button button_defaults_{ {5, 480}, "Load Defaults" };
    Button button_save_{ {80, 480}, "Save Settings" };
    Button button_ok_{ {165, 480}, "OK" };

    // STATUS TEXT
    Text text_status_{ {5, 500}, 230, "" };

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

    // SPECTRUM & UI PARAMETERS
    Text text_spectrum_title_{ {5, 190}, "Spectrum & UI Parameters" };
    Text text_rssi_smoothing_{ {5, 205}, "RSSI Smoothing Alpha:" };
    NumberField field_rssi_smoothing_{ {125, 202}, 3, {1}, 99, 30 };  // multiplied by 100 for 0.01 precision
    Text text_detection_reset_{ {5, 222}, "Detection Reset Interval:" };
    NumberField field_detection_reset_{ {150, 219}, 3, {10}, 500, 50 };
    Text text_max_drones_{ {5, 239}, "Max Displayed Drones:" };
    NumberField field_max_drones_{ {125, 236}, 2, {1}, 8, 3 };
    Text text_spectrum_min_{ {5, 256}, "Mini Spectrum Min Freq (GHz):" };
    NumberField field_spectrum_min_{ {180, 253}, 1, {1}, 6, 2400 };  // in MHz
    Text text_spectrum_max_{ {5, 273}, "Mini Spectrum Max Freq (GHz):" };
    NumberField field_spectrum_max_{ {180, 270}, 1, {1}, 6, 2500 };  // in MHz

    // AUDIO PARAMETERS
    Text text_audio_title_{ {5, 290}, "Audio Alert Parameters" };
    Text text_sos_freq_{ {5, 305}, "SOS Frequency (Hz):" };
    NumberField field_sos_freq_{ {115, 302}, 4, {500}, 3000, 1500 };
    Text text_beep_duration_{ {5, 322}, "Beep Duration (ms):" };
    NumberField field_beep_duration_{ {125, 319}, 3, {50}, 1000, 200 };

    // THREAT SCORING PARAMETERS
    Text text_threat_title_{ {5, 335}, "Threat Scoring Weights" };
    Text text_rssi_weight_{ {5, 350}, "RSSI Threat Weight:" };
    NumberField field_rssi_weight_{ {115, 347}, 2, {1}, 99, 40 };  // multiplied by 100 for 0.01 precision
    Text text_db_weight_{ {5, 367}, "Database Threat Weight:" };
    NumberField field_db_weight_{ {140, 364}, 2, {1}, 99, 60 };  // multiplied by 100 for 0.01 precision

    // HARDWARE & BUFFER PARAMETERS
    Text text_hardware_title_{ {5, 380}, "Hardware & Buffer Parameters" };
    Text text_detection_table_{ {5, 395}, "Detection Table Size:" };
    NumberField field_detection_table_{ {120, 392}, 4, {128}, 1024, 512 };
    Text text_max_tracked_{ {5, 412}, "Max Tracked Drones:" };
    NumberField field_max_tracked_{ {120, 409}, 2, {1}, 16, 8 };
    Text text_max_db_{ {5, 429}, "Max DB Entries:" };
    NumberField field_max_db_{ {100, 426}, 3, {32}, 128, 64 };
    Text text_spectrum_buffer_{ {5, 446}, "Spectrum Buffer Size:" };
    NumberField field_spectrum_buffer_{ {130, 443}, 4, {128}, 512, 256 };
    Text text_ism_center_{ {5, 463}, "ISM Center Freq (MHz):" };
    NumberField field_ism_center_{ {130, 460}, 4, {400}, 600, 433 };

    // Adjust ACTION BUTTONS lower
    // Button button_defaults_{ {5, 480}, "Load Defaults" };
    // Button button_save_{ {80, 480}, "Save Settings" };
    // Button button_ok_{ {165, 480}, "OK" };

    // STATUS TEXT
    // Text text_status_{ {5, 505}, 230, "" };

    // Prevent copying
    ConstantSettingsView(const ConstantSettingsView&) = delete;
    ConstantSettingsView& operator=(const ConstantSettingsView&) = delete;
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_CONSTANT_SETTINGS_VIEW_HPP__
