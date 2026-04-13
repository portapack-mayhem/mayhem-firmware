#ifndef HISTOGRAM_PROCESSOR_HPP
#define HISTOGRAM_PROCESSOR_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include "drone_types.hpp"
#include "constants.hpp"

namespace drone_analyzer {

/**
 * @brief Histogram bin data
 * @note Fixed-size structure, no heap allocation
 */
struct HistogramBin {
    uint16_t count;                      // Count in this bin
    uint8_t value;                        // Bin value (0-255)
    uint8_t reserved[5];                  // Reserved for future use (padding)
};

/**
 * @brief Histogram statistics
 * @note Fixed-size structure, no heap allocation
 */
struct HistogramStatistics {
    uint32_t total_samples;               // Total samples processed
    uint16_t peak_bin;                   // Peak bin index
    uint16_t peak_count;                 // Peak bin count
    uint16_t noise_floor;                // Noise floor level
    uint16_t signal_floor;               // Signal floor level
    uint8_t active_bins;                 // Number of active bins
    uint8_t reserved[9];                 // Reserved for future use (padding)
};

/**
 * @brief Histogram processor for spectrum analysis
 * @note No floating-point operations, uses integer arithmetic
 * @note No heap allocation, uses fixed-size arrays
 * @note Separated from UI layer (no UI dependencies)
 * @note Removed: complex histogram algorithms
 */
class HistogramProcessor {
public:
    /**
     * @brief Default constructor
     */
    HistogramProcessor() noexcept;

    /**
     * @brief Initialize histogram processor
     * @param bin_count Number of histogram bins
     * @return ErrorCode::SUCCESS if initialized, error code otherwise
     */
    [[nodiscard]] ErrorCode initialize(size_t bin_count) noexcept;

    /**
     * @brief Update histogram with new data
     * @param data Input data buffer
     * @param length Length of data buffer
     * @return ErrorCode::SUCCESS if updated, error code otherwise
     * @pre data must not be nullptr
     * @pre length must be > 0
     * @note Bins data into histogram based on value ranges
     */
    [[nodiscard]] ErrorCode update_histogram(
        const uint8_t* data,
        size_t length
    ) noexcept;

    /**
     * @brief Calculate bin values from histogram
     * @param bins Output bin array
     * @param max_bins Maximum number of bins to output
     * @return Number of bins calculated
     * @note Normalizes bin values for display
     */
    [[nodiscard]] size_t calculate_bin_values(
        HistogramBin* bins,
        size_t max_bins
    ) const noexcept;

    /**
     * @brief Get histogram data
     * @param histogram Output histogram buffer
     * @param max_length Maximum length of histogram buffer
     * @return Number of histogram entries
     * @note Returns raw histogram counts
     */
    [[nodiscard]] size_t get_histogram_data(
        uint16_t* histogram,
        size_t max_length
    ) const noexcept;

    /**
     * @brief Reset histogram
     * @note Clears all bins and statistics
     */
    void reset() noexcept;

    /**
     * @brief Get histogram statistics
     * @param stats Output statistics structure
     */
    void get_statistics(HistogramStatistics& stats) const noexcept;

    /**
     * @brief Get peak bin
     * @return Peak bin index
     */
    [[nodiscard]] size_t get_peak_bin() const noexcept;

    /**
     * @brief Get peak count
     * @return Peak bin count
     */
    [[nodiscard]] uint16_t get_peak_count() const noexcept;

    /**
     * @brief Get noise floor
     * @return Noise floor level
     */
    [[nodiscard]] uint16_t get_noise_floor() const noexcept;

    /**
     * @brief Get signal floor
     * @return Signal floor level
     */
    [[nodiscard]] uint16_t get_signal_floor() const noexcept;

    /**
     * @brief Set noise floor threshold
     * @param threshold Noise floor threshold
     */
    void set_noise_floor_threshold(uint16_t threshold) noexcept;

    /**
     * @brief Set signal floor threshold
     * @param threshold Signal floor threshold
     */
    void set_signal_floor_threshold(uint16_t threshold) noexcept;

    /**
     * @brief Get active bin count
     * @return Number of bins with counts above noise floor
     */
    [[nodiscard]] uint8_t get_active_bin_count() const noexcept;

private:
    /**
     * @brief Validate data parameters
     * @param data Data buffer
     * @param length Buffer length
     * @return ErrorCode::SUCCESS if valid, error code otherwise
     */
    [[nodiscard]] ErrorCode validate_data_params(
        const uint8_t* data,
        size_t length
    ) const noexcept;

    /**
     * @brief Calculate bin index for value
     * @param value Value to bin
     * @return Bin index
     * @note Maps value (0-255) to bin (0-bin_count-1)
     */
    [[nodiscard]] size_t value_to_bin(uint8_t value) const noexcept;

    /**
     * @brief Calculate noise floor from histogram
     * @return Noise floor level
     * @note Uses percentile-based estimation
     */
    [[nodiscard]] uint16_t calculate_noise_floor() const noexcept;

    /**
     * @brief Calculate signal floor from histogram
     * @return Signal floor level
     * @note Uses threshold-based detection
     */
    [[nodiscard]] uint16_t calculate_signal_floor() const noexcept;

    /**
     * @brief Find peak bin
     * @return Peak bin index
     */
    [[nodiscard]] size_t find_peak_bin() const noexcept;

    /**
     * @brief Count active bins
     * @return Number of bins with counts above noise floor
     */
    [[nodiscard]] uint8_t count_active_bins() const noexcept;

    /**
     * @brief Normalize histogram for display
     * @param normalized Output normalized histogram
     * @param max_length Maximum length of output buffer
     * @return Number of normalized values
     * @note Scales histogram to 0-255 range
     */
    [[nodiscard]] size_t normalize_histogram(
        uint8_t* normalized,
        size_t max_length
    ) const noexcept;

    // Configuration
    size_t bin_count_;
    uint16_t noise_floor_threshold_;
    uint16_t signal_floor_threshold_;

    // Histogram data (fixed-size, no heap allocation)
    std::array<uint16_t, HISTOGRAM_BUFFER_SIZE> histogram_;

    // Statistics
    HistogramStatistics statistics_;

    // State
    bool initialized_;
};

} // namespace drone_analyzer

#endif // HISTOGRAM_PROCESSOR_HPP
