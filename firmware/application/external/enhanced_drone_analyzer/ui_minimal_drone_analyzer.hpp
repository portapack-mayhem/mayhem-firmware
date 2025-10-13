// ui_minimal_drone_analyzer.hpp
// Minimal working version of Enhanced Drone Analyzer for Mayhem firmware
// This is the first step in modular refactoring

#ifndef __UI_MINIMAL_DRONE_ANALYZER_HPP__
#define __UI_MINIMAL_DRONE_ANALYZER_HPP__

// PORTA PACK INCLUDES - compatible with Mayhem firmware
#include "ui.hpp"
#include "ui/ui_button.hpp"
#include "ui/ui_text.hpp"
#include "ui/navigation.hpp"
#include "external_app.hpp"
#include "radio_state.hpp"  // RxRadioState for proper hardware management
#include "app_settings.hpp"
#include "baseband_api.hpp"
#include "message.hpp"            // Message system with ChannelSpectrum
// For frequency database - requires proper freqman_db integration later
// #include "freqman_db.hpp"

// FORWARD DECLARATIONS to avoid circular deps - proper includes in .cpp
class DroneFrequencyDatabase;
class BasicFrequencyScanner;
class BasicDroneDetector;
class DetectionLogger;

#include <cstdint>
#include <array>
#include <string>

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

// CLEAN TEXT-BASED UI - NO GRAPHICS, MINIMALIST V0 STYLE
namespace ui::external_app::enhanced_drone_analyzer {

class EnhancedDroneSpectrumAnalyzerView : public View {
public:
    EnhancedDroneSpectrumAnalyzerView(NavigationView& nav);

    // ШАГ 2.1: Исправить порядок остановки в деструкторе по образцу looking_glass
    ~EnhancedDroneSpectrumAnalyzerView() override {
        // Останавливаем все в правильной последовательности: thread -> receiver -> baseband
        if (scanning_thread_) {
            scanning_active_ = false;
            chThdWait(scanning_thread_);
            scanning_thread_ = nullptr;
        }

        // FIX в деструкторе: Стоп spectrum streaming ДО disable receiver
        if (spectrum_streaming_active_) {
            baseband::spectrum_streaming_stop();
            spectrum_streaming_active_ = false;
        }

        // Теперь receiver disable и baseband shutdown
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

    // MINIMALIST V0 STYLE - NO GRAPHICS
    DroneFrequencyDatabase* database_ = nullptr;
    BasicFrequencyScanner* scanner_ = nullptr;
    BasicDroneDetector* detector_ = nullptr;
    // NO spectrum_painter - clean text interface only

    // FIXED: Add missing spectrum_collector_fifo_ declaration
    // ChannelSpectrumFIFO* channel_fifo_ = nullptr; // REMOVED - conflict with message handler FIFO

    // FIXED: Add missing channels_ array with size matching scanner
    std::array<ScanningChannel, 64> channels_;
    size_t active_channels_count_ = 0;

    bool is_real_mode_ = false; // False = demo mode, True = real scanning

    // UI CONTROLS - expanded layout
    Button button_start_{ {0, 0}, "Start Scan" };
    Button button_stop_{ {0, 32}, "Stop Scan" };
    Button button_settings_{ {120, 0}, "Settings" };
    Button button_mode_{ {0, 284}, "Mode: Demo" }; // Toggle demo/real mode

    // STATUS DISPLAY
    Text text_status_{ {0, 64, 240, 16}, "Status: Ready" };
    Text text_detection_info_{ {0, 84, 240, 48}, "No detections" };
    Text text_database_info_{ {0, 136, 240, 32}, "Database: Not loaded" };
    Text text_scanning_info_{ {0, 172, 240, 32}, "Scanning: Idle" };

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
    ThreatLevel current_threat_level_for_simulation_ = ThreatLevel::NONE;

    // PHASE 3: Complete Message system integration (following Looking Glass pattern)
    bool spectrum_streaming_active_ = false;

    // ChannelSpectrum FIFO and message handlers - EXACTLY like Looking Glass
    ChannelSpectrumFIFO* channel_fifo{nullptr};

    MessageHandlerRegistration message_handler_spectrum_config{
        Message::ID::ChannelSpectrumConfig,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const ChannelSpectrumConfigMessage*>(p);
            this->channel_fifo = message.fifo;
        }};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            if (this->channel_fifo) {
                ChannelSpectrum channel_spectrum;
                while (channel_fifo->out(channel_spectrum)) {
                    this->on_channel_spectrum(channel_spectrum);
                }
            }
        }};

    // Event handlers
    void on_start_scan();
    void on_stop_scan();
    void on_open_settings();
    void on_toggle_mode();
    void on_show() override;
    void on_hide() override;

    // PHASE 1: Real RSSI using baseband spectrum streaming (interim solution)
    int32_t get_real_rssi_from_baseband_spectrum(rf::Frequency target_frequency);
    void process_real_rssi_data(size_t channel_idx, int32_t rssi);

    // PHASE 2: Spectrum streaming callbacks (fully integrated)
    int32_t get_real_rssi_from_spectrum_collector(rf::Frequency target_frequency);
    void on_spectrum_update(const Message* const message);

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

    // Neural network style error concealment implementation fields
    bool error_recovery_mode_ = false;  // Track if system is in error recovery state

    // Test hook methods for neural network style robustness testing
    void process_noisy_input(int32_t frequency) {}  // For testing invalid frequency handling
    void simulate_scan_cycle_with_noise() {}       // For testing thread safety
    void process_spectrum_noise(int32_t rssi_value) {} // For testing noise tolerance
    void perform_memory_intensive_operation() {}     // For testing memory constraints
    void apply_configuration_change(const std::string& key, int32_t value) {} // For testing config robustness
    void trigger_artificial_error() {}              // For testing error propagation
    void perform_normal_operation() {}               // For testing normal operation sequencing
    bool interpret_signal_in_context(const std::vector<int32_t>& pattern) { return false; } // For testing contextual interpretation
    void perform_operation_with_noise(int noise_level) {} // For testing noise resilience benchmarks

    // Neural network inspired error handling methods - bounded recovery
    void handle_scan_error(const char* error_msg);
    int validate_simulated_input(int raw_input);
    bool validate_detector_input(const BasicFrequencyScanner& scanner);

    // Step 3: Temporal error isolation methods - similar to LSTM/GRU for error containment
    void perform_robust_scan_cycle();  // Enhanced scan cycle with error classification
    void handle_minor_error(const char* context, int count);
    void handle_moderate_error(const char* context, int count);
    void handle_critical_error(const char* context);
};

} // namespace ui::external_app::enhanced_drone_analyzer

namespace ui::external_app::enhanced_drone_analyzer {

class MinimalDroneSpectrumAnalyzerView : public View {
public:
    MinimalDroneSpectrumAnalyzerView(NavigationView& nav);
    ~MinimalDroneSpectrumAnalyzerView() override = default;

    void focus() override;
    std::string title() const override { return "Enhanced Drone Analyzer"; }
    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;

private:
    NavigationView& nav_;

    // UI CONTROLS - simplified layout
    Button button_start_{ {0, 0}, "Start Scan" };
    Button button_stop_{ {0, 32}, "Stop Scan" };
    Button button_settings_{ {120, 0}, "Settings" };

    // STATUS DISPLAY
    Text text_status_{ {0, 70, 240, 16}, "Status: Ready" };

    // INFO MESSAGES WITH RUSSIAN TEXT
    // (Compatible with Mayhem firmware's text rendering)
    Text text_info_{ {0, 90, 240, 48}, "Enhanced Drone Analyzer\nпроект Кузнецова М.С.\n\nГотов к работе" };

    // VERSION INFO
    Text text_version_{ {0, 220, 240, 16}, "Version: MVP 0.1 (Step 2)" };

    // SCANNING STATE
    bool is_scanning_ = false;
    systime_t scan_start_time_ = 0;
    static constexpr uint32_t SCAN_TEST_DURATION_MS = 5000; // 5 seconds for demo

    // SCANNING THREAD (minimal implementation)
    static msg_t scanning_thread_function(void* arg);
    msg_t scanning_thread();
    Thread* scan_thread_ = nullptr;
    static constexpr uint32_t SCAN_THREAD_STACK_SIZE = 512;

    // Event handlers
    void on_start_scan();
    void on_stop_scan();
    void on_open_settings();
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_MINIMAL_DRONE_ANALYZER_HPP__
