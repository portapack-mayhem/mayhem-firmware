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
#include "channel_spectrum.hpp"     // Spectrum data structures

#include <memory>

namespace ui::external_app::enhanced_drone_analyzer {

// Configurable spectrum settings matching other spectrum apps
enum class SpectrumMode {
    ULTRA_NARROW = 0,    // 4MHz bandwidth
    NARROW = 1,          // 8MHz bandwidth
    MEDIUM = 2,          // 12MHz bandwidth (default for drone analysis)
    WIDE = 3,            // 20MHz bandwidth
    ULTRA_WIDE = 4       // 24MHz bandwidth (Looking Glass max)
};

// Bandwidth/sampling rate mapping following DetectorRx/Level patterns
static constexpr int32_t SPECTRUM_BANDWIDTHS[] = {4000000, 8000000, 12000000, 20000000, 24000000};
static constexpr int32_t SPECTRUM_SAMPLING_RATES[] = {4000000, 8000000, 12000000, 20000000, 24000000};

class DroneHardwareController {
public:
    DroneHardwareController(SpectrumMode mode = SpectrumMode::MEDIUM);
    ~DroneHardwareController();

    // Hardware lifecycle management (Following Looking Glass pattern)
    void initialize_hardware();
    void on_hardware_show();           // Called in View::on_show()
    void on_hardware_hide();           // Called in View::on_hide()
    void shutdown_hardware();

    // Spectrum configuration management
    void set_spectrum_mode(SpectrumMode mode);
    SpectrumMode get_spectrum_mode() const { return spectrum_mode_; }

    // Frequency tuning hardware control
    bool tune_to_frequency(rf::Frequency frequency_hz);

    // Spectrum streaming control
    void start_spectrum_streaming();
    void stop_spectrum_streaming();
    bool is_spectrum_streaming_active() const { return spectrum_streaming_active_; }

    // RSSI data retrieval from hardware
    int32_t get_real_rssi_from_hardware(rf::Frequency target_frequency);

    // Spectrum message handlers (moving from main class)
    void handle_channel_spectrum_config(const ChannelSpectrumConfigMessage* const message);
    void handle_channel_spectrum(const ChannelSpectrum& spectrum);
    void process_channel_spectrum_data(const ChannelSpectrum& spectrum);

    // FIFO management for spectrum data
    ChannelSpectrumFIFO* get_fifo() const { return fifo_; }

    // Radio state access
    const RxRadioState& get_radio_state() const { return radio_state_; }
    RxRadioState& get_radio_state() { return radio_state_; }

private:
    // Spectrum configuration
    SpectrumMode spectrum_mode_;                    // Current spectrum settings

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
