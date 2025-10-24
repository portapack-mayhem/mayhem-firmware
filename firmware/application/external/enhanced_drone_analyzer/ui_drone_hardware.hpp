// ui_drone_hardware.hpp
// Hardware management and spectrum processing for Enhanced Drone Analyzer
// Module 1 of 4: Handles all low-level radio hardware control

#ifndef __UI_DRONE_HARDWARE_HPP__
#define __UI_DRONE_HARDWARE_HPP__

#include "radio_state.hpp"          // RxRadioState for hardware management
#include "baseband_api.hpp"         // Baseband spectrum control
#include "portapack.hpp"            // Portapack system interface
#include "radio.hpp"                // Radio tuning interface
#include "message.hpp"              // Message system with ChannelSpectrum
#include "irq_controls.hpp"         // HardwareGuard for mutex protection

#include <mutex>                    // ADD: Mutex for HardwareGuard implementation

#include <memory>

namespace ui::external_app::enhanced_drone_analyzer {

// IMPORTANT: Spectrum constants moved to ui_drone_config.hpp for consolidation

class DroneHardwareController {
public:
    explicit DroneHardwareController(SpectrumMode mode = SpectrumMode::MEDIUM);
    ~DroneHardwareController();

    // Hardware lifecycle management (Following Looking Glass pattern)
    void initialize_hardware();
    void on_hardware_show();           // Called in View::on_show()
    void on_hardware_hide();           // Called in View::on_hide()
    void shutdown_hardware();

    // Spectrum configuration management
    void set_spectrum_mode(SpectrumMode mode);

    // PHASE 5 HARDWARE CONTROL RESTORATION: Getter/Setter functions for frequency and bandwidth
    uint32_t get_spectrum_bandwidth() const;
    void set_spectrum_bandwidth(uint32_t bandwidth_hz);
    Frequency get_spectrum_center_frequency() const;
    void set_spectrum_center_frequency(Frequency center_freq);

    // Frequency tuning hardware control
    bool tune_to_frequency(Frequency frequency_hz);

    // Spectrum streaming control
    void start_spectrum_streaming();
    void stop_spectrum_streaming();

    // RSSI data retrieval from hardware
    int32_t get_real_rssi_from_hardware(Frequency target_frequency);

    // Spectrum message handlers (moving from main class)
    void handle_channel_spectrum_config(const ChannelSpectrumConfigMessage* const message);
    void handle_channel_spectrum(const ChannelSpectrum& spectrum);
    void process_channel_spectrum_data(const ChannelSpectrum& spectrum);

private:
    // ADD: Global hardware mutex following Detector RX pattern
    static std::mutex g_hardware_mutex_;         // Global mutex for hardware access protection

    // Hardware guard for mutex protection following Detector RX pattern
    HardwareLockGuard hardware_guard_;                // RAII mutex protection for hardware access

    // ADD: Reusable hardware guard template
    class HardwareLockGuard {
        std::unique_lock<std::mutex> lock_;
    public:
        HardwareLockGuard() : lock_(g_hardware_mutex_) {}
        // RAII - automatically unlocks when destroyed
    };

    // Spectrum configuration
    SpectrumMode spectrum_mode_;                    // Current spectrum settings
    Frequency center_frequency_;                // ADD: Configurable center frequency
    uint32_t bandwidth_hz_;                         // ADD: Configurable bandwidth override

    // Helper methods for spectrum configuration
    int32_t get_configured_sampling_rate() const;
    int32_t get_configured_bandwidth() const;

    // Hardware components
    RxRadioState radio_state_;                     // Radio state management
    ChannelSpectrumFIFO* fifo_ = nullptr;         // Spectrum data FIFO

    // Spectrum streaming state
    bool spectrum_streaming_active_ = false;
    int32_t last_valid_rssi_ = -120;              // Cached RSSI for smoothing

    // Hardware initialization methods
    void initialize_radio_state();                // Radio hardware setup (SpectrumAnalysis pattern)
    void initialize_spectrum_collector();         // Spectrum collector setup
    void cleanup_spectrum_collector();            // Spectrum collector cleanup

    // Message handlers (moved from main class)
    MessageHandlerRegistration message_handler_spectrum_config_;
    MessageHandlerRegistration message_handler_frame_sync_;

    // Prevent copying
    DroneHardwareController(const DroneHardwareController&) = delete;
    DroneHardwareController& operator=(const DroneHardwareController&) = delete;
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_HARDWARE_HPP__
