#ifndef __UI_DRONE_UI_HPP__
#define __UI_DRONE_UI_HPP__
#include "ui.hpp"                       // UI base classes
#include "ui_button.hpp"                // Button UI components
#include "ui_text.hpp"                  // Text display components
#include "ui_progress_bar.hpp"          // Progress bar UI
#include "ui_navigation.hpp"            // Navigation system
#include "ui_options_field.hpp"         // Options field for scanning modes
#include "ui_drone_config.hpp"               // Consolidated types and enums
#include "ui_drone_audio.hpp"                // AudioManager definition - Migrated Session 5
#include "ui_drone_settings_complete.hpp"             // Settings structures
#include "ui_minimal_drone_analyzer.hpp"     // For LoadingScreenView main view push
#include "freqman_db.hpp"               // Frequency database (standard pattern)
#include "ui_drone_hardware.hpp"        // Hardware controller
#include "ui_drone_scanner.hpp"         // Scanner controller
#include <app_settings.hpp>             // SettingsManager
#include <memory>
#include <string>
#include <vector>
#include <mutex>
namespace ui::external_app::enhanced_drone_analyzer {
// PHASE 1: Smart Threat Header Component for Modern UI Design
class SmartThreatHeader : public View {
public:
    explicit SmartThreatHeader(Rect parent_rect = {0, 0, screen_width, 48});
    ~SmartThreatHeader() = default;

    // Core update function - single entry point for all threat data
    void update(ThreatLevel max_threat, size_t approaching, size_t static_count,
                size_t receding, rf::Frequency current_freq, bool is_scanning);

    // Individual setters for granular control
    void set_max_threat(ThreatLevel threat);
    void set_movement_counts(size_t approaching, size_t static_count, size_t receding);
    void set_current_frequency(rf::Frequency freq);
    void set_scanning_state(bool is_scanning);

    // Visual customization
    void set_color_scheme(bool use_dark_theme);

    SmartThreatHeader(const SmartThreatHeader&) = delete;
    SmartThreatHeader& operator=(const SmartThreatHeader&) = delete;

private:
    // UI Components - modular design
    ProgressBar threat_progress_bar_ {{0, 0, screen_width, 16}};
    Text threat_status_main_ {{0, 20, screen_width, 16}, "THREAT: LOW | ▲0 ■0 ▼0"};
    Text threat_frequency_ {{0, 38, screen_width, 16}, "2400.0MHz SCANNING"};

    // Color mapping functions
    Color get_threat_bar_color(ThreatLevel level) const;
    Color get_threat_text_color(ThreatLevel level) const;
    std::string get_threat_icon_text(ThreatLevel level) const;

    void paint(Painter& painter) override;

    // State variables for rendering
    ThreatLevel current_threat_ = ThreatLevel::NONE;
    bool is_scanning_ = false;
    rf::Frequency current_freq_ = 2400000000ULL; // Default 2.4GHz
    size_t approaching_count_ = 0;
    size_t static_count_ = 0;
    size_t receding_count_ = 0;
};

class DroneDisplayController {
public:
    explicit DroneDisplayController(NavigationView& nav);
    ~DroneDisplayController() = default;
    BigFrequencyDisplay& big_display() { return big_display_; }
    ProgressBar& scanning_progress() { return scanning_progress_; }
    Text& text_threat_summary() { return text_threat_summary_; }
    Text& text_status_info() { return text_status_info_; }
    Text& text_scanner_stats() { return text_scanner_stats_; }
    Text& text_trends_compact() { return text_trends_compact_; }
    Text& text_drone_1() { return text_drone_1; }
    Text& text_drone_2() { return text_drone_2; }
    Text& text_drone_3() { return text_drone_3; }
    void update_detection_display(const DroneScanner& scanner);
    void update_trends_display(size_t approaching, size_t static_count, size_t receding);
    NavigationView& nav() { return nav_; }
    void set_scanning_status(bool active, const std::string& message);
    void set_frequency_display(rf::Frequency freq);
    DroneDisplayController(const DroneDisplayController&) = delete;
    DroneDisplayController& operator=(const DroneDisplayController&) = delete;
    static constexpr size_t MAX_DISPLAYED_DRONES = 3;
    static constexpr size_t MAX_TRACKED_DRONES = 8;  // Maximum tracked before memory overflow
    Text text_drone_1{{0, SPECTRUM_Y_TOP + 8, screen_width/2, 16}, ""};
    Text text_drone_2{{screen_width/2, SPECTRUM_Y_TOP + 8, screen_width/2, 16}, ""};
    Text text_drone_3{{0, SPECTRUM_Y_TOP + 24, screen_width/2, 16}, ""};
    static constexpr const char* DRONE_DISPLAY_FORMAT = "%s %s %-4ddB %c";
    struct DisplayDroneEntry {
        rf::Frequency frequency;
        DroneType type;
        ThreatLevel threat;
        int32_t rssi;
        systime_t last_seen;
        std::string type_name;
        Color display_color;
    void add_detected_drone(rf::Frequency freq, DroneType type, ThreatLevel threat, int32_t rssi);
    };
    void add_detected_drone(rf::Frequency freq, DroneType type, ThreatLevel threat, int32_t rssi);
    void update_drones_display(const DroneScanner& scanner);
    void sort_drones_by_rssi();
    static constexpr size_t MINI_SPECTRUM_WIDTH = 200;   // Optimized width for details
    static constexpr size_t MINI_SPECTRUM_HEIGHT = 24;   // Increased height for better readability
    static constexpr uint32_t SPECTRUM_Y_TOP = 70;      // Enhanced resolution: lowered for taller waterfall
    void initialize_mini_spectrum();
    void process_mini_spectrum_data(const ChannelSpectrum& spectrum);
    bool process_bins(uint8_t* power_level);
    void render_mini_spectrum();
    void highlight_threat_zones_in_spectrum(const std::array<DisplayDroneEntry, MAX_DISPLAYED_DRONES>& drones);
    size_t frequency_to_spectrum_bin(rf::Frequency freq_hz) const;
    void clear_spectrum_buffers();
    bool validate_spectrum_data() const;
    size_t get_safe_spectrum_index(size_t x, size_t y) const;
    struct SpectrumConfig {
        rf::Frequency min_freq = 2400000000ULL;  // 2.4GHz default
        rf::Frequency max_freq = 2500000000ULL;  // 2.5GHz default
        uint32_t bandwidth = 24000000;            // 24MHz default
        uint32_t sampling_rate = 24000000;        // 24Msps default
    };
    void set_spectrum_range(rf::Frequency min_freq, rf::Frequency max_freq);
    SpectrumConfig spectrum_config_;
private:
    std::vector<DisplayDroneEntry> detected_drones_;  // Dynamic tracking
    std::array<DisplayDroneEntry, MAX_DISPLAYED_DRONES> displayed_drones_;  // Top 3 by RSSI
    std::array<Color, screen_width> spectrum_row;           // One line buffer for scrolling (PHASE 1 FIX)
    std::array<uint8_t, 200> spectrum_power_levels_;         // Raw power for color mapping (adjusted for new width)
    struct ThreatBin { size_t bin; ThreatLevel threat; };
    std::array<ThreatBin, MAX_DISPLAYED_DRONES> threat_bins_; // Bin indexes and threat levels for overlay
    size_t threat_bins_count_ = 0;                          // Number of active threat bins
    Gradient spectrum_gradient_;                             // Color gradient for spectrum (Search style)
    ChannelSpectrumFIFO* spectrum_fifo_ = nullptr;           // FIFO for spectrum data
    size_t pixel_index = 0;                                  // Current pixel in row (PHASE 1 FIX)
    uint32_t bins_hz_size = 0;                               // Hz accumulator (PHASE 2 FIX)
    uint32_t each_bin_size = 100000;                         // Hz per bin (configurable)
    uint8_t* powerlevel = nullptr;                            // Current power working variable
    uint8_t min_color_power = 0;                             // Filter threshold
    const uint8_t ignore_dc = 4;                             // DC spike bins to ignore
    std::mutex spectrum_access_mutex_;                       // Thread safety for spectrum rendering
    NavigationView& nav_;
    MessageHandlerRegistration message_handler_spectrum_config_{
        Message::ID::ChannelSpectrumConfig,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const ChannelSpectrumConfigMessage*>(p);
            this->spectrum_fifo_ = message.fifo;
        }};
    MessageHandlerRegistration message_handler_spectrum_frame_sync_{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            if (this->spectrum_fifo_) {
                ChannelSpectrum channel_spectrum;
                while (spectrum_fifo_->out(channel_spectrum)) {
                    this->process_mini_spectrum_data(channel_spectrum);
                }
                this->render_mini_spectrum();
            }
        }};
    BigFrequencyDisplay big_display_{{0, 24, 240, 32}};
    ProgressBar scanning_progress_{{0, 64, 240, 16}};
    Text text_threat_summary_{{0, 84, 240, 16}, "THREAT: NONE | ▲0 ■0 ▼0"};
    Text text_status_info_{{0, 100, 240, 16}, "Ready - Enhanced Drone Analyzer"};
    Text text_scanner_stats_{{0, 116, 240, 16}, "Detections: 0 | Scan Freq: N/A"};
    Text text_database_info_{{0, 132, 240, 16}, "Database: Ready"};
    Text text_trends_compact_{{0, 148, 240, 16}, "Trends: ▲0 ■0 ▼0"};
    Text text_status_{{0, 164, 240, 16}, "Status: Ready"};
    Text text_threat_level_{{0, 210, 240, 16}, "THREAT: NONE"};
    Text text_info_{{0, 226, 240, 16}, "Enhanced Drone Analyzer v0.2"};
    Text text_copyright_{{0, 242, 240, 16}, "© 2025 M.S. Kuznetsov"};
    Color get_threat_level_color(ThreatLevel level) const;
    const char* get_threat_level_name(ThreatLevel level) const;
};
class DroneUIController {
public:
    DroneUIController(NavigationView& nav,
                     DroneHardwareController& hardware,
                     DroneScanner& scanner,
                     AudioManager& audio_mgr);
    ~DroneUIController() = default;
    EnhancedDroneSpectrumAnalyzerView* create_main_view();
    void on_start_scan();
    void on_stop_scan();
    void on_toggle_mode();
    void show_menu();
    void on_load_frequency_file();
    void on_save_frequency();
    void on_toggle_audio_simple();  // PHASE 1: RESTORE Audio Enable Toggle
    void on_audio_toggle();
    void on_advanced_settings();
    void on_open_settings();  // New: Open settings dialog
    void on_open_constant_settings();  // New: Open constant settings dialog
    bool is_scanning() const { return scanning_active_; }
    DroneAnalyzerSettings& settings() { return settings_; }
    const DroneAnalyzerSettings& settings() const { return settings_; }
    DroneUIController(const DroneUIController&) = delete;
    DroneUIController& operator=(const DroneUIController&) = delete;
private:
    NavigationView& nav_;
    DroneHardwareController& hardware_;
    DroneScanner& scanner_;
    AudioManager& audio_mgr_;
    bool scanning_active_;
    std::unique_ptr<DroneDisplayController> display_controller_;
    DroneAnalyzerSettings settings_;
    app_settings::SettingsManager constant_settings_manager_{
        "rx_drone_analyzer"sv,
        app_settings::Mode::RX,
        {
            {"spectrum_mode"sv, reinterpret_cast<uint32_t*>(&settings_.spectrum_mode)},
            {"scan_interval"sv, &settings_.spectrum.min_scan_interval_ms},
            {"stale_timeout"sv, &settings_.spectrum.stale_timeout_ms},
            {"rssi_threshold"sv, &settings_.spectrum.default_rssi_threshold},
            {"rssi_alpha"sv, &settings_.spectrum.rssi_smoothing_alpha},
            {"min_detections"sv, &settings_.detection.min_detection_count},
            {"hysteresis_db"sv, &settings_.detection.hysteresis_margin_db},
            {"trend_db"sv, &settings_.detection.trend_threshold_db},
            {"reset_interval"sv, &settings_.detection.detection_reset_interval},
            {"alert_freq"sv, &settings_.audio.alert_frequency_hz},
            {"beep_duration"sv, &settings_.audio.beep_duration_ms},
            {"alert_squelch"sv, &settings_.audio.alert_squelch_db},
            {"show_graph"sv, &settings_.display.show_rssi_graph},
            {"show_trends"sv, &settings_.display.show_trends},
            {"max_display"sv, &settings_.display.max_display_drones},
            {"color_scheme"sv, &settings_.display.color_scheme},
        }};
    void on_manage_frequencies();
    void on_create_new_database();
    void on_frequency_warning();
    void show_system_status();
    void show_performance_stats();
    void show_debug_info();
    void select_spectrum_mode(SpectrumMode mode);
    void on_spectrum_range_config();
    void on_add_preset_quick();
    void on_hardware_control_menu();
    void on_save_settings();
    void on_load_settings();
};
class EnhancedDroneSpectrumAnalyzerView : public View {
public:
    explicit EnhancedDroneSpectrumAnalyzerView(NavigationView& nav);
    ~EnhancedDroneSpectrumAnalyzerView() override = default;
    void focus() override;
    std::string title() const override { return "Enhanced Drone Analyzer"; }
    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    void on_show() override;
    void on_hide() override;
private:
    NavigationView& nav_;
    std::unique_ptr<DroneHardwareController> hardware_;
    std::unique_ptr<DroneScanner> scanner_;
    std::unique_ptr<AudioManager> audio_mgr_;
    std::unique_ptr<DroneUIController> ui_controller_;
    std::unique_ptr<ScanningCoordinator> scanning_coordinator_;
    std::unique_ptr<DroneDisplayController> display_controller_; // Now separate
    Button button_start_{{0, 0}, "START/STOP"};
    Button button_menu_{{120, 0}, "settings"};
    OptionsField field_scanning_mode_{{80, 190, 160, 24}, OptionsField::StringOptions{"Database Scan", "Wideband Monitor", "Hybrid Discovery"}, "Mode"};
    void start_scanning_thread();
    void stop_scanning_thread();
    bool handle_start_stop_button();
    bool handle_menu_button();
    EnhancedDroneSpectrumAnalyzerView(const EnhancedDroneSpectrumAnalyzerView&) = delete;
    EnhancedDroneSpectrumAnalyzerView& operator=(const EnhancedDroneSpectrumAnalyzerView&) = delete;
};
} // namespace ui::external_app::enhanced_drone_analyzer
#endif // __UI_DRONE_UI_HPP__
