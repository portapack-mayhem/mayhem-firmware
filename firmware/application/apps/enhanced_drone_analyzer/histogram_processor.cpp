#include "histogram_processor.hpp"

namespace drone_analyzer {

// ============================================================================
// Constructor / Destructor
// ============================================================================

HistogramProcessor::HistogramProcessor() noexcept
    : bin_count_(HISTOGRAM_BIN_COUNT)
    , noise_floor_threshold_(HISTOGRAM_NOISE_FLOOR)
    , signal_floor_threshold_(HISTOGRAM_SIGNAL_THRESHOLD)
    , histogram_{}
    , statistics_{}
    , initialized_(false) {
}

// ============================================================================
// Initialization
// ============================================================================

ErrorCode HistogramProcessor::initialize(size_t bin_count) noexcept {
    // Validate bin count
    if (bin_count == 0 || bin_count > HISTOGRAM_BUFFER_SIZE) {
        return ErrorCode::INVALID_PARAMETER;
    }

    bin_count_ = bin_count;

    // Clear histogram
    for (size_t i = 0; i < HISTOGRAM_BUFFER_SIZE; ++i) {
        histogram_[i] = 0;
    }

    // Reset statistics
    statistics_.total_samples = 0;
    statistics_.peak_bin = 0;
    statistics_.peak_count = 0;
    statistics_.noise_floor = 0;
    statistics_.signal_floor = 0;
    statistics_.active_bins = 0;

    initialized_ = true;
    return ErrorCode::SUCCESS;
}

// ============================================================================
// Public Methods
// ============================================================================

ErrorCode HistogramProcessor::update_histogram(
    const uint8_t* data,
    size_t length
) noexcept {
    // Validate parameters
    ErrorCode error = validate_data_params(data, length);
    if (error != ErrorCode::SUCCESS) {
        return error;
    }

    // Reset histogram periodically to prevent monotonic overflow
    // After 256 samples, reset to maintain relative differences
    if (statistics_.total_samples > 0 && (statistics_.total_samples % 256) == 0) {
        for (size_t i = 0; i < HISTOGRAM_BUFFER_SIZE; ++i) {
            histogram_[i] = 0;
        }
    }

    // Bin data into histogram
    for (size_t i = 0; i < length; ++i) {
        const size_t bin = value_to_bin(data[i]);
        if (bin < bin_count_) {
            histogram_[bin]++;
        }
    }

    // Update statistics
    statistics_.total_samples += static_cast<uint32_t>(length);

    // Recalculate derived statistics
    statistics_.noise_floor = calculate_noise_floor();
    statistics_.signal_floor = calculate_signal_floor();
    statistics_.peak_bin = static_cast<uint16_t>(find_peak_bin());
    statistics_.peak_count = histogram_[statistics_.peak_bin];
    statistics_.active_bins = count_active_bins();

    return ErrorCode::SUCCESS;
}

size_t HistogramProcessor::calculate_bin_values(
    HistogramBin* bins,
    size_t max_bins
) const noexcept {
    if (bins == nullptr || max_bins == 0) {
        return 0;
    }

    // Calculate number of bins to return
    const size_t num_bins = (bin_count_ < max_bins) ? bin_count_ : max_bins;

    // Find maximum count for normalization
    uint16_t max_count = 0;
    for (size_t i = 0; i < bin_count_; ++i) {
        if (histogram_[i] > max_count) {
            max_count = histogram_[i];
        }
    }

    // Calculate bin values
    for (size_t i = 0; i < num_bins; ++i) {
        bins[i].count = histogram_[i];

        // Normalize to 0-255 range
        if (max_count > 0) {
            // Use integer arithmetic: (count * 255) / max_count
            // To avoid overflow, use 16.16 fixed-point multiplication
            const uint32_t scaled = (static_cast<uint32_t>(histogram_[i]) << 16) / max_count;
            bins[i].value = static_cast<uint8_t>((scaled * HISTOGRAM_MAX_VALUE) >> 16);
        } else {
            bins[i].value = 0;
        }
    }

    return num_bins;
}

size_t HistogramProcessor::get_histogram_data(
    uint16_t* histogram,
    size_t max_length
) const noexcept {
    if (histogram == nullptr || max_length == 0) {
        return 0;
    }

    // Calculate number of entries to return
    const size_t num_entries = (bin_count_ < max_length) ? bin_count_ : max_length;

    // Copy histogram data
    for (size_t i = 0; i < num_entries; ++i) {
        histogram[i] = histogram_[i];
    }

    return num_entries;
}

void HistogramProcessor::reset() noexcept {
    // Clear histogram
    for (size_t i = 0; i < HISTOGRAM_BUFFER_SIZE; ++i) {
        histogram_[i] = 0;
    }

    // Reset statistics
    statistics_.total_samples = 0;
    statistics_.peak_bin = 0;
    statistics_.peak_count = 0;
    statistics_.noise_floor = 0;
    statistics_.signal_floor = 0;
    statistics_.active_bins = 0;
}

void HistogramProcessor::get_statistics(HistogramStatistics& stats) const noexcept {
    stats = statistics_;
}

size_t HistogramProcessor::get_peak_bin() const noexcept {
    return static_cast<size_t>(statistics_.peak_bin);
}

uint16_t HistogramProcessor::get_peak_count() const noexcept {
    return statistics_.peak_count;
}

uint16_t HistogramProcessor::get_noise_floor() const noexcept {
    return statistics_.noise_floor;
}

uint16_t HistogramProcessor::get_signal_floor() const noexcept {
    return statistics_.signal_floor;
}

void HistogramProcessor::set_noise_floor_threshold(uint16_t threshold) noexcept {
    noise_floor_threshold_ = threshold;
}

void HistogramProcessor::set_signal_floor_threshold(uint16_t threshold) noexcept {
    signal_floor_threshold_ = threshold;
}

uint8_t HistogramProcessor::get_active_bin_count() const noexcept {
    return statistics_.active_bins;
}

// ============================================================================
// Private Methods
// ============================================================================

ErrorCode HistogramProcessor::validate_data_params(
    const uint8_t* data,
    size_t length
) const noexcept {
    if (data == nullptr) {
        return ErrorCode::BUFFER_INVALID;
    }

    if (length == 0) {
        return ErrorCode::BUFFER_EMPTY;
    }

    return ErrorCode::SUCCESS;
}

size_t HistogramProcessor::value_to_bin(uint8_t value) const noexcept {
    // Map value (0-255) to bin (0-bin_count-1)
    // Use integer arithmetic: bin = (value * bin_count) / 256
    // To avoid overflow, use bit shift approximation

    if (bin_count_ == HISTOGRAM_EXTENDED_SIZE) {
        return static_cast<size_t>(value);
    } else if (bin_count_ == HISTOGRAM_HALF_SIZE) {
        return static_cast<size_t>(value >> 1);
    } else if (bin_count_ == 64) {
        return static_cast<size_t>(value >> 2);
    } else if (bin_count_ == 32) {
        return static_cast<size_t>(value >> 3);
    } else if (bin_count_ == 16) {
        return static_cast<size_t>(value >> 4);
    } else if (bin_count_ == 8) {
        return static_cast<size_t>(value >> 5);
    } else if (bin_count_ == 4) {
        return static_cast<size_t>(value >> 6);
    } else if (bin_count_ == 2) {
        return static_cast<size_t>(value >> 7);
    } else {
        // General case: (value * bin_count) / 256
        // Use 16.16 fixed-point to avoid overflow
        const uint32_t scaled = (static_cast<uint32_t>(value) << 16) / HISTOGRAM_EXTENDED_SIZE;
        return static_cast<size_t>((scaled * bin_count_) >> 16);
    }
}

uint16_t HistogramProcessor::calculate_noise_floor() const noexcept {
    // Use percentile-based estimation (25th percentile)
    // Find the bin at the 25th percentile

    if (statistics_.total_samples == 0) {
        return 0;
    }

    // Calculate cumulative count
    const uint32_t target_count = statistics_.total_samples / 4;  // 25th percentile
    uint32_t cumulative = 0;

    for (size_t i = 0; i < bin_count_; ++i) {
        cumulative += histogram_[i];
        if (cumulative >= target_count) {
            return static_cast<uint16_t>(i);
        }
    }

    return 0;
}

uint16_t HistogramProcessor::calculate_signal_floor() const noexcept {
    // Use threshold-based detection
    // Signal floor is the first bin with count above threshold

    for (size_t i = 0; i < bin_count_; ++i) {
        if (histogram_[i] >= signal_floor_threshold_) {
            return static_cast<uint16_t>(i);
        }
    }

    return static_cast<uint16_t>(bin_count_);
}

size_t HistogramProcessor::find_peak_bin() const noexcept {
    uint16_t max_count = 0;
    size_t peak_bin = 0;

    for (size_t i = 0; i < bin_count_; ++i) {
        if (histogram_[i] > max_count) {
            max_count = histogram_[i];
            peak_bin = i;
        }
    }

    return peak_bin;
}

uint8_t HistogramProcessor::count_active_bins() const noexcept {
    uint8_t count = 0;

    for (size_t i = 0; i < bin_count_; ++i) {
        if (histogram_[i] > noise_floor_threshold_) {
            count++;
        }
    }

    return count;
}

size_t HistogramProcessor::normalize_histogram(
    uint8_t* normalized,
    size_t max_length
) const noexcept {
    if (normalized == nullptr || max_length == 0) {
        return 0;
    }

    // Find maximum count for normalization
    uint16_t max_count = 0;
    for (size_t i = 0; i < bin_count_; ++i) {
        if (histogram_[i] > max_count) {
            max_count = histogram_[i];
        }
    }

    // Calculate number of entries to return
    const size_t num_entries = (bin_count_ < max_length) ? bin_count_ : max_length;

    // Normalize histogram to 0-255 range
    if (max_count > 0) {
        for (size_t i = 0; i < num_entries; ++i) {
            // Use integer arithmetic: (count * 255) / max_count
            // To avoid overflow, use 16.16 fixed-point multiplication
            const uint32_t scaled = (static_cast<uint32_t>(histogram_[i]) << 16) / max_count;
            normalized[i] = static_cast<uint8_t>((scaled * HISTOGRAM_MAX_VALUE) >> 16);
        }
    } else {
        // All zeros
        for (size_t i = 0; i < num_entries; ++i) {
            normalized[i] = 0;
        }
    }

    return num_entries;
}

} // namespace drone_analyzer
