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
#include "ui_drone_audio_settings_about.hpp" // About modal

#include "freqman_db.hpp"               // Frequency database
#include "ui_drone_hardware.hpp"        // Hardware controller
#include "ui_drone_scanner.hpp"         // Scanner controller
#include "ui_drone_audio.hpp"           // Audio controller

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

    // Text fields for displaying detected drones (Recon text pattern)
    Text text_drone_1{{0, MINI_SPECTRUM_Y_START + 8, screen_width/2, 16}, ""};
    Text text_drone_2{{screen_width/2, MINI_SPECTRUM_Y_START + 8, screen_width/2, 16}, ""};
    Text text_drone_3{{0, MINI_SPECTRUM_Y_START + 24, screen_width/2, 16}, ""};

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
    const std::vector<DisplayDroneEntry>& get_current_drones() const { return displayed_drones_; }

    // Mini waterfall spectrum (Search/Looking Glass pattern)
    static constexpr size_t MINI_SPECTRUM_WIDTH = 120;   // Half screen width
    static constexpr size_t MINI_SPECTRUM_HEIGHT = 8;    // 8 lines height
    static constexpr uint32_t MINI_SPECTRUM_Y_START = 180; // Below text status

    void initialize_mini_spectrum();
    void process_mini_spectrum_data(const ChannelSpectrum& spectrum);
    void render_mini_spectrum();
    void highlight_threat_zones_in_spectrum(const std::vector<DisplayDroneEntry>& drones);
    size_t frequency_to_spectrum_bin(rf::Frequency freq_hz) const;

private:
    // UI state for drone display
    std::vector<DisplayDroneEntry> detected_drones_;
    std::vector<DisplayDroneEntry> displayed_drones_;  // Top 3 by RSSI

    // Mini waterfall spectrum data (Search pattern)
    std::vector<std::vector<Color>> mini_spectrum_data_;   // MINI_SPECTRUM_HEIGHT x MINI_SPECTRUM_WIDTH
    std::vector<uint8_t> spectrum_power_levels_;           // Raw power for color mapping
    Gradient spectrum_gradient_;                            // Color gradient for spectrum (Search style)
    ChannelSpectrumFIFO* spectrum_fifo_ = nullptr;          // FIFO for spectrum data
    uint32_t spectrum_line_index_ = 0;                      // Current line in waterfall

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
