#ifndef HARDWARE_CONTROLLER_HPP
#define HARDWARE_CONTROLLER_HPP

#include <cstdint>
#include <cstddef>
#include <ch.h>
#include "message.hpp"
#include "drone_types.hpp"
#include "locking.hpp"
#include "constants.hpp"

// Forward declaration for ReceiverModel to avoid circular dependency
class ReceiverModel;

namespace drone_analyzer {

/**
 * @brief Hardware state tracking
 */
enum class HardwareState : uint8_t {
    UNINITIALIZED = 0,
    INITIALIZING = 1,
    READY = 2,
    TUNING = 3,
    STREAMING = 4,
    ERROR = 5
};

/**
 * @brief Hardware configuration
 */
struct HardwareConfig {
    FreqHz center_frequency;
    uint32_t sample_rate;
    uint16_t gain;
    uint8_t lna_gain;
    uint8_t vga_gain;
    
    /**
     * @brief Default constructor
     */
    HardwareConfig() noexcept;
    
    /**
     * @brief Constructor with values
     */
    HardwareConfig(FreqHz freq, uint32_t rate, uint16_t g) noexcept;
};

/**
 * @brief RSSI sample data
 */
struct RssiSample {
    RssiValue rssi;
    SystemTime timestamp;
    FreqHz frequency;
    
    /**
     * @brief Default constructor
     */
    RssiSample() noexcept;
    
    /**
     * @brief Constructor with values
     */
    RssiSample(RssiValue r, SystemTime t, FreqHz f) noexcept;
};

/**
 * @brief Hardware controller for HackRF One
 * @note Abstraction layer for RF hardware operations
 * @note Thread-safe with mutex protection
 * @note Retry logic and fallback modes for error recovery
 */
class HardwareController {
public:
    /**
     * @brief Constructor
     */
    HardwareController() noexcept;
    
    /**
     * @brief Destructor
     */
    ~HardwareController() noexcept;
    
    // Delete copy and move operations
    HardwareController(const HardwareController&) = delete;
    HardwareController& operator=(const HardwareController&) = delete;
    HardwareController(HardwareController&&) = delete;
    HardwareController& operator=(HardwareController&&) = delete;
    
    /**
     * @brief Initialize hardware
     * @note Must be called before any other operations
     * @return ErrorCode::SUCCESS if initialized, error code otherwise
     */
    [[nodiscard]] ErrorCode initialize() noexcept;
    
    /**
     * @brief Shutdown hardware
     * @note Stops streaming and releases resources
     * @return ErrorCode::SUCCESS if shutdown, error code otherwise
     */
    [[nodiscard]] ErrorCode shutdown() noexcept;
    
    /**
     * @brief Tune to frequency with PLL lock verification
     * @param frequency Target frequency in Hz
     * @param max_retries Maximum retry attempts (default: 3)
     * @return ErrorCode::SUCCESS if tuned, error code otherwise
     */
    [[nodiscard]] ErrorCode tune_to_frequency(
        FreqHz frequency,
        uint32_t max_retries = MAX_HARDWARE_RETRIES
    ) noexcept;
    
    /**
     * @brief Start spectrum streaming
     * @return ErrorCode::SUCCESS if started, error code otherwise
     */
    [[nodiscard]] ErrorCode start_spectrum_streaming() noexcept;
    
    /**
     * @brief Stop spectrum streaming
     * @return ErrorCode::SUCCESS if stopped, error code otherwise
     */
    [[nodiscard]] ErrorCode stop_spectrum_streaming() noexcept;
    
    /**
     * @brief Get RSSI sample
     * @return ErrorResult containing RSSI sample or error
     */
    [[nodiscard]] ErrorResult<RssiSample> get_rssi_sample() noexcept;
    
    /**
     * @brief Get current frequency
     * @return ErrorResult containing current frequency or error
     */
    [[nodiscard]] ErrorResult<FreqHz> get_current_frequency() const noexcept;
    
    /**
     * @brief Get hardware state
     * @return Current hardware state
     */
    [[nodiscard]] HardwareState get_state() const noexcept;
    
    /**
     * @brief Check if hardware is ready
     * @return true if ready, false otherwise
     */
    [[nodiscard]] bool is_ready() const noexcept;
    
    /**
     * @brief Check if streaming is active
     * @return true if streaming, false otherwise
     */
    [[nodiscard]] bool is_streaming() const noexcept;
    
    /**
     * @brief Set hardware configuration
     * @param config Configuration to apply
     * @return ErrorCode::SUCCESS if configured, error code otherwise
     */
    [[nodiscard]] ErrorCode set_config(const HardwareConfig& config) noexcept;
    
    /**
     * @brief Get hardware configuration
     * @return Current configuration
     */
    [[nodiscard]] HardwareConfig get_config() const noexcept;
    
    /**
     * @brief Set gain
     * @param gain Gain value (0-40)
     * @return ErrorCode::SUCCESS if set, error code otherwise
     */
    [[nodiscard]] ErrorCode set_gain(uint16_t gain) noexcept;
    
    /**
     * @brief Get gain
     * @return Current gain value
     */
    [[nodiscard]] uint16_t get_gain() const noexcept;
    
    /**
     * @brief Check PLL lock status
     * @return true if locked, false otherwise
     */
    [[nodiscard]] bool is_pll_locked() const noexcept;
    
    /**
     * @brief Reset hardware to default state
     * @return ErrorCode::SUCCESS if reset, error code otherwise
     */
    [[nodiscard]] ErrorCode reset() noexcept;
    
    /**
     * @brief Get last error code
     * @return Last error code
     */
    [[nodiscard]] ErrorCode get_last_error() const noexcept;
    
private:
    /**
     * @brief Internal: Initialize hardware components
     * @note Called by initialize() with mutex held
     * @return ErrorCode::SUCCESS if initialized, error code otherwise
     */
    [[nodiscard]] ErrorCode initialize_internal() noexcept;
    
    /**
     * @brief Internal: Tune to frequency
     * @note Called by tune_to_frequency() with mutex held
     * @param frequency Target frequency in Hz
     * @return ErrorCode::SUCCESS if tuned, error code otherwise
     */
    [[nodiscard]] ErrorCode tune_internal(FreqHz frequency) noexcept;
    
    /**
     * @brief Internal: Check PLL lock status
     * @note Called by tune_to_frequency() for verification
     * @return true if locked, false otherwise
     */
    [[nodiscard]] bool check_pll_lock_internal() const noexcept;
    
    /**
     * @brief Internal: Start spectrum streaming
     * @note Called by start_spectrum_streaming() with mutex held
     * @return ErrorCode::SUCCESS if started, error code otherwise
     */
    [[nodiscard]] ErrorCode start_streaming_internal() noexcept;
    
    /**
     * @brief Internal: Stop spectrum streaming
     * @note Called by stop_spectrum_streaming() with mutex held
     * @return ErrorCode::SUCCESS if stopped, error code otherwise
     */
    [[nodiscard]] ErrorCode stop_streaming_internal() noexcept;
    
    /**
     * @brief Internal: Read RSSI from hardware
     * @note Called by get_rssi_sample() with mutex held
     * @return ErrorResult containing RSSI sample or error
     */
    [[nodiscard]] ErrorResult<RssiSample> read_rssi_internal() noexcept;
    
    /**
     * @brief Internal: Validate frequency
     * @param frequency Frequency to validate
     * @return ErrorCode::SUCCESS if valid, error code otherwise
     */
    [[nodiscard]] ErrorCode validate_frequency_internal(FreqHz frequency) const noexcept;
    
    /**
     * @brief Internal: Validate gain
     * @param gain Gain to validate
     * @return ErrorCode::SUCCESS if valid, error code otherwise
     */
    [[nodiscard]] ErrorCode validate_gain_internal(uint16_t gain) const noexcept;
    
    /**
     * @brief Internal: Apply configuration to hardware
     * @note Called by set_config() with mutex held
     * @param config Configuration to apply
     * @return ErrorCode::SUCCESS if applied, error code otherwise
     */
    [[nodiscard]] ErrorCode apply_config_internal(const HardwareConfig& config) noexcept;
    
    /**
     * @brief Internal: Handle hardware error
     * @param error Error code to handle
     * @return ErrorCode::SUCCESS if recovered, error code otherwise
     */
    [[nodiscard]] ErrorCode handle_error_internal(ErrorCode error) noexcept;
    
    // Hardware state
    HardwareState state_;
    
    // Current configuration
    HardwareConfig config_;
    
    // Current frequency
    FreqHz current_frequency_;
    
    // Last error code
    ErrorCode last_error_;
    
    // Retry counter
    uint32_t retry_count_;
    
    // PLL lock flag
    AtomicFlag pll_locked_;
    
    // Streaming active flag
    AtomicFlag streaming_active_;
    
    // Mutex for thread safety (LockOrder::STATE_MUTEX)
    mutable Mutex mutex_;
};

} // namespace drone_analyzer

#endif // HARDWARE_CONTROLLER_HPP
