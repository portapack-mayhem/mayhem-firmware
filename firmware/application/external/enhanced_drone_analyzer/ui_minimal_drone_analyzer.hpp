// ui_minimal_drone_analyzer.hpp
// Minimal working version of Enhanced Drone Analyzer for Mayhem firmware
// This is the first step in modular refactoring

#ifndef __UI_MINIMAL_DRONE_ANALYZER_HPP__
#define __UI_MINIMAL_DRONE_ANALYZER_HPP__

// PORTA PACK INCLUDES - compatible with Mayhem firmware
#include "ui.hpp"
#include "ui/ui_button.hpp"
#include "ui/ui_text.hpp"
#include "ui/ui_progress_bar.hpp"  // V0 PROGRESS BAR INTEGRATION
#include "ui/navigation.hpp"
#include "external_app.hpp"
#include "radio_state.hpp"  // RxRadioState for proper hardware management
#include "app_settings.hpp"
#include "baseband_api.hpp"
#include "message.hpp"            // Message system with ChannelSpectrum

// PORTA PACK ADDITIONAL INCLUDES for frequency management - following Recon pattern
#include "freqman_db.hpp"
#include "freqman.hpp"

// MODULAR COMPONENTS
#include "ui_drone_audio.hpp"
#include "ui_drone_tracking.hpp"
#include "ui_drone_spectrum_scanner.hpp"
#include <vector>

#include <cstdint>
#include <array>
#include <string>
#include <map>

// INLINE DEFINITIONS for compatibility
// RENUMBERED ENUM from V0: matches military threat levels exactly
enum class DroneType {
    UNKNOWN = 0,
    DJI_MAVIC,
    DJI_MINI,
    FPV_RACING,
    ORLAN_10 = 200,  // Russian state-own drone
    LANCET = 210,    // Russian military drone
    SHAHED_136 = 220,// Iranian suicide drone
    BAYRAKTAR_TB2 = 230, // Ukrainian/Turkish drone
    MILITARY_UAV = 240,
    SVO = 250,       // SVO codes
    CHUZHOY = 251,   // Enemy
    HUYLO = 252,     // Armed forces
    VSU = 253,       // Armed forces of Ukraine
    CODE_300 = 300   // Military code
};

enum class ThreatLevel {
    NONE = 0,
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    CRITICAL = 4
};

// PORTAPACK EMBEDDED TRACKING SYSTEM - Hardware-compatible implementation
// Optimized for Cortex-M4 constraints (16KB RAM, no heap allocation)
enum class MovementTrend : uint8_t {
    STATIC = 0,      // RSSI стабилен (+/- 3dB)
    APPROACHING = 1, // RSSI растет (дрон приближается)
    RECEDING = 2,    // RSSI падает (дрон удаляется)
    UNKNOWN = 3      // Недостаточно данных для анализа
};

// EMBEDDED-FRIENDLY: Fixed-size structure (15 bytes), no dynamic allocation
struct TrackedDrone {
    uint32_t frequency;       // 4 bytes: Частота в Hz (433MHz = 433000000)
    int16_t last_rssi;        // 2 bytes: Последний RSSI (-120 to -20 dB)
    int16_t prev_rssi;        // 2 bytes: Предыдущий RSSI для тренда
    systime_t last_seen;      // 4 bytes: ChibiOS system time
    uint8_t drone_type;       // 1 byte: DroneType enum as uint8_t
    uint8_t trend_history;    // 1 byte: Битовая маска последних 4 трендов
    uint8_t update_count;     // 1 byte: Количество обновлений (0 = free slot)

    // EMBEDDED CONSTRUCTOR: Initialize to zero (free slot)
    TrackedDrone() : frequency(0), last_rssi(-120), prev_rssi(-120),
                     last_seen(0), drone_type(0), trend_history(0), update_count(0) {}

    // ADD RSSI WITH TREND CALCULATION - Embedded optimized
    void add_rssi(int16_t rssi, systime_t time) {
        prev_rssi = last_rssi;
        last_rssi = rssi;
        last_seen = time;

        if (update_count < 255) update_count++; // Prevent overflow

        // MINIMUM 2 measurements for trend calculation
        if (update_count >= 2) {
            calculate_simple_trend();
        }
    }

    // GET CURRENT TREND - Extract from bit mask
    MovementTrend get_trend() const {
        return static_cast<MovementTrend>(trend_history & 0x03); // Last 2 bits
    }

private:
    // CALCULATE TREND BASED ON RSSI CHANGE - Hardware optimized
    void calculate_simple_trend() {
        const int16_t TREND_THRESHOLD_DB = 3; // 3dB minimum change
        int16_t diff = last_rssi - prev_rssi;

        // PORTAPACK CONSTRAINT: Gradual trend accumulation
        // Store last 4 trends in 8-bit mask (2 bits per trend)
        MovementTrend new_trend = MovementTrend::STATIC;

        if (diff > TREND_THRESHOLD_DB) {
            new_trend = MovementTrend::APPROACHING;  // Signal stronger = closer
        } else if (diff < -TREND_THRESHOLD_DB) {
            new_trend = MovementTrend::RECEDING;     // Signal weaker = farther
        }

        // BIT SHIFT: Rotate history mask left by 2, add new trend
        trend_history = (trend_history << 2) | static_cast<uint8_t>(new_trend);
    }
};

// INTEGRATED: Direct Level/Recon pattern validation
// Using embedded-friendly structures like in Scanner/Recon/Recon
struct SimpleDroneValidation {
    // EMBEDDED PATTERN: Direct RSSI validation like Level app
    static bool validate_rssi_signal(int32_t rssi_db, ThreatLevel threat) {
        // DIRECT from Level/Recon pattern: Simple threshold checks
        if (rssi_db < -100 || rssi_db > -20) return false;  // Like Level hardware limits

        // EMBEDDED: Simple threat-based threshold like in Scanner
        int8_t min_rssi = (threat >= ThreatLevel::HIGH) ? -85 : -90;
        return rssi_db > min_rssi;
    }
};

// CLEAN TEXT-BASED UI - NO GRAPHICS, MINIMALIST V0 STYLE
namespace ui::external_app::enhanced_drone_analyzer {

class EnhancedDroneSpectrumAnalyzerView : public View {
public:
    EnhancedDroneSpectrumAnalyzerView(NavigationView& nav);

    // Исправлено по образцу Level/Scanner: деструктор с RxRadioState cleanup
    ~EnhancedDroneSpectrumAnalyzerView() {
        // Останавливаем все в правильной последовательности: thread -> spectrum -> receiver -> baseband
        if (scanning_thread_) {
            scanning_active_ = false;
            chThdWait(scanning_thread_);
            scanning_thread_ = nullptr;
        }

        // Стоп spectrum streaming ДО disable receiver (Looking Glass pattern)
        if (spectrum_streaming_active_) {
            baseband::spectrum_streaming_stop();
            spectrum_streaming_active_ = false;
        }

        // Теперь receiver disable и baseband shutdown (Level pattern)
        receiver_model.disable();
        baseband::shutdown();
    }

    void focus() override;
    std::string title() const override { return "Enhanced Drone Analyzer"; }
    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;

private:
    NavigationView& nav_;

    // Добавлена RxRadioState для правильного hardware управления (по образцу Level/Scanner)
    RxRadioState radio_state_{};

    // Spectrum streaming state (following Looking Glass pattern)
    bool spectrum_streaming_active_ = false;

    // FIXED: Replace ScanningChannel with FreqmanDB following Recon pattern
    FreqmanDB freq_db_;  // DIRECT FreqmanDB usage like Recon/Scanner
    size_t current_db_index_ = 0;  // Track current frequency index

    // Variable declarations for tracking counts (needed by implementation)
    size_t approaching_count_ = 0;
    size_t receding_count_ = 0;
    size_t static_count_ = 0;
    size_t active_channels_count_ = 0;  // For progress calculation

    // Legacy variables for compatibility
    DroneFrequencyDatabase* database_ = nullptr;  // Will be removed in final cleanup

    bool is_real_mode_ = false; // False = demo mode, True = real scanning

    // MODULAR COMPONENTS
    DroneAudioAlert audio_alerts_;  // Audio feedback manager
    DroneTracker tracker_;         // Drone tracking and trend analysis
    DroneSpectrumScanner spectrum_scanner_;  // Spectrum scanning and RSSI processing

    // PORTAPACK EMBEDDED TRACKING - Hardware constraints optimized (DEPRECATED - use DroneTracker)
    // FIXED-SIZE ARRAY: No dynamic allocation, fits in 16KB RAM
    static constexpr size_t MAX_TRACKED_DRONES = 8;  // 8 drones * 15 bytes = 120 bytes total
    TrackedDrone tracked_drones_[MAX_TRACKED_DRONES]; // Static array, no heap (transitional)
    size_t tracked_drones_count_ = 0;  // Current count of active drones (transitional)

    // UI CONTROLS - FREQUENCY MANAGEMENT ENABLED LAYOUT
    Button button_start_{ {0, 0}, "START" };      // Scan start/stop
    Button button_stop_{ {48, 0}, "STOP" };       // Scan start/stop
    Button button_save_freq_{ {96, 0}, "SAVE" };  // Save detected drone frequency

    // SECOND ROW - File management and advanced controls
    Button button_load_file_{ {0, 32}, "LOAD" };   // Load frequency file
    Button button_advanced_{ {48, 32}, "ADV" };   // Advanced settings
    Button button_mode_{ {96, 32}, "INFO" };      // Always shows info (no toggle)
    Button button_audio_{ {0, 32}, "AUDIO" };     // Audio beacon control
    Button button_settings_{ {96, 32}, "SETTINGS" }; // Settings modal

    // PROGRESS BAR (V0 Inspired) - positioned where spectrum bars would be in V0
    ProgressBar scanning_progress_bar_{ {0, 64, 240, 16} };

    // STATUS DISPLAY (moved down below progress)
    Text text_status_{ {0, 84, 240, 16}, "Status: Ready" };
    Text text_detection_info_{ {0, 104, 240, 48}, "No detections" };
    Text text_database_info_{ {0, 156, 240, 32}, "Database: Not loaded" };
    Text text_scanning_info_{ {0, 192, 240, 32}, "Scanning: Idle" };

    // INFO MESSAGES WITH RUSSIAN TEXT
    Text text_info_{ {0, 208, 240, 32}, "Enhanced Drone Analyzer\nv0.2 Step 4" };
    Text text_version_{ {0, 244, 240, 16}, "© 2025 M.S. Kuznetsov" };

    // SCANNING THREADING
    static msg_t scanning_thread_function(void* arg);
    msg_t scanning_thread();
    Thread* scanning_thread_ = nullptr;
    static constexpr uint32_t SCAN_THREAD_STACK_SIZE = 1024;
    bool scanning_active_ = false;

    // PHASE 2: REAL SPECTRUM STREAMING STATE (integrated with baseband)
    uint32_t scan_cycles_ = 0;
    uint32_t total_detections_ = 0;
    uint32_t current_channel_idx_ = 0;
    int32_t last_valid_rssi_ = -120;  // For RSSI smoothing

    // SIMULATION STATE (to be removed in Phase 2)
    ThreatLevel max_detected_threat_ = ThreatLevel::NONE;

    // Фиксированные message handlers по образцу Looking Glass и Search (Phase 1 исправления)
    MessageHandlerRegistration message_handler_spectrum_config_{
        Message::ID::ChannelSpectrumConfig,
        [this](Message* const p) {
            const auto message = static_cast<const ChannelSpectrumConfigMessage*>(p);
            this->on_channel_spectrum_config(message);
        }
    };

    MessageHandlerRegistration message_handler_spectrum_{
        Message::ID::ChannelSpectrum,
        [this](Message* const p) {
            const auto message = static_cast<const ChannelSpectrumMessage*>(p);
            this->on_channel_spectrum(*message);
        }
    };

    // Event handlers
    void on_start_scan();
    void on_stop_scan();
    void on_open_settings();
    void on_toggle_mode();
    void on_show() override;
    void on_hide() override;

    // Spectrum handling methods (following Search and Glass patterns)
    void on_channel_spectrum_config(const ChannelSpectrumConfigMessage* const message);
    void on_channel_spectrum(const ChannelSpectrum& spectrum);

    //

// FIXED: Remove unimplemented method declarations that are causing link errors
// These can be added back when actually implemented

    // Initialize database and scanner in proper order
    void initialize_database_and_scanner();
    void cleanup_database_and_scanner();

    // Enhanced scanning logic
    void perform_scan_cycle();
    void update_detection_display();
    void update_database_stats();

    // Thread safe UI updates
    void request_ui_update();

    // Real vs Demo mode handling
    bool is_demo_mode() const { return !is_real_mode_; }
    void switch_to_demo_mode();
    void switch_to_real_mode();

    // Handling errors and validation
    void handle_scan_error(const char* error_msg);

    // Tracking detected drones
    void update_tracked_drone(DroneType type, rf::Frequency frequency, int32_t rssi, ThreatLevel threat_level);

    // REMOVED: Advanced tracking data - too complex for embedded system

    // TEXT DISPLAYS - Portapack H2 optimized layout
    Text text_threat_level_{ {0, 210, 240, 16}, "THREAT: NONE" };
    Text text_info_{ {0, 226, 240, 16}, "Enhanced Drone Analyzer v0.2" };
    Text text_copyright_{ {0, 242, 240, 16}, "© 2025 M.S. Kuznetsov" };

    // Add threat color methods (following V0 spectrum painter pattern)
    Color get_threat_level_color(ThreatLevel level) const;
    const char* get_threat_level_name(ThreatLevel level) const;

    // FREQUENCY MANAGEMENT METHODS - IMPLEMENTED VIA FREQMANDB DIRECT ACCESS
    // Direct FreqmanDB access (like Recon) - no separate file management needed
    // All frequency management handled through FreqmanDB instance freq_db_

    // IMPLEMENTED: Frequency range warning system (using number of entries in freq_db_)
    void on_frequency_warning();
    void show_frequency_warning_dialog();
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_MINIMAL_DRONE_ANALYZER_HPP__
