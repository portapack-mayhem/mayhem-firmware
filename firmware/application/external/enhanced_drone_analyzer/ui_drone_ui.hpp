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
#include "ui_drone_audio.hpp"           // Audio controller
#include "ui_drone_audio_settings_about.hpp" // About modal
#include "ui_drone_settings.hpp"             // Settings structures

#include "freqman_db.hpp"               // Frequency database (standard pattern)
#include "ui_drone_hardware.hpp"        // Hardware controller
#include "ui_drone_scanner.hpp"         // Scanner controller

#include <app_settings.hpp>             // SettingsManager

#include <memory>
#include <string>
#include <vector>

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

    // UI: Detected drones list with RSSI sorting (Search pattern)
    static constexpr size_t MAX_DISPLAYED_DRONES = 3;
    static constexpr size_t MAX_TRACKED_DRONES = 8;  // Maximum tracked before memory overflow

    // Text fields for displaying detected drones (PHASE 4 FIX: Compile-time bounds)
    // SPECTRUM_Y_TOP + 8: Below spectrum area, compile-time safe positioning
    Text text_drone_1{{0, SPECTRUM_Y_TOP + 8, screen_width/2, 16}, ""};
    Text text_drone_2{{screen_width/2, SPECTRUM_Y_TOP + 8, screen_width/2, 16}, ""};
    Text text_drone_3{{0, SPECTRUM_Y_TOP + 24, screen_width/2, 16}, ""};

    // Display format constants
    static constexpr const char* DRONE_DISPLAY_FORMAT = "%s %s %-4ddB %c";
    struct DisplayDroneEntry {
        rf::Frequency frequency;
        DroneType type;
        ThreatLevel threat;
        int32_t rssi;
        systime_t last_seen;
        std::string type_name;
        Color display_color;
    };

    void add_detected_drone(rf::Frequency freq, DroneType type, ThreatLevel threat, int32_t rssi);
    void update_drones_display();
    void sort_drones_by_rssi();
    const std::array<DisplayDroneEntry, MAX_DISPLAYED_DRONES>& get_current_drones() const { return displayed_drones_; }

    // Mini waterfall spectrum (PHASE 1 FIX: Looking Glass scroll area pattern)
    static constexpr size_t MINI_SPECTRUM_WIDTH = 240;   // Full screen width for Looking Glass compatibility
    static constexpr size_t MINI_SPECTRUM_HEIGHT = 16;   // Increased to 16 for better readability (FIX #2)
    static constexpr uint32_t SPECTRUM_Y_TOP = 70;      // Enhanced resolution: lowered for taller waterfall

    void initialize_mini_spectrum();
    void process_mini_spectrum_data(const ChannelSpectrum& spectrum);
    bool process_bins(uint8_t* power_level);
    void render_mini_spectrum();
    void highlight_threat_zones_in_spectrum(const std::array<DisplayDroneEntry, MAX_DISPLAYED_DRONES>& drones);
    size_t frequency_to_spectrum_bin(rf::Frequency freq_hz) const;

    // EMBEDDED SAFETY: Spectrum processing utilities
    void clear_spectrum_buffers();
    bool validate_spectrum_data() const;
    size_t get_safe_spectrum_index(size_t x, size_t y) const;

    // SPECTRUM CONFIGURATION SYSTEM: Replace hard-coded ranges with configurable
    struct SpectrumConfig {
        rf::Frequency min_freq = 2400000000ULL;  // 2.4GHz default
        rf::Frequency max_freq = 2500000000ULL;  // 2.5GHz default
        uint32_t bandwidth = 24000000;            // 24MHz default
        uint32_t sampling_rate = 24000000;        // 24Msps default
    };

    void set_spectrum_range(rf::Frequency min_freq, rf::Frequency max_freq);
    SpectrumConfig spectrum_config_;

private:
    // UI state for drone display (EMBEDDED FIX: Vector for safety, array for fixed display)
    std::vector<DisplayDroneEntry> detected_drones_;  // Dynamic tracking
    std::array<DisplayDroneEntry, MAX_DISPLAYED_DRONES> displayed_drones_;  // Top 3 by RSSI

    // PHASE 3 FIX: Memory optimization - exact size arrays following Looking Glass
    std::array<Color, screen_width> spectrum_row;           // One line buffer for scrolling (PHASE 1 FIX)
    std::array<uint8_t, MINI_SPECTRUM_WIDTH> spectrum_power_levels_; // Raw power for color mapping

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

    // UI: Detected drones list with RSSI sorting (Search pattern)
    NavigationView& nav_;

    // Spectrum message handlers (Search pattern)
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
    void on_open_settings();  // New: Open settings dialog
    void on_open_constant_settings();  // New: Open constant settings dialog

    // UI state management
    bool is_scanning() const { return scanning_active_; }

    // Settings access
    DroneAnalyzerSettings& settings() { return settings_; }
    const DroneAnalyzerSettings& settings() const { return settings_; }

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

    // Settings management
    DroneAnalyzerSettings settings_;

    // Settings persistence - renamed to distinguish from runtime settings
    app_settings::SettingsManager constant_settings_manager_{
        "rx_drone_analyzer"sv,
        app_settings::Mode::RX,
        {
            // Spectrum settings
            {"spectrum_mode"sv, reinterpret_cast<uint32_t*>(&settings_.spectrum_mode)},

            {"scan_interval"sv, &settings_.spectrum.min_scan_interval_ms},
            {"stale_timeout"sv, &settings_.spectrum.stale_timeout_ms},
            {"rssi_threshold"sv, &settings_.spectrum.default_rssi_threshold},
            {"rssi_alpha"sv, &settings_.spectrum.rssi_smoothing_alpha},

            // Detection settings
            {"min_detections"sv, &settings_.detection.min_detection_count},
            {"hysteresis_db"sv, &settings_.detection.hysteresis_margin_db},
            {"trend_db"sv, &settings_.detection.trend_threshold_db},
            {"reset_interval"sv, &settings_.detection.detection_reset_interval},

            // Audio settings
            {"alert_freq"sv, &settings_.audio.alert_frequency_hz},
            {"beep_duration"sv, &settings_.audio.beep_duration_ms},
            {"alert_squelch"sv, &settings_.audio.alert_squelch_db},

            // Display settings
            {"show_graph"sv, &settings_.display.show_rssi_graph},
            {"show_trends"sv, &settings_.display.show_trends},
            {"max_display"sv, &settings_.display.max_display_drones},
            {"color_scheme"sv, &settings_.display.color_scheme},
        }};

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

    // Controller instances - UPDATED WITH SCANNING COORDINATOR
    std::unique_ptr<DroneHardwareController> hardware_;
    std::unique_ptr<DroneScanner> scanner_;
    std::unique_ptr<DroneAudioController> audio_;
    std::unique_ptr<DroneUIController> ui_controller_;

    // SCANNING COORDINATOR: Fixes the broken scanning architecture
    std::unique_ptr<ScanningCoordinator> scanning_coordinator_;
    std::unique_ptr<DroneDisplayController> display_controller_; // Now separate

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
