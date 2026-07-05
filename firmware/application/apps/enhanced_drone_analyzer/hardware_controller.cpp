#include <cstdint>

#include "ch.h"

#include "hardware_controller.hpp"
#include "portapack.hpp"
#include "message.hpp"

namespace drone_analyzer {

// ============================================================================
// HardwareConfig Implementation
// ============================================================================

HardwareConfig::HardwareConfig() noexcept
    : center_frequency(0)
    , sample_rate(DEFAULT_SAMPLE_RATE_HZ)
    , gain(DEFAULT_GAIN)
    , lna_gain(DEFAULT_LNA_GAIN)
    , vga_gain(DEFAULT_VGA_GAIN) {
}

HardwareConfig::HardwareConfig(FreqHz freq, uint32_t rate, uint16_t g) noexcept
    : center_frequency(freq)
    , sample_rate(rate)
    , gain(g)
    , lna_gain(DEFAULT_LNA_GAIN)
    , vga_gain(DEFAULT_VGA_GAIN) {
}

// ============================================================================
// RssiSample Implementation
// ============================================================================

RssiSample::RssiSample() noexcept
    : rssi(RSSI_NOISE_FLOOR_DBM)
    , timestamp(0)
    , frequency(0) {
}

RssiSample::RssiSample(RssiValue r, SystemTime t, FreqHz f) noexcept
    : rssi(r)
    , timestamp(t)
    , frequency(f) {
}

// ============================================================================
// HardwareController Implementation
// ============================================================================

HardwareController::HardwareController() noexcept
    : state_(HardwareState::UNINITIALIZED)
    , config_()
    , current_frequency_(0)
    , last_error_(ErrorCode::SUCCESS)
    , retry_count_(0)
    , pll_locked_()
    , streaming_active_()
    , mutex_() {

    chMtxInit(&mutex_);
}

HardwareController::~HardwareController() noexcept {
    if (state_ != HardwareState::UNINITIALIZED) {
        (void)shutdown();
    }

}

ErrorCode HardwareController::initialize() noexcept {
    MutexLock<LockOrder::STATE_MUTEX> lock(mutex_);
    
    if (state_ != HardwareState::UNINITIALIZED) {
        return ErrorCode::INITIALIZATION_INCOMPLETE;
    }
    
    state_ = HardwareState::INITIALIZING;
    
    ErrorCode result = initialize_internal();
    
    if (result == ErrorCode::SUCCESS) {
        state_ = HardwareState::READY;
        current_frequency_ = config_.center_frequency;
        last_error_ = ErrorCode::SUCCESS;
    } else {
        state_ = HardwareState::ERROR;
        last_error_ = result;
    }
    
    return result;
}

ErrorCode HardwareController::initialize_internal() noexcept {
    return apply_config_internal(config_);
}

ErrorCode HardwareController::shutdown() noexcept {
    MutexLock<LockOrder::STATE_MUTEX> lock(mutex_);

    if (streaming_active_.test()) {
        (void)stop_streaming_internal();
    }

    state_ = HardwareState::UNINITIALIZED;
    pll_locked_.clear();
    streaming_active_.clear();

    return ErrorCode::SUCCESS;
}

ErrorCode HardwareController::tune_to_frequency(
    FreqHz frequency,
    uint32_t /* max_retries */
) noexcept {
    // Validate frequency
    ErrorCode validate_result = validate_frequency_internal(frequency);
    if (validate_result != ErrorCode::SUCCESS) {
        last_error_ = validate_result;
        return validate_result;
    }

    MutexLock<LockOrder::STATE_MUTEX> lock(mutex_);

    if (state_ != HardwareState::READY && state_ != HardwareState::STREAMING) {
        last_error_ = ErrorCode::HARDWARE_NOT_INITIALIZED;
        return ErrorCode::HARDWARE_NOT_INITIALIZED;
    }

    state_ = HardwareState::TUNING;

    // Tune to frequency
    ErrorCode tune_result = tune_internal(frequency);
    if (tune_result != ErrorCode::SUCCESS) {
        state_ = HardwareState::ERROR;
        last_error_ = tune_result;
        return tune_result;
    }

    // Wait for PLL stabilization
    chThdSleepMilliseconds(5);

    current_frequency_ = frequency;
    pll_locked_.set();
    state_ = streaming_active_.test() ? HardwareState::STREAMING : HardwareState::READY;
    last_error_ = ErrorCode::SUCCESS;
    return ErrorCode::SUCCESS;
}

ErrorCode HardwareController::tune_internal(FreqHz frequency) noexcept {
    rf::Frequency rf_freq = rf::Frequency(frequency);
    portapack::receiver_model.set_target_frequency(rf_freq);

    // LNA, VGA, RF amp are controlled by UI widgets (field_lna_/field_vga_/field_rf_amp_)
    // which write directly to portapack::receiver_model on user change.
    // Do NOT re-apply config gains here — it would override user settings on every
    // frequency hop during scanning.

    portapack::receiver_model.set_baseband_bandwidth(config_.sample_rate);

    return ErrorCode::SUCCESS;
}

bool HardwareController::check_pll_lock_internal() const noexcept {
    // Placeholder for PLL lock check
    // In actual implementation, this would read PLL lock status from hardware
    return pll_locked_.test();
}

ErrorCode HardwareController::start_spectrum_streaming() noexcept {
    MutexLock<LockOrder::STATE_MUTEX> lock(mutex_);
    
    if (state_ != HardwareState::READY && state_ != HardwareState::TUNING) {
        last_error_ = ErrorCode::HARDWARE_NOT_INITIALIZED;
        return ErrorCode::HARDWARE_NOT_INITIALIZED;
    }
    
    if (streaming_active_.test()) {
        return ErrorCode::SUCCESS;  // Already streaming
    }
    
    ErrorCode result = start_streaming_internal();
    
    if (result == ErrorCode::SUCCESS) {
        streaming_active_.set();
        state_ = HardwareState::STREAMING;
        last_error_ = ErrorCode::SUCCESS;
    } else {
        last_error_ = result;
    }
    
    return result;
}

ErrorCode HardwareController::start_streaming_internal() noexcept {
    portapack::receiver_model.set_modulation(ReceiverModel::Mode::SpectrumAnalysis);
    return ErrorCode::SUCCESS;
}

ErrorCode HardwareController::stop_spectrum_streaming() noexcept {
    MutexLock<LockOrder::STATE_MUTEX> lock(mutex_);

    if (!streaming_active_.test() && state_ != HardwareState::ERROR) {
        return ErrorCode::SUCCESS;  // Not streaming and not in error state
    }
    
    ErrorCode result = stop_streaming_internal();
    
    if (result == ErrorCode::SUCCESS) {
        streaming_active_.clear();
        state_ = HardwareState::READY;
        last_error_ = ErrorCode::SUCCESS;
    } else {
        last_error_ = result;
    }
    
    return result;
}

ErrorCode HardwareController::stop_streaming_internal() noexcept {
    // STUB: Actual spectrum streaming controlled by baseband:: API
    // in drone_scanner_ui.cpp. This method only tracks state.
    return ErrorCode::SUCCESS;
}

ErrorResult<RssiSample> HardwareController::get_rssi_sample() noexcept {
    MutexLock<LockOrder::STATE_MUTEX> lock(mutex_);
    
    if (state_ != HardwareState::STREAMING) {
        return ErrorResult<RssiSample>::failure(ErrorCode::HARDWARE_NOT_INITIALIZED);
    }
    
    return read_rssi_internal();
}

ErrorResult<RssiSample> HardwareController::read_rssi_internal() noexcept {
    // STUB: Actual RSSI extracted from spectrum data in scanner.cpp:extract_rssi()
    // This method returns noise floor as placeholder for API compatibility
    RssiSample sample;
    sample.timestamp = chTimeNow();
    sample.frequency = current_frequency_;
    sample.rssi = RSSI_NOISE_FLOOR_DBM;
    return ErrorResult<RssiSample>::success(sample);
}

ErrorResult<FreqHz> HardwareController::get_current_frequency() const noexcept {
    MutexLock<LockOrder::STATE_MUTEX> lock(mutex_);
    
    if (state_ == HardwareState::UNINITIALIZED) {
        return ErrorResult<FreqHz>::failure(ErrorCode::HARDWARE_NOT_INITIALIZED);
    }
    
    return ErrorResult<FreqHz>::success(current_frequency_);
}

HardwareState HardwareController::get_state() const noexcept {
    MutexTryLock<LockOrder::STATE_MUTEX> lock(mutex_);
    return lock.is_locked() ? state_ : HardwareState::UNINITIALIZED;
}

bool HardwareController::is_ready() const noexcept {
    MutexTryLock<LockOrder::STATE_MUTEX> lock(mutex_);
    if (!lock.is_locked()) return false;
    return state_ == HardwareState::READY || state_ == HardwareState::STREAMING;
}

bool HardwareController::is_streaming() const noexcept {
    return streaming_active_.test();
}

ErrorCode HardwareController::set_config(const HardwareConfig& config) noexcept {
    MutexLock<LockOrder::STATE_MUTEX> lock(mutex_);
    
    if (state_ != HardwareState::READY && state_ != HardwareState::STREAMING) {
        last_error_ = ErrorCode::HARDWARE_NOT_INITIALIZED;
        return ErrorCode::HARDWARE_NOT_INITIALIZED;
    }
    
    ErrorCode result = apply_config_internal(config);
    
    if (result == ErrorCode::SUCCESS) {
        config_ = config;
        last_error_ = ErrorCode::SUCCESS;
    } else {
        last_error_ = result;
    }
    
    return result;
}

HardwareConfig HardwareController::get_config() const noexcept {
    MutexLock<LockOrder::STATE_MUTEX> lock(mutex_);
    return config_;
}

ErrorCode HardwareController::set_gain(uint16_t gain) noexcept {
    ErrorCode validate_result = validate_gain_internal(gain);
    if (validate_result != ErrorCode::SUCCESS) {
        last_error_ = validate_result;
        return validate_result;
    }
    
    MutexLock<LockOrder::STATE_MUTEX> lock(mutex_);
    
    if (state_ != HardwareState::READY && state_ != HardwareState::STREAMING) {
        last_error_ = ErrorCode::HARDWARE_NOT_INITIALIZED;
        return ErrorCode::HARDWARE_NOT_INITIALIZED;
    }
    
    // Update gain in config
    config_.gain = gain;
    
    // Apply to hardware
    // Placeholder for gain setting
    // In actual implementation, this would write gain registers
    
    last_error_ = ErrorCode::SUCCESS;
    return ErrorCode::SUCCESS;
}

uint16_t HardwareController::get_gain() const noexcept {
    MutexLock<LockOrder::STATE_MUTEX> lock(mutex_);
    return config_.gain;
}

bool HardwareController::is_pll_locked() const noexcept {
    return pll_locked_.test();
}

ErrorCode HardwareController::reset() noexcept {
    MutexLock<LockOrder::STATE_MUTEX> lock(mutex_);

    if (streaming_active_.test()) {
        (void)stop_streaming_internal();
    }

    state_ = HardwareState::READY;
    pll_locked_.clear();
    streaming_active_.clear();
    retry_count_ = 0;

    HardwareConfig default_config;
    ErrorCode result = apply_config_internal(default_config);
    if (result == ErrorCode::SUCCESS) {
        config_ = default_config;
        current_frequency_ = config_.center_frequency;
        last_error_ = ErrorCode::SUCCESS;
    } else {
        last_error_ = result;
    }

    return result;
}

ErrorCode HardwareController::get_last_error() const noexcept {
    MutexTryLock<LockOrder::STATE_MUTEX> lock(mutex_);
    return lock.is_locked() ? last_error_ : ErrorCode::SUCCESS;
}

ErrorCode HardwareController::validate_frequency_internal(FreqHz frequency) const noexcept {
    if (frequency < MIN_FREQUENCY_HZ || frequency > MAX_FREQUENCY_HZ) {
        return ErrorCode::INVALID_PARAMETER;
    }
    
    return ErrorCode::SUCCESS;
}

ErrorCode HardwareController::validate_gain_internal(uint16_t gain) const noexcept {
    constexpr uint16_t MAX_GAIN = 40;
    if (gain > MAX_GAIN) {
        return ErrorCode::INVALID_PARAMETER;
    }
    
    return ErrorCode::SUCCESS;
}

ErrorCode HardwareController::apply_config_internal(const HardwareConfig& config) noexcept {
    portapack::receiver_model.set_sampling_rate(config.sample_rate);

    portapack::receiver_model.set_baseband_bandwidth(config.sample_rate);

    portapack::receiver_model.set_lna(config.lna_gain);
    portapack::receiver_model.set_vga(config.vga_gain);

    portapack::receiver_model.set_rf_amp(config.gain > 20);

    rf::Frequency rf_freq = rf::Frequency(config.center_frequency);
    portapack::receiver_model.set_target_frequency(rf_freq);

    return ErrorCode::SUCCESS;
}

ErrorCode HardwareController::handle_error_internal(ErrorCode error) noexcept {
    // Handle error based on type
    switch (error) {
        case ErrorCode::PLL_LOCK_FAILURE:
            // Fallback: use last known frequency
            pll_locked_.clear();
            return ErrorCode::SUCCESS;
        
        case ErrorCode::HARDWARE_TIMEOUT:
            // TODO: Implement retry logic with exponential backoff
            return ErrorCode::SUCCESS;
        
        case ErrorCode::HARDWARE_FAILURE:
            // Reset hardware
            return reset();
        
        default:
            return error;
    }
}

} // namespace drone_analyzer
