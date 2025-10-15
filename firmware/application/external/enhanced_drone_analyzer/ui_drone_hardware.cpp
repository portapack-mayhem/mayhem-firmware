// ui_drone_hardware.cpp
// Hardware management implementation for Enhanced Drone Analyzer

#include "ui_drone_hardware.hpp"
#include "receiver_model.hpp"
#include <algorithm>

namespace ui::external_app::enhanced_drone_analyzer {

DroneHardwareController::DroneHardwareController(SpectrumMode mode)
    : spectrum_mode_(mode),
      radio_state_(ReceiverModel::Mode::SpectrumAnalysis), // RAII initialization, not moved
      message_handler_spectrum_config_{
          Message::ID::ChannelSpectrumConfig,
          [this](Message* const p) {
              const auto message = *reinterpret_cast<const ChannelSpectrumConfigMessage*>(p);
              fifo_ = message.fifo;
              handle_channel_spectrum_config(&message);
          }
      },
      message_handler_frame_sync_{
          Message::ID::DisplayFrameSync,
          [this](const Message* const) {
              if (fifo_) {
                  ChannelSpectrum channel_spectrum;
                  while (fifo_->out(channel_spectrum)) {
                      handle_channel_spectrum(channel_spectrum);
                  }
              }
          }
      }
{
    initialize_hardware();
}

DroneHardwareController::~DroneHardwareController() {
    shutdown_hardware();
}

void DroneHardwareController::initialize_hardware() {
    initialize_radio_state();
    initialize_spectrum_collector();
}

void DroneHardwareController::set_spectrum_mode(SpectrumMode mode) {
    spectrum_mode_ = mode;
    // Apply new configuration
    initialize_radio_state();
}

int32_t DroneHardwareController::get_configured_sampling_rate() const {
    return SPECTRUM_SAMPLING_RATES[static_cast<size_t>(spectrum_mode_)];
}

int32_t DroneHardwareController::get_configured_bandwidth() const {
    return SPECTRUM_BANDWIDTHS[static_cast<size_t>(spectrum_mode_)];
}

void DroneHardwareController::initialize_radio_state() {
    // FIX: Remove double RxRadioState initialization - RAII in constructor handles it
    // radio_state_ is already initialized with ReceiverModel::Mode::SpectrumAnalysis

    // SETUP RECEIVER LIKE SpectrumAnalysisModel, but configurable
    receiver_model.set_baseband_configuration({
        .mode = 4,                              // Spectrum mode = 4
        .sampling_rate = get_configured_sampling_rate(),
        .decimation_factor = 1,
    });
    receiver_model.set_baseband_bandwidth(get_configured_bandwidth());

    // CONFIGURE SQUELCH FOR SPECTRUM ANALYSIS (Following all spectrum apps)
    receiver_model.set_squelch_level(0);  // No squelch for spectrum analysis

    // FIX: Enable receiver FIRST, then tune (Looking Glass pattern)
    receiver_model.enable();

    // Now safe to tune frequency
    radio::set_tuning_frequency(433000000);  // 433 MHz ISM band center
}

void DroneHardwareController::initialize_spectrum_collector() {
    // PHASE 2: Initialize baseband spectrum streaming instead of custom FIFO
    // The message handler setup in constructor handles the spectrum data flow

    // Reset collection state
    last_valid_rssi_ = -120;
    spectrum_streaming_active_ = false;
}

void DroneHardwareController::cleanup_spectrum_collector() {
    // Following Looking Glass pattern: only reset cached RSSI values
    // Spectrum streaming is managed by on_hide(), not here
    last_valid_rssi_ = -120;
}

void DroneHardwareController::on_hardware_show() {
    // FIX: Follow Looking Glass/SpectrumAnalysis EXACT pattern
    // Receiver is already enabled in initialize_radio_state()

    // 1. Configure spectrum parameters BEFORE starting streaming (Looking Glass)
    baseband::set_spectrum(get_configured_bandwidth(), 0);  // 0 trigger for spectrum analysis

    // 2. Start spectrum streaming (ALWAYS do this in on_show())
    start_spectrum_streaming();
}

void DroneHardwareController::on_hardware_hide() {
    // FOLLOWING LOOKING GLASS PATTERN: Cleanup in on_hide()
    // 1. Stop spectrum streaming FIRST
    stop_spectrum_streaming();

    // 2. Disable receiver
    receiver_model.disable();
}

void DroneHardwareController::shutdown_hardware() {
    stop_spectrum_streaming();
    cleanup_spectrum_collector();
}

void DroneHardwareController::start_spectrum_streaming() {
    baseband::spectrum_streaming_start();
    spectrum_streaming_active_ = true;
}

void DroneHardwareController::stop_spectrum_streaming() {
    if (spectrum_streaming_active_) {
        baseband::spectrum_streaming_stop();
        spectrum_streaming_active_ = false;
    }
}

bool DroneHardwareController::tune_to_frequency(rf::Frequency frequency_hz) {
    // Validate frequency range for Portapack hardware
    if (frequency_hz < 50000000 || frequency_hz > 6000000000) {
        return false; // Out of range
    }

    // Hardware tuning
    radio::set_tuning_frequency(frequency_hz);
    chThdSleepMilliseconds(10); // Allow PLL to settle

    return true;
}

// Spectrum data processing - these were moved from the main UI class
void DroneHardwareController::handle_channel_spectrum_config(
    const ChannelSpectrumConfigMessage* const message)
{
    // Following Search/Looking Glass pattern for spectrum message handling
    // FIFO setup typically handled in baseband initialization
    (void)message;  // Suppress compiler warning for unused parameter
}

void DroneHardwareController::handle_channel_spectrum(const ChannelSpectrum& spectrum) {
    // STEP 2: IMPROVED RSSI CALCULATION (Search/Recon pattern)
    int32_t peak_rssi = -120;
    int32_t avg_rssi = 0;
    size_t valid_bins = 0;

    // Calculate both peak and average RSSI (Search app pattern)
    for (size_t bin_index = 0; bin_index < spectrum.db.size(); ++bin_index) {
        int32_t bin_rssi = spectrum.db[bin_index];
        if (bin_rssi >= -120 && bin_rssi <= -20) {  // Valid range
            if (bin_rssi > peak_rssi) {
                peak_rssi = bin_rssi;
            }
            avg_rssi += bin_rssi;
            valid_bins++;
        }
    }

    // Calculate average if we have valid bins (Level pattern)
    if (valid_bins > 0) {
        avg_rssi /= valid_bins;

        // STEP 3: RSSI FILTERING WITH WEIGHTED AVERAGE (Enhanced stability)
        // Use weighted average to smooth noise while maintaining responsiveness
        const float ALPHA = 0.3f;  // New reading weight - balance between stability and responsiveness
        last_valid_rssi_ = static_cast<int32_t>((ALPHA * avg_rssi) + ((1.0f - ALPHA) * last_valid_rssi_));
    }

    // Delegate further processing to specialized method
    process_channel_spectrum_data(spectrum);
}

void DroneHardwareController::process_channel_spectrum_data(const ChannelSpectrum& spectrum) {
    // Stop current spectrum streaming cycle (following other spectrum apps)
    baseband::spectrum_streaming_stop();

    // Allow scanning thread to process new RSSI data
    // This callback runs in hardware interrupt context, so keep it minimal

    // Restart streaming if still needed
    if (spectrum_streaming_active_) {
        baseband::spectrum_streaming_start();
    }

    (void)spectrum;  // Suppress unused parameter warning
}

int32_t DroneHardwareController::get_real_rssi_from_hardware(rf::Frequency target_frequency) {
    // PHASE 3: Now uses real spectrum data from spectrum callbacks
    return last_valid_rssi_;  // Real hardware RSSI from spectrum analysis
}

} // namespace ui::external_app::enhanced_drone_analyzer
