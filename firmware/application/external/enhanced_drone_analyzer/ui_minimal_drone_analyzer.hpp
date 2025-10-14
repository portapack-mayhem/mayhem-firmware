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

    // MODULAR COMPONENTS - unified types
#include "ui_drone_types.hpp"
#include "ui_drone_audio.hpp"
#include "ui_drone_audio_settings.hpp"
#include "ui_drone_tracking.hpp"
#include "ui_drone_spectrum_scanner.hpp"
#include "ui_drone_frequency_manager.hpp"
#include <vector>

// CLEAN TEXT-BASED UI - NO GRAPHICS, MINIMALIST V0 STYLE
namespace ui::external_app::enhanced_drone_analyzer {

class EnhancedDroneSpectrumAnalyzerView : public View {
public:
    EnhancedDroneSpectrumAnalyzerView(NavigationView& nav);

    // Fixed destructor with proper cleanup order like other spectrum apps
    ~EnhancedDroneSpectrumAnalyzerView() {
        // Stop thread first to avoid resource conflicts
        if (scanning_thread_) {
            scanning_active_ = false;
            chThdWait(scanning_thread_);
            scanning_thread_ = nullptr;
        }
        // Note: No spectrum/receiver cleanup here - hardware init done in on_show()/on_hide()
    }

    void focus() override;
    std::string title() const override { return "Enhanced Drone Analyzer"; }
    void paint(Painter& painter) override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;

private:
    NavigationView& nav_;

    // Исправлено: RxRadioState с корректным SpectrumAnalysis mode (как в Looking Glass)
    RxRadioState radio_state_{ReceiverModel::Mode::SpectrumAnalysis};

    // CRITICAL FIX: Add FIFO for proper spectrum data handling (Looking Glass pattern)
    ChannelSpectrumFIFO* fifo_ = nullptr;

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

    // REMOVED: Legacy database pointer causing null dereference crashes
    // All database operations now handled directly via freq_db_ member

    bool is_real_mode_ = true; // PK: FIXED - default to real mode, corrected inverted logic

    // INLINE FUNCTION OBJECTS FOR AUDIO/DATABASE CALLS
    // Using functional approach to avoid module instantiation conflicts

    // BANDWIDTH OFFSET SCANNING: New implementation ±6MHz from center (default 3MHz)
    // Hardware-optimized for Portapack real-time spectrum analysis

    // PRIMARY CONTROLS (Like Level app - minimal, focused)
    Button button_start_{ {0, 0}, "START/STOP" };  // Toggle scan on/off (primary action)
    Button button_menu_{ {120, 0}, "settings" };  // Menu for secondary functions

    // BIG DISPLAY & STATUS (Like Scanner - large frequency + color coding)
    BigFrequencyDisplay big_display_{ {0, 24, 240, 32} };
    ProgressBar scanning_progress_{ {0, 64, 240, 16} };

    // COMPACT STATUS DISPLAY (Like Recon - essential info only)
    Text text_threat_summary_{ {0, 84, 240, 16}, "THREAT: NONE | ▲0 ■0 ▼0" };
    Text text_status_info_{ {0, 100, 240, 16}, "Ready - Enhanced Drone Analyzer" };
    Text text_scanner_stats_{ {0, 116, 240, 16}, "Detections: 0 | Scan Freq: N/A" };

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

    // SPECTRUM MESSAGE HANDLERS (Looking Glass pattern)
    MessageHandlerRegistration message_handler_spectrum_config_{
        Message::ID::ChannelSpectrumConfig,
        [this](Message* const p) {
            const auto message = *reinterpret_cast<const ChannelSpectrumConfigMessage*>(p);
            fifo_ = message.fifo;
            on_channel_spectrum_config(&message);
        }
    };

    MessageHandlerRegistration message_handler_frame_sync_{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            if (fifo_) {
                ChannelSpectrum channel_spectrum;
                while (fifo_->out(channel_spectrum)) {
                    on_channel_spectrum(channel_spectrum);
                }
            }
        }
    };

    // Initialize database and scanner in proper order
    void initialize_database_and_scanner();
    void cleanup_database_and_scanner();
    void perform_scan_cycle();
    void update_detection_display();
    void update_database_stats();

    // Real vs Demo mode handling
    bool is_demo_mode() const { return !is_real_mode_; }
    void switch_to_demo_mode();
    void switch_to_real_mode();

    // Handling errors and validation
    void handle_scan_error(const char* error_msg);

    // Tracking detected drones
    void update_tracked_drone(DroneType type, rf::Frequency frequency, int32_t rssi, ThreatLevel threat_level);

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

    // IMPLEMENTED: Full frequency management UI (advanced database editor)
    void on_manage_frequencies();

    // IMPLEMENTED: Create empty database functionality (for creating new frequency files)
    void on_create_new_database();
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_MINIMAL_DRONE_ANALYZER_HPP__
