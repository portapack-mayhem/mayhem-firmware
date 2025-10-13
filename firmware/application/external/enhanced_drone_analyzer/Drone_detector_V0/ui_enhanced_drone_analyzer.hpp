// ui_enhanced_drone_analyzer.hpp
// Enhanced Drone Spectrum Analyzer UI for PortaPack Mayhem firmware

#ifndef __UI_ENHANCED_DRONE_ANALYZER_HPP__
#define __UI_ENHANCED_DRONE_ANALYZER_HPP__

// UI components - adapted for PortaPack Mayhem compatibility
#include "ui/ui.hpp"
#include "ui/ui_spectrum.hpp"
#include "ui/ui_button.hpp"
#include "ui/ui_checkbox.hpp"
#include "ui/ui_text.hpp"
#include "ui/number_field.hpp"
#include "ui/ui_freq_field.hpp"
#include "ui/color.hpp"
#include "ui/string_format.hpp"
#include "message.hpp"
#include "radio_state.hpp"
#include "ch.h"
#include "hal.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <deque>

#include "ui_enhanced_frequency_database.hpp"
#include "ui_enhanced_spectrum_painter.hpp"
// REMOVED: #include "receiver_model.hpp" - not found in mayhem firmware, using radio::* API instead

// Baseband API for audio beep functionality (compatible with Mayhem firmware)
#include "baseband_api.hpp"

// Baseband integration includes
#include "spectrum_collector.hpp"
#include "message.hpp"

// Forward declarations
class EnhancedDroneSpectrumAnalyzerView;
class AudioSettingsManager;

// Data structures
struct enhanced_drone_channel_t {
    rf::Frequency frequency = 0;
    int32_t rssi_threshold = -80;
    DroneType drone_type = DroneType::UNKNOWN;
    ThreatLevel threat_level = ThreatLevel::NONE;
    bool is_detected = false;
    bool is_approaching = false;
    bool is_new_detection = false;
    uint32_t detection_count = 0;
    systime_t last_detection = 0;
    systime_t first_detection = 0;
    int32_t peak_rssi = -120;
    int32_t average_rssi = -120;
    int32_t current_rssi = -120;
    uint32_t consecutive_detections = 0;
    uint32_t rssi_sample_count = 0;  // Added for proper EWMA calculation
    bool is_logged = false;
};

struct enhanced_analyzer_config_t {
    uint32_t scan_time_ms = 50;
    int32_t rssi_threshold = -80;
    bool sound_enabled = true;
    bool auto_log = true;
    bool waterfall_enabled = false;
    int32_t squelch_level = -100;
    bool threat_alerts = true;
    uint8_t sensitivity_level = 7;
    bool auto_scan = false;
    bool advanced_mode = false;
};

struct custom_frequency_entry_t {
    rf::Frequency frequency_hz = 0;
    std::string name = "";
    DroneType drone_type = DroneType::UNKNOWN;
    ThreatLevel threat_level = ThreatLevel::MEDIUM;
    int32_t rssi_threshold = -80;
    std::string notes = "";
    bool enabled = true;
    uint32_t bandwidth_hz = 6000000;  // Default 6MHz center span
    int32_t center_offset_hz = 0;     // Offset from main frequency for scanning
};

// Drone trend enumeration
enum class DroneTrend {
    DETECTED,
    APPROACHING,
    RECEDING
};

// Enhanced TrackedDrone class for individual drone tracking
class TrackedDrone {
public:
    TrackedDrone(uint32_t drone_id, const EnhancedFrequencyEntry* entry, int32_t initial_rssi);
    void update_rssi(int32_t new_rssi);
    DroneTrend calculate_trend() const;
    Color get_color() const;
    bool should_cleanup(int32_t threshold) const;

    uint32_t id;
    const EnhancedFrequencyEntry* frequency_entry;
    DroneTrend trend;
    systime_t last_update;
    uint32_t detection_count;
    bool active;
    systime_t last_seen_time;
    uint32_t consecutive_detections;
    float confidence_score;
    uint32_t suspicious_flags;

private:
    // OPTIMIZED: Ring buffer instead of deque to prevent reallocations
    std::array<int32_t, MAX_RSSI_HISTORY> rssi_buffer_;
    size_t head_ = 0;
    size_t size_ = 0;
};

// Waterfall Spectrum for temporal signal visualization
class WaterfallSpectrum {
private:
    static constexpr size_t BUFFER_SIZE = 32;
    static constexpr int SLICE_PIXEL_HEIGHT = 2;

    struct WaterfallSlice {
        uint32_t timestamp;
        int16_t rssi_levels[64];  // RSSI for 64 channels max
        uint8_t threat_mask;       // Bitmask for threat levels
    };

    WaterfallSlice buffer_[BUFFER_SIZE];
    size_t head_ = 0;
    size_t valid_slices_ = 0;
    bool enabled_ = false;

public:
    void enable(bool state) { enabled_ = state; }
    bool enabled() const { return enabled_; }

    void add_slice(const std::array<enhanced_drone_channel_t, 64>& channels, size_t active_count) {
        if (!enabled_) return;

        auto& slice = buffer_[head_];
        slice.timestamp = chTimeNow();
        slice.threat_mask = 0;

        // Copy RSSI data with compression (int32_t -> int16_t)
        for (size_t i = 0; i < std::min(active_count, size_t(64)); ++i) {
            slice.rssi_levels[i] = static_cast<int16_t>(channels[i].current_rssi);
        }

        // Create threat bitmask (first 8 channels)
        for (size_t i = 0; i < std::min(active_count, size_t(8)); ++i) {
            if (channels[i].threat_level >= ThreatLevel::HIGH) {
                slice.threat_mask |= (1 << i);
            }
        }

        head_ = (head_ + 1) % BUFFER_SIZE;
        valid_slices_ = std::min(valid_slices_ + 1, BUFFER_SIZE);
    }

    void paint(Painter& painter, Rect rect, size_t active_channels) {
        if (!enabled_ || valid_slices_ == 0) return;

        const int total_height = valid_slices_ * SLICE_PIXEL_HEIGHT;

        for (size_t time_idx = 0; time_idx < valid_slices_; ++time_idx) {
            size_t slice_idx = (head_ + BUFFER_SIZE - time_idx - 1) % BUFFER_SIZE;
            const auto& slice = buffer_[slice_idx];

            int y_base = rect.top() + total_height - (time_idx + 1) * SLICE_PIXEL_HEIGHT;

            for (size_t freq_idx = 0; freq_idx < std::min(active_channels, size_t(64)); ++freq_idx) {
                int x = rect.left() + (freq_idx * rect.width()) / active_channels;
                int16_t rssi = slice.rssi_levels[freq_idx];

                Color color = get_waterfall_color(rssi, time_idx);

                // Draw vertical line for this frequency in time
                for (int dy = 0; dy < SLICE_PIXEL_HEIGHT; ++dy) {
                    int y = y_base + dy;
                    if (y >= rect.top() && y < rect.bottom()) {
                        painter.draw_pixel(x, y, color);
                    }
                }
            }

            // Draw threat indicators if any threats in this slice
            if (slice.threat_mask && time_idx < 5) { // Only recent slices
                draw_threat_indicators(painter, rect, slice, time_idx);
            }
        }
    }

    void clear() {
        head_ = 0;
        valid_slices_ = 0;
        memset(buffer_, 0, sizeof(buffer_));
    }

private:
    Color get_waterfall_color(int16_t rssi, size_t time_idx) const {
        // Ignore very weak signals
        if (rssi < -90) return Color::black();

        // Color scheme: Blue=weak, Green=medium, Red=strong
        Color base_color;
        if (rssi < -70) {
            base_color = Color(0, 0, std::min(255, (-rssi)));
        } else if (rssi < -50) {
            base_color = Color(0, std::min(255, (rssi + 70) * 3), 0);
        } else {
            base_color = Color(std::min(255, (rssi + 50) * 2), 0, 0);
        }

        // Apply fade effect for older data (brighter = newer)
        float fade_factor = 1.0f - (time_idx * 0.03f);
        fade_factor = std::max(0.2f, fade_factor);

        return Color(
            static_cast<uint8_t>(base_color.r() * fade_factor),
            static_cast<uint8_t>(base_color.g() * fade_factor),
            static_cast<uint8_t>(base_color.b() * fade_factor)
        );
    }

    void draw_threat_indicators(Painter& painter, Rect rect, const WaterfallSlice& slice, size_t time_idx) {
        int y_text = rect.top() + (valid_slices_ - time_idx) * SLICE_PIXEL_HEIGHT - 8;

        for (size_t i = 0; i < 8; ++i) {
            if (slice.threat_mask & (1 << i)) {
                int x_threat = rect.left() + (i * rect.width()) / 8;
                // Draw small red indicator
                painter.draw_pixel(x_threat, y_text, Color::red());
            }
        }
    }
};

// Audio Settings Manager
class AudioSettingsManager {
public:
    AudioSettingsManager();
    void load_settings();
    void save_settings();

    bool beep_enabled() const { return beep_enabled_; }
    uint16_t get_frequency(ThreatLevel level) const;
    uint16_t get_duration(ThreatLevel level) const;
    void set_tone(ThreatLevel level, uint16_t freq, uint16_t duration);

private:
    bool beep_enabled_;
    std::array<uint16_t, 5> beep_frequencies_;  // Per threat level
    std::array<uint16_t, 5> beep_durations_;
};

// Frequency Manager for custom frequency handling
class FrequencyManager {
public:
    FrequencyManager();

    const EnhancedFrequencyEntry* get_frequency(size_t index) const;
    size_t active_frequencies() const { return active_frequencies_; }
    size_t enabled_count() const;
    void add_frequency(const EnhancedFrequencyEntry& entry);
    void remove_frequency(size_t index);
    void toggle_frequency(size_t index);
    void load_frequencies();
    void save_frequencies();

private:
    static constexpr size_t MAX_FREQUENCIES = 64;
    std::array<EnhancedFrequencyEntry, MAX_FREQUENCIES> frequencies_;
    size_t active_frequencies_;
};

// Logging system using mayhem's LogFile for SD card storage
class DetectionLogger {
public:
    DetectionLogger();
    ~DetectionLogger();

    // Error types for embedded diagnostics
    enum LoggerError {
        NO_ERROR = 0,
        FILE_OPEN_ERROR,
        WRITE_ERROR,
        FILESYSTEM_FULL
    };

    // Compatible structure for logging
    struct DetectionData {
        uint32_t timestamp;
        uint32_t frequency_hz;
        int32_t rssi_db;
        ThreatLevel threat_level;
        DroneType drone_type;
        uint32_t detection_count;
        float confidence_score;
        char notes[128];  // Added notes field
    };

    // Diagnostics methods
    LoggerError get_last_error() const { return last_error_; }
    uint32_t get_last_error_time() const { return last_error_time_; }
    void clear_error() { last_error_ = NO_ERROR; last_error_time_ = 0; }

    bool log_detection(const DetectionData& data);
    bool export_session_summary();
    uint32_t get_logged_entries_count() const { return logged_count_; }

private:
    LogFile csv_logger_;
    uint32_t logged_count_ = 0;
    uint32_t session_start_time_;
    bool csv_header_written_ = false;

    // CSV format helpers compatible with mayhem LogFile
    void ensure_csv_header();
    void format_csv_entry(const DetectionData& data, std::string& result);
    const char* threat_level_to_string(ThreatLevel level) const;
    const char* drone_type_to_string(DroneType type) const;

    std::filesystem::path generate_log_filename() const;
};

// Advanced Analytics for threat pattern recognition
class AdvancedAnalytics {
public:
    AdvancedAnalytics();

    struct AnalyticsResult {
        struct Pattern {
            std::string description;
            uint32_t occurrences;
            ThreatLevel risk_level;
            bool is_trend;
            uint32_t confidence_percent;
        };

        struct Prediction {
            std::string predicted_event;
            uint32_t time_to_event_ms;
            float probability;
            std::string basis;
        };

        struct Statistics {
            uint32_t total_detections;
            uint32_t unique_frequencies;
            uint32_t threat_distribution[5]; // per ThreatLevel
            float average_session_time;
            uint32_t false_positive_count;
        };

        std::vector<Pattern> detected_patterns;
        std::vector<Prediction> predictions;
        Statistics stats;
    };

    AnalyticsResult analyze_session_data(const std::vector<std::unique_ptr<TrackedDrone>>& drones,
                                       const std::array<enhanced_drone_channel_t, 64>& channels,
                                       uint32_t session_time_ms);

    struct ValidationResult {
        bool is_valid;
        float confidence;
        std::string reasoning;
        std::string recommendations;
    };

    ValidationResult validate_detection_logic(const enhanced_drone_channel_t& channel,
                                            int32_t current_rssi,
                                            uint32_t detection_count,
                                            const DetectionLogger& logger);

private:
    uint32_t analysis_count_ = 0;

    // Pattern recognition helpers
    void detect_frequency_patterns(std::vector<AnalyticsResult::Pattern>& patterns,
                                 const std::array<enhanced_drone_channel_t, 64>& channels);
    void detect_timing_patterns(std::vector<AnalyticsResult::Pattern>& patterns,
                              const std::vector<std::unique_ptr<TrackedDrone>>& drones);
    void detect_correlation_patterns(std::vector<AnalyticsResult::Pattern>& patterns,
                                   const std::array<enhanced_drone_channel_t, 64>& channels,
                                   const std::vector<std::unique_ptr<TrackedDrone>>& drones);

    // Prediction algorithms
    void generate_behavior_predictions(std::vector<AnalyticsResult::Prediction>& predictions,
                                    const std::vector<std::unique_ptr<TrackedDrone>>& drones);
    void calculate_threat_trends(std::vector<AnalyticsResult::Prediction>& predictions,
                               const std::array<enhanced_drone_channel_t, 64>& channels);

    // Statistical analysis
    void compute_session_statistics(AnalyticsResult::Statistics& stats,
                                  const std::vector<std::unique_ptr<TrackedDrone>>& drones,
                                  const std::array<enhanced_drone_channel_t, 64>& channels,
                                  uint32_t session_time_ms);

    // Validation and reasoning
    bool apply_common_sense_checks(const enhanced_drone_channel_t& channel,
                                 int32_t current_rssi);
    float calculate_signal_consistency_score(const enhanced_drone_channel_t& channel,
                                           int32_t current_rssi);
    std::string generate_validation_reasoning(bool is_valid,
                                            const enhanced_drone_channel_t& channel);
};

// Reasoning Engine for intelligent threat analysis
class ReasoningEngine {
public:
    ReasoningEngine();

    struct ReasoningResult {
        AlertLevel suggested_level = AlertLevel::NONE;
        std::string reasoning_text = "";
        std::string recommended_actions = "";
        float confidence = 0.0f;
        uint32_t flags = 0;
    };

    enum AlertLevel {
        NONE = 0,
        MONITOR,      // Only monitoring required
        TRACK,        // Start tracking
        ALERT,        // Alert operator
        ACTIVATE      // Immediate action
    };

    ReasoningResult analyze_situation(const std::vector<std::unique_ptr<TrackedDrone>>& drones,
                                     const enhanced_analyzer_config_t& config,
                                     uint32_t total_detections);

    bool validate_detection(const enhanced_drone_channel_t& channel,
                           int32_t current_rssi,
                           uint32_t detection_count);

    std::string generate_threat_summary(const std::vector<std::unique_ptr<TrackedDrone>>& drones);
    void update_patterns(uint32_t current_time);

private:
    uint32_t last_analysis_time_ = 0;
    uint32_t analysis_count_ = 0;

    struct PatternData {
        uint32_t recent_detections = 0;
        uint32_t pattern_start_time = 0;
        bool suspicious_activity = false;
        uint32_t consecutive_high_threats = 0;
    } pattern_data_;
};

// Frequency Manager Dialog for managing custom frequencies
class FrequencyManagerDialog : public View {
public:
    FrequencyManagerDialog(NavigationView& nav, FrequencyManager& freq_mgr);
    ~FrequencyManagerDialog() = default;

    void focus() override;
    std::string title() const override { return "Frequency Manager"; }
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    void paint(Painter& painter) override;

private:
    NavigationView& nav_;
    FrequencyManager& freq_mgr_;

    Button button_add_{ {0, 280}, "Add" };
    Button button_edit_{ {60, 280}, "Edit" };
    Button button_delete_{ {120, 280}, "Del" };
    Button button_toggle_{ {180, 280}, "Toggle" };
    Button button_center_{ {0, 260}, "Center Settings" };
    Button button_expand_limit_{ {180, 320}, "Expand" };

    FreqField field_center_freq_{ {120, 260}, config().center_frequency };
    NumberField field_bandwidth_{ {0, 240}, "Bandwidth (MHz):", 1, 20, 1, ' ' };
    NumberField field_offset_{ {120, 240}, "Center Offset (Hz):", -10000000, 10000000, 500000, ' ' };

    Text text_freq_list_{ {0, 30, 240, 200}, "" };

    size_t selected_index_ = 0;

    void draw_frequency_list();
    void on_add_frequency();
    void on_edit_frequency();
    void on_delete_frequency();
    void on_toggle_frequency();
    void on_center_settings();
    void on_expand_limit();
    void update_center_display();
    void validate_center_settings();
};

// Settings Dialog - REDESIGNED WITH SUBMENU ACCESS
class SettingsDialog : public View {
public:
    SettingsDialog(NavigationView& nav, enhanced_analyzer_config_t& config,
                  AudioSettingsManager& audio_mgr,
                  EnhancedDroneSpectrumAnalyzerView* analyzer_ptr = nullptr);
    ~SettingsDialog() = default;

    void focus() override;
    std::string title() const override { return "Scanner Settings"; }
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    void paint(Painter& painter) override;

private:
    NavigationView& nav_;
    enhanced_analyzer_config_t& config_;
    AudioSettingsManager& audio_manager_;
    EnhancedDroneSpectrumAnalyzerView* analyzer_;  // Reference to main analyzer

    // BOTTOM CONTROLS
    Button button_ok_{ {0, 280}, "OK" };
    Button button_audio_{ {120, 280}, "Audio Tones" };

    // FORM FIELDS
    NumberField field_rssi_thresh_{ {0, 50}, "RSSI Threshold:", -120, -30, 5, ' ' };
    NumberField field_scan_interval_{ {0, 80}, "Scan Interval:", 50, 2000, 25, ' ' };
    NumberField field_channel_delay_{ {0, 110}, "Channel Delay:", 0, 1000, 5, ' ' };

    // SUBMENU BUTTONS - ACCESS TO ADVANCED FEATURES
    Button button_freq_manager_{ {0, 150}, "Frequency Manager" };
    Button button_tracking_{ {120, 150}, "Tracking" };
    Button button_reasoning_{ {0, 180}, "Reasoning" };
    Button button_analytics_{ {120, 180}, "Analytics" };
    Button button_about_{ {0, 210}, "About Author" };

    void close_dialog();
    void open_audio_tones_dialog();
    void open_freq_manager();
    void open_tracking();
    void open_reasoning();
    void open_analytics();
};

// Tracked Drones Dialog
class TrackedDronesDialog : public View {
public:
    TrackedDronesDialog(NavigationView& nav, std::vector<std::unique_ptr<TrackedDrone>>& tracked_drones);
    ~TrackedDronesDialog() = default;

    void focus() override;
    std::string title() const override { return "Tracked Drones"; }
    bool on_key(const KeyEvent key) override;
    void paint(Painter& painter) override;

private:
    NavigationView& nav_;
    std::vector<std::unique_ptr<TrackedDrone>>& tracked_drones_;

    void draw_drone_info(size_t index, Coord y_pos);
    const char* get_trend_str(DroneTrend trend);
};

// Enhanced Drone Spectrum Analyzer Main View
class EnhancedDroneSpectrumAnalyzerView : public View {
public:
    EnhancedDroneSpectrumAnalyzerView(NavigationView& nav);
    ~EnhancedDroneSpectrumAnalyzerView();

    void focus() override;
    std::string title() const override { return "Enhanced Drone Analyzer"; }
    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    bool on_encoder(const EncoderEvent delta) override;

private:
    NavigationView& nav_;

    // UI Controls - REDESIGNED LAYOUT WITH PROPER SPACING
    // PRIMARY ROW: Essential scanning controls
    Button button_start_{ {0, 0}, "Start" };
    Button button_settings_{ {80, 0}, "Settings" };
    Button button_audio_{ {160, 0}, "Audio" };

    // SECONDARY ROW: Advanced and status controls
    Button button_stop_{ {0, 32}, "Stop" };
    Button button_advanced_{ {80, 32}, "Advanced" };

    // CLEANED UP - MOVED TO SUBMENUS
    // button_manage_freq_, button_track_, button_reason_, button_analytics_
    // moved to Settings dialog submenu for better UX and space management

    Checkbox checkbox_sound_{ {0, 64}, "Sound" };
    Checkbox checkbox_auto_scan_{ {80, 64}, "Auto" };
    Checkbox checkbox_waterfall_{ {160, 64}, "WF" };

    Text text_status_{ {0, 130, 240, 12}, "Status: Ready" };
    Text text_detection_info_{ {0, 144, 240, 28}, "No detections" };
    Text text_threat_level_{ {0, 184, 240, 16}, "Threat: None" };
    Text text_frequency_info_{ {0, 200, 240, 16}, "Freq: ---" };

    // Scanning progress bar - shows current channel scan progress
    ProgressBar scanning_progress_bar_{ {0, 95, 240, 16} };

    // Spectrum painter
    EnhancedSpectrumPainter spectrum_painter_;

    // Waterfall spectrum
    WaterfallSpectrum waterfall_spectrum_;

    // Configuration and managers
    enhanced_analyzer_config_t config_;
    FrequencyManager freq_manager_;
    AudioSettingsManager audio_manager_;
    ReasoningEngine reasoning_engine_;
    DetectionLogger logger_;
    AdvancedAnalytics analytics_;

    // Friend declaration for SettingsDialog access
    friend class SettingsDialog;

    // Getter methods for internal managers (used by SettingsDialog)
    FrequencyManager& get_freq_manager() { return freq_manager_; }
    std::vector<std::unique_ptr<TrackedDrone>>& get_tracked_drones() { return tracked_drones_; }

    // Threading and timing
    Thread* scan_thread_ = nullptr;
    mutex_t data_mutex_;
    bool is_scanning_ = false;
    bool safe_lock_enabled_ = true;  // Enable/disable thread safety for debugging
    bool is_stopping_ = false;  // Added for graceful shutdown
    bool is_paused_ = false;
    bool is_advanced_mode_ = false;
    size_t current_channel_index_ = 0;

    // Baseband spectrum processing
    SpectrumCollector spectrum_collector_;

    // Radio and filesystem
    // REMOVED: ReceiverModel receiver_model_ - using radio::* API instead
    std::filesystem::path logs_dir_{"LOGS"};

    // Statistics
    uint32_t total_scans_ = 0;
    uint32_t total_detections_ = 0;
    uint32_t critical_detections_ = 0;
    uint32_t next_drone_id_ = 1;

    // Advanced tracking
    std::array<TrackedDrone, MAX_TRACKED_DRONES> tracked_drones_static_;
    std::array<TrackedDrone*, MAX_TRACKED_DRONES> tracked_drones_;
    size_t tracked_drones_count_ = 0;
    static constexpr int32_t CLEANUP_THRESHOLD = -90;
    static constexpr size_t MAX_RSSI_HISTORY = 16;
    static constexpr size_t MAX_TRACKED_DRONES = 16;  // Embedded optimization

    static constexpr size_t MAX_CHANNELS = 128;  // Reduced from 256 for embedded RAM
    static constexpr size_t DEFAULT_MAX_ACTIVE_CHANNELS = 64;
    std::array<enhanced_drone_channel_t, MAX_CHANNELS> channels_;
    size_t active_channels_count_ = 0;

    NumberField field_scan_time_{ {0, 100}, "Scan Time (ms):", 50, 5000, 50, ' ' };
    NumberField field_rssi_threshold_{ {0, 120}, "RSSI Thresh:", -120, -30, 5, ' ' };
    NumberField field_squelch_level_{ {0, 140}, "Squelch:", -120, -30, 5, ' ' };

    systime_t scan_start_time_ = 0;
    uint32_t critical_threats_ = 0;
    uint32_t high_threats_ = 0;
    uint32_t medium_threats_ = 0;

    // Private methods
    void initialize_channels();
    void start_scanning();
    void stop_scanning();
    void pause_scanning();
    void scan_next_channel();
    static msg_t scanning_thread_function(void* arg);
    void scanning_thread();

    void process_rssi_data(size_t channel_index, int32_t rssi);
    void on_detection(const EnhancedFrequencyEntry* entry, int32_t rssi);
    void update_tracked_drones();
    void cleanup_inactive_drones();

    void update_status_display();
    void update_detection_display();
    void update_threat_display();

    const char* get_drone_type_name(DroneType type) const;
    const char* get_threat_level_name(ThreatLevel level) const;
    Color get_threat_level_color(ThreatLevel level) const;

    void play_detection_beep(ThreatLevel level);
    void play_sos_signal();

    void on_start_scan();
    void on_stop_scan();
    void on_settings();
    void on_save_log();
    void on_advanced_mode();
    void on_toggle_waterfall();

    void open_frequency_manager();
    void open_audio_settings();
    void open_settings_dialog();
    void open_tracked_drones_dialog();
    void on_reason_analysis();
    void on_analytics_analysis();

    void load_settings();
    void save_settings();
    void validate_all_settings();

    // Advanced UI methods
    // OPTIMIZATION: UI rendering cache and throttling
    uint32_t last_render_time_ = 0;
    static constexpr uint32_t MAX_RENDER_FPS = 10;  // Limit to 10 FPS for better performance
    ThreatLevel cached_threat_level_ = ThreatLevel::NONE;
    bool ui_needs_update_ = true;

    void draw_status_header();
    void draw_live_drone_status();
    void draw_scanning_info();
    void draw_quick_reference();
    void draw_control_info();
    void draw_simple_spectrum_analyzer();

    void show_channel_details(size_t index);
    void draw_frequency_list();
    void draw_tracking_status();
};

#endif // __UI_ENHANCED_DRONE_ANALYZER_HPP__
