// ui_settings_combined.hpp - Unified header for Enhanced Drone Analyzer Settings App
// Combines: ui_drone_common_types.hpp, ui_drone_config.hpp, ui_drone_settings_complete.hpp, ui_minimal_drone_analyzer.hpp
// Created during migration: Provides unified settings management interface

#ifndef __UI_SETTINGS_COMBINED_HPP__
#define __UI_SETTINGS_COMBINED_HPP__

// ===========================================
// PART 1: COMMON TYPES (Shared with Scanner App)
// ===========================================

#include <cstdint>
#include <string>
#include <vector>

using Frequency = uint64_t;

enum class ThreatLevel {
    NONE,
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

enum class DroneType {
    UNKNOWN,
    MAVIC,
    PHANTOM,
    DJI_MINI,
    PARROT_ANAFI,
    PARROT_BEBOP,
    PX4_DRONE,
    MILITARY_DRONE
};

enum class MovementTrend {
    UNKNOWN,
    STATIC,
    APPROACHING,
    RECEDING
};

enum class SpectrumMode {
    NARROW,
    MEDIUM,
    WIDE,
    ULTRA_WIDE
};

struct TrackedDrone {
    uint32_t frequency;
    uint8_t drone_type;
    uint8_t threat_level;
    uint8_t update_count;
    systime_t last_seen;
};

struct DisplayDroneEntry {
    Frequency frequency;
    DroneType type;
    ThreatLevel threat;
    int32_t rssi;
    systime_t last_seen;
    std::string type_name;
    Color display_color;
    MovementTrend trend;
};

struct DronePreset {
    std::string display_name;
    std::string name_template;
    Frequency frequency_hz;
    ThreatLevel threat_level;
    DroneType drone_type;

    bool is_valid() const {
        return !display_name.empty() && frequency_hz > 0;
    }
};

static constexpr size_t MAX_TRACKED_DRONES = 8;
static constexpr size_t MAX_DISPLAYED_DRONES = 3;
static constexpr size_t MINI_SPECTRUM_WIDTH = 200;
static constexpr size_t MINI_SPECTRUM_HEIGHT = 24;
static constexpr uint32_t MIN_HARDWARE_FREQ = 1'000'000;
static constexpr uint32_t MAX_HARDWARE_FREQ = 6'000'000'000;
static constexpr uint32_t WIDEBAND_DEFAULT_MIN = 2'400'000'000ULL;
static constexpr uint32_t WIDEBAND_DEFAULT_MAX = 2'500'000'000ULL;
static constexpr uint32_t WIDEBAND_SLICE_WIDTH = 25'000'000;
static constexpr uint32_t WIDEBAND_MAX_SLICES = 20;
static constexpr int32_t DEFAULT_RSSI_THRESHOLD_DB = -90;
static constexpr int32_t WIDEBAND_RSSI_THRESHOLD_DB = -95;
static constexpr int32_t WIDEBAND_DYNAMIC_THRESHOLD_OFFSET_DB = 5;
static constexpr uint8_t MIN_DETECTION_COUNT = 3;
static constexpr int32_t HYSTERESIS_MARGIN_DB = 5;

using freqman_entry = struct {
    uint64_t frequency_a;
    uint64_t frequency_b;
    uint8_t type;
    uint16_t modulation;
    uint16_t bandwidth;
    uint16_t step;
    std::string description;
    std::string tonal;
};

static constexpr size_t DETECTION_TABLE_SIZE = 256;

struct WidebandSlice {
    Frequency center_frequency;
    size_t index;
};

struct WidebandScanData {
    Frequency min_freq;
    Frequency max_freq;
    size_t slices_nb;
    WidebandSlice slices[20];
    size_t slice_counter;

    void reset() {
        min_freq = WIDEBAND_DEFAULT_MIN;
        max_freq = WIDEBAND_DEFAULT_MAX;
        slices_nb = 0;
        slice_counter = 0;
    }
};

struct DetectionLogEntry {
    uint32_t timestamp;
    uint32_t frequency_hz;
    int32_t rssi_db;
    ThreatLevel threat_level;
    DroneType drone_type;
    uint8_t detection_count;
    float confidence_score;
};

// ===========================================
// PART 2: CONFIGURATION STRUCTURES (from ui_drone_config.hpp)
// ===========================================

namespace ui::external_app::enhanced_drone_analyzer {

struct DroneAnalyzerSettings {
    // Core scanning parameters
    SpectrumMode spectrum_mode = SpectrumMode::MEDIUM;
    uint32_t scan_interval_ms = 1000;
    int32_t rssi_threshold_db = DEFAULT_RSSI_THRESHOLD_DB;
    bool enable_audio_alerts = true;
    uint16_t audio_alert_frequency_hz = 800;
    uint32_t audio_alert_duration_ms = 500;

    // Wideband scanning
    bool enable_wideband_scanning = false;
    Frequency wideband_min_freq_hz = WIDEBAND_DEFAULT_MIN;
    Frequency wideband_max_freq_hz = WIDEBAND_DEFAULT_MAX;

    // Frequency range limits
    Frequency min_frequency_hz = 2400000000ULL;  // 2.4GHz
    Frequency max_frequency_hz = 2500000000ULL;  // 2.5GHz

    // Display options
    bool show_detailed_info = true;
    bool auto_save_logs = true;
    std::string log_file_path = "/eda_logs";

    // File paths
    std::string freqman_path = "DRONES";
    std::string settings_file_path = "/settings/scanner_config.txt";

    // Hardware settings
    uint32_t hardware_bandwidth_hz = 24000000;
    bool enable_real_hardware = true;
    bool demo_mode = false;
};

enum class Language {
    ENGLISH,
    RUSSIAN,
    GERMAN,
    FRENCH
};

class DroneAnalyzerSettingsManager {
public:
    static bool load(DroneAnalyzerSettings& settings);
    static bool save(const DroneAnalyzerSettings& settings);
    static void reset_to_defaults(DroneAnalyzerSettings& settings);
    static bool validate(const DroneAnalyzerSettings& settings);
    static std::string serialize(const DroneAnalyzerSettings& settings);
    static bool deserialize(DroneAnalyzerSettings& settings, const std::string& data);

    static void set_language(Language lang) { current_language_ = lang; }
    static Language get_language() { return current_language_; }
    static const char* translate(const std::string& key);

    // Default translations (English)
    static const std::map<std::string, const char*> translations_english;
    static const char* get_translation(const std::string& key);

private:
    static Language current_language_;
};

static DroneAnalyzerSettingsManager& get_settings_manager();

struct ConfigData {
    SpectrumMode spectrum_mode = SpectrumMode::MEDIUM;
    int32_t rssi_threshold_db = DEFAULT_RSSI_THRESHOLD_DB;
    uint32_t scan_interval_ms = 1000;
    bool enable_audio_alerts = true;
    std::string freqman_path = "DRONES";
};

class ScannerConfig {
public:
    explicit ScannerConfig(ConfigData config = {});
    ~ScannerConfig() = default;

    bool load_from_file(const std::string& filepath);
    bool save_to_file(const std::string& filepath) const;

    const ConfigData& get_data() const { return config_data_; }
    ConfigData& get_data() { return config_data_; }

    void set_frequency_range(uint32_t min_hz, uint32_t max_hz);
    void set_rssi_threshold(int32_t threshold);
    void set_scan_interval(uint32_t interval_ms);
    void set_audio_alerts(bool enabled);
    void set_freqman_path(const std::string& path);

    // Legacy compatibility methods
    void set_scanning_mode(const std::string& mode);
    bool is_valid() const;

    ScannerConfig(const ScannerConfig&) = delete;
    ScannerConfig& operator=(const ScannerConfig&) = delete;

private:
    ConfigData config_data_;
};

using PresetMenuView = std::function<void(const DronePreset&)>;
using FilteredPresetMenuView = std::function<void(const DronePreset&, const std::vector<DronePreset>&)>;

class DroneFrequencyPresets {
public:
    static const std::vector<DronePreset>& get_all_presets();
    static std::vector<DroneType> get_available_types();
    static std::string get_type_display_name(DroneType type);

    static std::vector<DronePreset> get_presets_of_type(const std::vector<DronePreset>& all_presets, DroneType type);
    static bool apply_preset(ScannerConfig& config, const DronePreset& preset);

private:
    static const std::vector<DronePreset> standard_presets_;
};

class DronePresetSelector {
public:
    static void show_preset_menu(NavigationView& nav, PresetMenuView callback);
    static void show_type_filtered_presets(NavigationView& nav, DroneType type, FilteredPresetMenuView callback);
    static PresetMenuView create_config_updater(ScannerConfig& config_to_update);
};

class SimpleDroneValidation {
public:
    static bool validate_frequency_range(Frequency freq_hz);
    static bool validate_rssi_signal(int32_t rssi_db, ThreatLevel threat);
    static ThreatLevel classify_signal_strength(int32_t rssi_db);
    static DroneType identify_drone_type(Frequency freq_hz, int32_t rssi_db);
    static bool validate_drone_detection(Frequency freq_hz, int32_t rssi_db,
                                       DroneType type, ThreatLevel threat);
};

struct DroneFrequencyEntry {
    Frequency frequency_hz;
    DroneType drone_type;
    ThreatLevel threat_level;
    int32_t rssi_threshold_db;
    uint32_t bandwidth_hz;
    const char* description;

    DroneFrequencyEntry(Frequency freq, DroneType type, ThreatLevel threat,
                       int32_t rssi_thresh, uint32_t bw_hz, const char* desc);

    bool is_valid() const;
};

using DroneFrequencyDatabase = std::vector<DroneFrequencyEntry>;

// ===========================================
// PART 3: SETTINGS UI CLASSES (from ui_drone_settings_complete.hpp)
// ===========================================

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_tabview.hpp"
#include "app_settings.hpp"
#include <string>
#include <sstream>

struct DroneAudioSettings {
    bool audio_enabled = true;
    uint16_t alert_frequency_hz = 800;
    uint32_t alert_duration_ms = 500;
    uint8_t volume_level = 50;
    bool repeat_alerts = false;
};

class AudioSettingsView : public View {
public:
    explicit AudioSettingsView(NavigationView& nav);
    ~AudioSettingsView() = default;

    void focus() override;
    std::string title() const override { return "Audio Settings"; }

    AudioSettingsView(const AudioSettingsView&) = delete;
    AudioSettingsView& operator=(const AudioSettingsView&) = delete;

private:
    NavigationView& nav_;
    DroneAudioSettings audio_settings_;

    Checkbox checkbox_audio_enabled_{{8, 8, 16, 16}};
    Text text_audio_enabled_{{28, 8, 200, 16}, "Enable Audio Alerts"};

    NumberField number_alert_frequency_{{8, 32, 120, 24}, 1, "Alert Frequency (Hz)"};
    NumberField number_alert_duration_{{8, 64, 120, 24}, 1, "Duration (ms)"};
    NumberField number_volume_{{8, 96, 120, 24}, 1, "Volume (%)"};

    Checkbox checkbox_repeat_{{8, 128, 16, 16}};
    Text text_repeat_{{28, 128, 200, 16}, "Repeat Alerts"};

    Button button_save_{{screen_width - 120, screen_height - 32, 120, 32}, "Save Settings"};

    void load_current_settings();
    void save_current_settings();
    void update_ui_from_settings();
    void update_settings_from_ui();

    void on_save_settings();
};

class HardwareSettingsView : public View {
public:
    explicit HardwareSettingsView(NavigationView& nav);
    ~HardwareSettingsView() = default;

    void focus() override;
    std::string title() const override { return "Hardware Settings"; }

    HardwareSettingsView(const HardwareSettingsView&) = delete;
    HardwareSettingsView& operator=(const HardwareSettingsView&) = delete;

private:
    NavigationView& nav_;

    Checkbox checkbox_real_hardware_{{8, 8, 16, 16}};
    Text text_real_hardware_{{28, 8, 224, 16}, "Use Real Hardware (Disable for Demo)"};

    OptionsField field_spectrum_mode_{{8, 32, 160, 24},
                                     {"Narrow", "Medium", "Wide", "Ultra Wide"}, 1, "Spectrum Mode"};

    NumberField number_bandwidth_{{8, 64, 120, 24}, 1, "Hardware Bandwidth (Hz)"};
    NumberField number_min_freq_{{8, 96, 120, 24}, 1, "Min Frequency (Hz)"};
    NumberField number_max_freq_{{8, 128, 120, 24}, 1, "Max Frequency (Hz)"};

    Button button_save_{{screen_width - 120, screen_height - 32, 120, 32}, "Save Settings"};

    void load_current_settings();
    void save_current_settings();
    void update_ui_from_settings();
    void update_settings_from_ui();

    void on_save_settings();
};

class ScanningSettingsView : public View {
public:
    explicit ScanningSettingsView(NavigationView& nav);
    ~ScanningSettingsView() = default;

    void focus() override;
    std::string title() const override { return "Scanning Settings"; }

    ScanningSettingsView(const ScanningSettingsView&) = delete;
    ScanningSettingsView& operator=(const ScanningSettingsView&) = delete;

private:
    NavigationView& nav_;

    OptionsField field_scanning_mode_{{8, 8, 160, 24},
                                     {"Database Scan", "Wideband Monitor", "Hybrid Discovery"},
                                     0, "Scanning Mode"};

    NumberField number_scan_interval_{{8, 32, 120, 24}, 1, "Scan Interval (ms)"};
    NumberField number_rssi_threshold_{{8, 64, 120, 24}, 1, "RSSI Threshold (dB)"};

    Checkbox checkbox_wideband_{{8, 96, 16, 16}};
    Text text_wideband_{{28, 96, 200, 16}, "Enable Wideband Scanning"};

    Button button_presets_{{8, 128, 120, 32}, "Frequency Presets"};
    Button button_save_{{screen_width - 120, screen_height - 32, 120, 32}, "Save Settings"};

    void load_current_settings();
    void save_current_settings();
    void update_ui_from_settings();
    void update_settings_from_ui();

    void on_save_settings();
    void on_show_presets();
    void on_wideband_enabled_changed();
};

class SettingsTabbedView : public TabView {
public:
    explicit SettingsTabbedView(NavigationView& nav);
    ~SettingsTabbedView() = default;

    void focus() override;

    SettingsTabbedView(const SettingsTabbedView&) = delete;
    SettingsTabbedView& operator=(const SettingsTabbedView&) = delete;

private:
    NavigationView& nav_;

    AudioSettingsView audio_view_;
    HardwareSettingsView hardware_view_;
    ScanningSettingsView scanning_view_;

    void update_tab_content();
};

// ===========================================
// PART 4: MINIMAL UI FRAMEWORK (from ui_minimal_drone_analyzer.hpp)
// ===========================================

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

class DroneAnalyzerSettingsView : public View {
public:
    explicit DroneAnalyzerSettingsView(NavigationView& nav);
    ~DroneAnalyzerSettingsView() override = default;

    void focus() override;
    std::string title() const override { return "EDA Settings"; }
    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    void on_show() override;
    void on_hide() override;

    DroneAnalyzerSettingsView(const DroneAnalyzerSettingsView&) = delete;
    DroneAnalyzerSettingsView& operator=(const DroneAnalyzerSettingsView&) = delete;

private:
    NavigationView& nav_;

    Button button_audio_settings_{{8, 32, 120, 32}, "Audio Settings"};
    Button button_hardware_settings_{{136, 32, 120, 32}, "Hardware Settings"};
    Button button_scanning_settings_{{8, 72, 120, 32}, "Scanning Settings"};
    Button button_advanced_settings_{{136, 72, 120, 32}, "Advanced Settings"};

    Button button_load_defaults_{{8, 112, 120, 32}, "Load Defaults"};
    Button button_save_all_{{136, 112, 120, 32}, "Save All Settings"};

    Text text_title_{{8, 8, screen_width - 16, 16}, "Enhanced Drone Analyzer Settings"};

    void on_button_audio_settings();
    void on_button_hardware_settings();
    void on_button_scanning_settings();
    void on_button_advanced_settings();
    void on_button_load_defaults();
    void on_button_save_all();

    void show_audio_settings();
    void show_hardware_settings();
    void show_scanning_settings();
    void show_advanced_settings();
    void load_default_settings();
    void save_all_settings();

    DroneAnalyzerSettings current_settings_;
    bool settings_modified_ = false;
};

class DroneAnalyzerSettingsMainView : public View {
public:
    explicit DroneAnalyzerSettingsMainView(NavigationView& nav);
    ~DroneAnalyzerSettingsMainView() override = default;

    void focus() override;
    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    void on_show() override;
    void on_hide() override;

    std::string title() const override { return "EDA Settings App"; }

    DroneAnalyzerSettingsMainView(const DroneAnalyzerSettingsMainView&) = delete;
    DroneAnalyzerSettingsMainView& operator=(const DroneAnalyzerSettingsMainView&) = delete;

private:
    NavigationView& nav_;
    std::vector<Button> menu_buttons_;

    void setup_menu_buttons();
    void on_menu_button_pressed(size_t button_index);
    void refresh_display();

    DroneAnalyzerSettings app_settings_;
    bool initialized_ = false;
};

class LoadingView : public View {
public:
    LoadingView(NavigationView& nav, const std::string& loading_text = "Loading...");
    ~LoadingView() = default;

    void focus() override;
    std::string title() const override { return "Loading"; }
    void paint(Painter& painter) override;
    void on_show() override;
    void on_hide() override;

    LoadingView(const LoadingView&) = delete;
    LoadingView& operator=(const LoadingView&) = delete;

private:
    NavigationView& nav_;
    Text loading_text_1_;
    Text loading_text_2_;
    systime_t start_time_ = 0;
    static constexpr uint32_t MIN_LOADING_TIME_MS = 500;
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_SETTINGS_COMBINED_HPP__
