// ui_scanner_combined.hpp - Unified header for Enhanced Drone Analyzer Scanner App
// Combines: ui_drone_common_types.hpp, ui_drone_scanner.hpp, ui_drone_hardware.hpp, ui_drone_ui.hpp
// Created during migration: Split monolithic app into focused Scanner application

#ifndef __UI_SCANNER_COMBINED_HPP__
#define __UI_SCANNER_COMBINED_HPP__

// ===========================================
// PART 1: COMMON TYPES (from ui_drone_common_types.hpp)
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

class TrackedDrone {
public:
    TrackedDrone() : frequency(0), drone_type(static_cast<uint8_t>(DroneType::UNKNOWN)),
                     threat_level(static_cast<uint8_t>(ThreatLevel::NONE)), update_count(0), last_seen(0) {}

    void add_rssi(int16_t rssi, systime_t timestamp) {
        // Store RSSI history for trend calculation
        rssi_history_[history_index_] = rssi;
        timestamp_history_[history_index_] = timestamp;
        history_index_ = (history_index_ + 1) % MAX_HISTORY;

        if (last_seen < timestamp) {
            last_seen = timestamp;
            update_count++;
        }
    }

    MovementTrend get_trend() const {
        if (update_count < 2) return MovementTrend::UNKNOWN;

        // Analyze RSSI trend over last few samples
        int32_t recent_rssi = 0, older_rssi = 0;
        size_t recent_count = 0, older_count = 0;

        for (size_t i = 0; i < MAX_HISTORY; i++) {
            if (i < history_index_) {
                recent_rssi += rssi_history_[i];
                recent_count++;
            } else if (i > history_index_ && i < MAX_HISTORY - 1) {
                older_rssi += rssi_history_[i];
                older_count++;
            }
        }

        if (recent_count == 0 || older_count == 0) return MovementTrend::UNKNOWN;

        int32_t avg_recent = recent_rssi / recent_count;
        int32_t avg_older = older_rssi / older_count;
        int32_t diff_dB = avg_recent - avg_older;

        if (diff_dB > 5) return MovementTrend::APPROACHING;
        if (diff_dB < -5) return MovementTrend::RECEDING;
        return MovementTrend::STATIC;
    }

    uint32_t frequency;
    uint8_t drone_type;
    uint8_t threat_level;
    uint8_t update_count;
    systime_t last_seen;

private:
    static constexpr size_t MAX_HISTORY = 8;
    int16_t rssi_history_[MAX_HISTORY] = {0};
    systime_t timestamp_history_[MAX_HISTORY] = {0};
    size_t history_index_ = 0;

    TrackedDrone(const TrackedDrone&) = delete;
    TrackedDrone& operator=(const TrackedDrone&) = delete;
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
static constexpr uint64_t MAX_HARDWARE_FREQ = 6'000'000'000ULL;
static constexpr uint32_t WIDEBAND_DEFAULT_MIN = 2'400'000'000ULL;
static constexpr uint32_t WIDEBAND_DEFAULT_MAX = 2'500'000'000ULL;
static constexpr uint32_t WIDEBAND_SLICE_WIDTH = 25'000'000;
static constexpr uint32_t WIDEBAND_MAX_SLICES = 20;
static constexpr int32_t DEFAULT_RSSI_THRESHOLD_DB = -90;
static constexpr int32_t WIDEBAND_RSSI_THRESHOLD_DB = -95;
static constexpr int32_t WIDEBAND_DYNAMIC_THRESHOLD_OFFSET_DB = 5;
static constexpr uint8_t MIN_DETECTION_COUNT = 3;
static constexpr int32_t HYSTERESIS_MARGIN_DB = 5;

// freqman_entry defined in freqman_db.hpp - removed conflicting typedef

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
// PART 2: CONFIGURATION STRUCTURES (Shared with Settings App)
// ===========================================

struct DroneAnalyzerSettings {
    // Core scanning parameters
    SpectrumMode spectrum_mode = SpectrumMode::MEDIUM;
    uint32_t scan_interval_ms = 1000;
    int32_t rssi_threshold_db = DEFAULT_RSSI_THRESHOLD_DB;
    bool enable_audio_alerts = true;
    uint16_t audio_alert_frequency_hz = 800;
    uint32_t audio_alert_duration_ms = 500;

    // Hardware settings
    uint32_t hardware_bandwidth_hz = 24000000;
    bool enable_real_hardware = true;
    bool demo_mode = false;
};

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

    void set_scanning_mode(const std::string& mode);
    bool is_valid() const;

    ScannerConfig(const ScannerConfig&) = delete;
    ScannerConfig& operator=(const ScannerConfig&) = delete;

private:
    ConfigData config_data_;
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

// ===========================================
// PART 3: SCANNER CLASSES (from ui_drone_scanner.hpp)
// ===========================================

#include "freqman_db.hpp"
#include <ch.h>
#include <vector>
#include <array>

namespace ui::external_app::enhanced_drone_analyzer {

class DetectionRingBuffer {
public:
    DetectionRingBuffer();
    ~DetectionRingBuffer() = default;

    void update_detection(size_t frequency_hash, uint8_t detection_count, int32_t rssi_value);
    uint8_t get_detection_count(size_t frequency_hash) const;
    int32_t get_rssi_value(size_t frequency_hash) const;
    void clear();

    DetectionRingBuffer(const DetectionRingBuffer&) = delete;
    DetectionRingBuffer& operator=(const DetectionRingBuffer&) = delete;

private:
    uint8_t detection_counts_[DETECTION_TABLE_SIZE] = {0};
    int32_t rssi_values_[DETECTION_TABLE_SIZE] = {0};
} __attribute__((aligned(4)));

extern DetectionRingBuffer global_detection_ring;
extern DetectionRingBuffer& local_detection_ring;

class DroneScanner {
public:
    enum class ScanningMode {
        DATABASE,
        WIDEBAND_CONTINUOUS,
        HYBRID
    };

    DroneScanner();
    ~DroneScanner();

    void start_scanning();
    void stop_scanning();
    bool is_scanning_active() const { return scanning_active_; }
    bool load_frequency_database();
    size_t get_database_size() const;

    ScanningMode get_scanning_mode() const { return scanning_mode_; }
    std::string scanning_mode_name() const;
    void set_scanning_mode(ScanningMode mode);

    void switch_to_real_mode();
    void switch_to_demo_mode();

    void perform_scan_cycle(DroneHardwareController& hardware);
    void process_rssi_detection(const freqman_entry& entry, int32_t rssi);
    void update_tracked_drone(DroneType type, Frequency frequency, int32_t rssi, ThreatLevel threat_level);
    void remove_stale_drones();

    Frequency get_current_scanning_frequency() const;
    ThreatLevel get_max_detected_threat() const { return max_detected_threat_; }
    const TrackedDrone& getTrackedDrone(size_t index) const;
    void handle_scan_error(const char* error_msg);

    std::string get_session_summary() const;

    DroneScanner(const DroneScanner&) = delete;
    DroneScanner& operator=(const DroneScanner&) = delete;

    class DroneDetectionLogger {
    public:
        DroneDetectionLogger();
        ~DroneDetectionLogger();
        DroneDetectionLogger(const DroneDetectionLogger&) = delete;
        DroneDetectionLogger& operator=(const DroneDetectionLogger&) = delete;

        void start_session();
        void end_session();
        bool log_detection(const DetectionLogEntry& entry);
        std::filesystem::path get_log_filename() const;

    private:
        LogFile csv_log_;
        bool session_active_;
        uint32_t session_start_;
        uint32_t logged_count_;
        bool header_written_;

        bool ensure_csv_header();
        std::string format_csv_entry(const DetectionLogEntry& entry);
        std::string format_session_summary(size_t scan_cycles, size_t total_detections) const;
        std::filesystem::path generate_log_filename() const;
    };

private:
    static msg_t scanning_thread_function(void* arg);
    msg_t scanning_thread();

    void initialize_database_and_scanner();
    void cleanup_database_and_scanner();
    void scan_init_from_loaded_frequencies();

    void perform_database_scan_cycle(DroneHardwareController& hardware);
    void perform_wideband_scan_cycle(DroneHardwareController& hardware);
    void perform_hybrid_scan_cycle(DroneHardwareController& hardware);

    void update_tracking_counts();
    void update_trends_compact_display();
    bool validate_detection_simple(int32_t rssi_db, ThreatLevel threat);
    Frequency get_current_radio_frequency() const;

    Thread* scanning_thread_ = nullptr;
    static constexpr uint32_t SCAN_THREAD_STACK_SIZE = 2048;
    bool scanning_active_ = false;

    FreqmanDB freq_db_;
    size_t current_db_index_ = 0;
    Frequency last_scanned_frequency_ = 0;

    uint32_t scan_cycles_ = 0;
    uint32_t total_detections_ = 0;

    ScanningMode scanning_mode_ = ScanningMode::DATABASE;
    bool is_real_mode_ = true;

    std::array<TrackedDrone, MAX_TRACKED_DRONES> tracked_drones_;
    size_t tracked_drones_count_ = 0;

    size_t approaching_count_ = 0;
    size_t receding_count_ = 0;
    size_t static_count_ = 0;

    ThreatLevel max_detected_threat_ = ThreatLevel::NONE;
    int32_t last_valid_rssi_ = -120;

    static const uint8_t DETECTION_DELAY = 3;
    WidebandScanData wideband_scan_data_;
    FreqmanDB drone_database_;
    DroneDetectionLogger detection_logger_;
};

// ===========================================
// PART 3: HARDWARE CLASSES (from ui_drone_hardware.hpp)
// ===========================================

#include "radio_state.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "radio.hpp"
#include "message.hpp"
#include "irq_controls.hpp"

class DroneHardwareController {
public:
    explicit DroneHardwareController(SpectrumMode mode = SpectrumMode::MEDIUM);
    ~DroneHardwareController();

    void initialize_hardware();
    void on_hardware_show();
    void on_hardware_hide();
    void shutdown_hardware();

    void set_spectrum_mode(SpectrumMode mode);
    uint32_t get_spectrum_bandwidth() const;
    void set_spectrum_bandwidth(uint32_t bandwidth_hz);
    Frequency get_spectrum_center_frequency() const;
    void set_spectrum_center_frequency(Frequency center_freq);

    bool tune_to_frequency(Frequency frequency_hz);
    void start_spectrum_streaming();
    void stop_spectrum_streaming();
    int32_t get_real_rssi_from_hardware(Frequency target_frequency);

    void handle_channel_spectrum_config(const ChannelSpectrumConfigMessage* const message);
    void handle_channel_spectrum(const ChannelSpectrum& spectrum);
    void process_channel_spectrum_data(const ChannelSpectrum& spectrum);

    DroneHardwareController(const DroneHardwareController&) = delete;
    DroneHardwareController& operator=(const DroneHardwareController&) = delete;

private:
    void initialize_radio_state();
    void initialize_spectrum_collector();
    void cleanup_spectrum_collector();

    int32_t get_configured_sampling_rate() const;
    int32_t get_configured_bandwidth() const;

    MessageHandlerRegistration message_handler_spectrum_config_;
    MessageHandlerRegistration message_handler_frame_sync_;

    SpectrumMode spectrum_mode_;
    Frequency center_frequency_;
    uint32_t bandwidth_hz_;
    RxRadioState radio_state_;
    ChannelSpectrumFIFO* fifo_ = nullptr;
    bool spectrum_streaming_active_ = false;
    int32_t last_valid_rssi_ = -120;
};

// ===========================================
// PART 4: UI CLASSES (from ui_drone_ui.hpp)
// ===========================================

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "app_settings.hpp"
#include "freqman_db.hpp"
#include <memory>

class SmartThreatHeader : public View {
public:
    explicit SmartThreatHeader(Rect parent_rect = {0, 0, screen_width, 48});
    ~SmartThreatHeader() = default;

    void update(ThreatLevel max_threat, size_t approaching, size_t static_count,
                size_t receding, Frequency current_freq, bool is_scanning);
    void set_max_threat(ThreatLevel threat);
    void set_movement_counts(size_t approaching, size_t static_count, size_t receding);
    void set_current_frequency(Frequency freq);
    void set_scanning_state(bool is_scanning);
    void set_color_scheme(bool use_dark_theme);

    SmartThreatHeader(const SmartThreatHeader&) = delete;
    SmartThreatHeader& operator=(const SmartThreatHeader&) = delete;

private:
    ProgressBar threat_progress_bar_ {{0, 0, screen_width, 16}};
    Text threat_status_main_ {{0, 20, screen_width, 16}, "THREAT: LOW | ▲0 ■0 ▼0"};
    Text threat_frequency_ {{0, 38, screen_width, 16}, "2400.0MHz SCANNING"};

    Color get_threat_bar_color(ThreatLevel level) const;
    Color get_threat_text_color(ThreatLevel level) const;
    std::string get_threat_icon_text(ThreatLevel level) const;

    void paint(Painter& painter) override;

    ThreatLevel current_threat_ = ThreatLevel::NONE;
    bool is_scanning_ = false;
    Frequency current_freq_ = 2400000000ULL;
    size_t approaching_count_ = 0;
    size_t static_count_ = 0;
    size_t receding_count_ = 0;
};

class ThreatCard : public View {
public:
    explicit ThreatCard(size_t card_index = 0, Rect parent_rect = {0, 0, screen_width, 24});
    ~ThreatCard() = default;

    void update_card(const DisplayDroneEntry& drone);
    void clear_card();
    std::string render_compact() const;
    Color get_card_bg_color() const;
    Color get_card_text_color() const;

    ThreatCard(const ThreatCard&) = delete;
    ThreatCard& operator=(const ThreatCard&) = delete;

private:
    size_t card_index_;
    Text card_text_ {{0, 2, screen_width, 20}, ""};
    bool is_active_ = false;

    Frequency frequency_ = 0;
    ThreatLevel threat_ = ThreatLevel::NONE;
    MovementTrend trend_ = MovementTrend::STATIC;
    int32_t rssi_ = -120;
    uint8_t detection_count_ = 0;
    systime_t last_seen_ = 0;
    std::string threat_name_ = "UNKNOWN";

    void paint(Painter& painter) override;
};

enum class DisplayMode { SCANNING, ALERT, NORMAL };

class ConsoleStatusBar : public View {
public:
    explicit ConsoleStatusBar(size_t bar_index = 0, Rect parent_rect = {0, 0, screen_width, 16});
    ~ConsoleStatusBar() = default;

    void update_scanning_progress(uint32_t progress_percent, uint32_t total_cycles = 0, uint32_t detections = 0);
    void update_alert_status(ThreatLevel threat, size_t total_drones, const std::string& alert_msg);
    void update_normal_status(const std::string& primary, const std::string& secondary);
    void set_display_mode(DisplayMode mode);

    ConsoleStatusBar(const ConsoleStatusBar&) = delete;
    ConsoleStatusBar& operator=(const ConsoleStatusBar&) = delete;

private:
    size_t bar_index_;
    DisplayMode mode_ = DisplayMode::NORMAL;

    Text progress_text_  {{0, 1, screen_width, 16}, ""};
    Text alert_text_     {{0, 1, screen_width, 16}, ""};
    Text normal_text_    {{0, 1, screen_width, 16}, ""};

    void paint(Painter& painter) override;
};

class DroneDisplayController {
public:
    explicit DroneDisplayController(NavigationView& nav);
    ~DroneDisplayController() = default;

    BigFrequency& big_display() { return big_display_; }
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
    void set_scanning_status(bool active, const std::string& message);
    void set_frequency_display(Frequency freq);
    void add_detected_drone(Frequency freq, DroneType type, ThreatLevel threat, int32_t rssi);
    void update_drones_display(const DroneScanner& scanner);
    void sort_drones_by_rssi();
    void render_drone_text_display();

    void initialize_mini_spectrum();
    void process_mini_spectrum_data(const ChannelSpectrum& spectrum);
    bool process_bins(uint8_t* power_level);
    void render_mini_spectrum();
    void highlight_threat_zones_in_spectrum(const std::array<DisplayDroneEntry, MAX_DISPLAYED_DRONES>& drones);
    size_t frequency_to_spectrum_bin(Frequency freq_hz) const;
    void clear_spectrum_buffers();
    bool validate_spectrum_data() const;
    size_t get_safe_spectrum_index(size_t x, size_t y) const;

    void set_spectrum_range(Frequency min_freq, Frequency max_freq);

    DroneDisplayController(const DroneDisplayController&) = delete;
    DroneDisplayController& operator=(const DroneDisplayController&) = delete;

    static constexpr const char* DRONE_DISPLAY_FORMAT = "%s %s %-4ddB %c";
    struct SpectrumConfig {
        Frequency min_freq = 2400000000ULL;
        Frequency max_freq = 2500000000ULL;
        uint32_t bandwidth = 24000000;
        uint32_t sampling_rate = 24000000;
    };

private:
    BigFrequency big_display_{{{4, 6 * 16, 28 * 8, 52}}};
    ProgressBar scanning_progress_{{{0, 7 * 16, screen_width, 8}}};
    Text text_threat_summary_{{{0, 8 * 16, screen_width, 16}, "THREAT: NONE"}};
    Text text_status_info_{{{0, 9 * 16, screen_width, 16}, "Ready"}};
    Text text_scanner_stats_{{{0, 10 * 16, screen_width, 16}, "No database"}};
    Text text_trends_compact_{{{0, 11 * 16, screen_width, 16}, ""}};
    Text text_drone_1{{{screen_width - 120, 12 * 16, 120, 16}, ""}};
    Text text_drone_2{{{screen_width - 120, 13 * 16, 120, 16}, ""}};
    Text text_drone_3{{{screen_width - 120, 14 * 16, 120, 16}, ""}};

    std::vector<DisplayDroneEntry> detected_drones_;
    std::array<DisplayDroneEntry, MAX_DISPLAYED_DRONES> displayed_drones_;

    std::array<Color, 240u> spectrum_row;
    std::array<uint8_t, 200> spectrum_power_levels_;
    struct ThreatBin { size_t bin; ThreatLevel threat; };
    std::array<ThreatBin, MAX_DISPLAYED_DRONES> threat_bins_;
    size_t threat_bins_count_ = 0;

    Gradient spectrum_gradient_;
    ChannelSpectrumFIFO* spectrum_fifo_ = nullptr;
    size_t pixel_index = 0;
    uint32_t bins_hz_size = 0;
    uint32_t each_bin_size = 100000;
    uint8_t* powerlevel = nullptr;
    uint8_t min_color_power = 0;
    const uint8_t ignore_dc = 4;

    SpectrumConfig spectrum_config_;
    NavigationView& nav_;

    MessageHandlerRegistration message_handler_spectrum_config_;
    MessageHandlerRegistration message_handler_frame_sync_;

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

    void on_start_scan();
    void on_stop_scan();
    void on_toggle_mode();
    void show_menu();
    void on_load_frequency_file();
    void on_save_frequency();
    void on_toggle_audio_simple();
    void on_audio_toggle();
    void on_advanced_settings();
    void on_open_settings();
    void on_open_constant_settings();

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
    app_settings::SettingsManager constant_settings_manager_;

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
    std::unique_ptr<SmartThreatHeader> smart_header_;
    std::unique_ptr<ConsoleStatusBar> status_bar_;
    std::array<std::unique_ptr<ThreatCard>, 3> threat_cards_;

    NavigationView& nav_;
    std::unique_ptr<DroneHardwareController> hardware_;
    std::unique_ptr<DroneScanner> scanner_;
    std::unique_ptr<AudioManager> audio_mgr_;
    std::unique_ptr<DroneUIController> ui_controller_;
    std::unique_ptr<ScanningCoordinator> scanning_coordinator_;
    std::unique_ptr<DroneDisplayController> display_controller_;

    Button button_start_{{screen_width - 120, screen_height - 32, 120, 32}, "START/STOP"};
    Button button_menu_{{screen_width - 60, screen_height - 32, 60, 32}, "⚙️"};

    std::vector<std::string> scanning_mode_options_ = {"Database Scan", "Wideband Monitor", "Hybrid Discovery"};
    OptionsField field_scanning_mode_{{80, 190, 160, 24}, scanning_mode_options_, 0, "Mode"};

    void initialize_modern_layout();
    void update_modern_layout();
    void handle_scanner_update();
    void start_scanning_thread();
    void stop_scanning_thread();
    bool handle_start_stop_button();
    bool handle_menu_button();

    EnhancedDroneSpectrumAnalyzerView(const EnhancedDroneSpectrumAnalyzerView&) = delete;
    EnhancedDroneSpectrumAnalyzerView& operator=(const EnhancedDroneSpectrumAnalyzerView&) = delete;
};

class LoadingScreenView : public View {
public:
    LoadingScreenView(NavigationView& nav);
    ~LoadingScreenView() = default;

    void paint(Painter& painter) override;

private:
    NavigationView& nav_;
    Text text_eda_{Rect{108, 213, 24, 16}, "EDA"};
    systime_t timer_start_ = 0;
};

// ScanningCoordinator definition from ui_drone_scanner.hpp
class ScanningCoordinator {
public:
    ScanningCoordinator(NavigationView& nav,
                       DroneHardwareController& hardware,
                       DroneScanner& scanner,
                       DroneDisplayController& display_controller,
                       AudioManager& audio_controller);

    ~ScanningCoordinator();

    ScanningCoordinator(const ScanningCoordinator&) = delete;
    ScanningCoordinator& operator=(const ScanningCoordinator&) = delete;

    void start_coordinated_scanning();
    void stop_coordinated_scanning();
    bool is_scanning_active() const { return scanning_active_; }

    void show_session_summary(const std::string& summary);
    void update_runtime_parameters(const DroneAnalyzerSettings& settings);

private:
    static msg_t scanning_thread_function(void* arg);
    msg_t coordinated_scanning_thread();

    Thread* scanning_thread_ = nullptr;
    static constexpr size_t SCANNING_THREAD_STACK_SIZE = 2048;
    bool scanning_active_ = false;
    NavigationView& nav_;
    DroneHardwareController& hardware_;
    DroneScanner& scanner_;
    DroneDisplayController& display_controller_;
    AudioManager& audio_controller_;
    uint32_t scan_interval_ms_ = 750;
};

// AudioManager forward declarations needed for scanner app
struct DroneAudioSettings;
class AudioManager;
struct DroneAnalyzerSettings;

// Implementation includes and definitions would go here in .cpp file

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_SCANNER_COMBINED_HPP__
