#include <cstdint>
#include <algorithm>

#include "ch.h"

#include "scanner.hpp"
#include "receiver_model.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"

namespace drone_analyzer {

/**
 * @brief Convert a spectrum.db uint8_t value to calibrated dBm (with cached gain)
 * @param value Raw spectrum.db value (0-255, from baseband)
 * @param total_gain LNA + VGA + (RF_AMP enabled ? RF_AMP_GAIN_DB : 0) in dB
 * @return RSSI in dBm, gain-compensated
 * @note Encoding: spectrum.db = clamp(dBV*5 + 255, 0, 255) => dBV_norm = (value - 255) / 5
 * @note dBm = dBV_norm - total_gain
 */
static int32_t spectrum_value_to_dbm(const uint8_t value, int32_t total_gain) noexcept {
    const int32_t dbv_norm = (static_cast<int32_t>(value) - 255) / 5;
    int32_t rssi = dbv_norm - total_gain;
    if (rssi < RSSI_MIN_DBM) rssi = RSSI_MIN_DBM;
    if (rssi > RSSI_MAX_DBM) rssi = RSSI_MAX_DBM;
    return rssi;
}

/**
 * @brief Read total gain from hardware (SPI access).
 * @note Call once and pass to spectrum_value_to_dbm() to avoid repeated SPI reads.
 */
static int32_t get_current_total_gain() noexcept {
    return static_cast<int32_t>(portapack::receiver_model.lna()) +
           static_cast<int32_t>(portapack::receiver_model.vga()) +
           (portapack::receiver_model.rf_amp() ? RF_AMP_GAIN_DB : 0);
}

/**
 * @brief RSSI extraction — scan usable bins with edge skip
 */
static int32_t extract_rssi(const ChannelSpectrum& spectrum) noexcept {
    uint8_t peak = 0;

    for (size_t i = FFT_EDGE_SKIP; i < FFT_DC_SPIKE_START; ++i) {
        if (spectrum.db[i] > peak) peak = spectrum.db[i];
    }
    for (size_t i = FFT_DC_SPIKE_END; i < (FFT_BIN_COUNT - FFT_EDGE_SKIP); ++i) {
        if (spectrum.db[i] > peak) peak = spectrum.db[i];
    }

    const int32_t total_gain = get_current_total_gain();
    return spectrum_value_to_dbm(peak, total_gain);
}

/**
 * @brief Compute k-th percentile via quickselect (O(n) average, in-place)
 * @param buf Buffer to partition (will be modified)
 * @param count Number of elements in buffer
 * @param percentile Percentile to compute (0-100, where 25 = 25th percentile)
 * @return Percentile value (0 if count == 0)
 * @note 25th percentile is more robust than median for noise floor estimation
 *       when signal occupies >50% of bins (WiFi-dense environments).
 *       Median becomes signal-biased in that case; 25th percentile stays near
 *       the true noise floor.
 */
static uint8_t quickselect_percentile(uint8_t* buf, size_t count, uint8_t percentile) noexcept {
    if (count == 0) return 0;
    const size_t k = (count * static_cast<size_t>(percentile)) / 100;
    const size_t k_safe = (k < count) ? k : count - 1;
    size_t left = 0;
    size_t right = count - 1;
    while (left < right) {
        const size_t pivot_idx = left + (right - left) / 2;
        const uint8_t pivot = buf[pivot_idx];
        buf[pivot_idx] = buf[right];
        buf[right] = pivot;
        size_t store = left;
        for (size_t i = left; i < right; ++i) {
            if (buf[i] < pivot) {
                const uint8_t t = buf[store];
                buf[store] = buf[i];
                buf[i] = t;
                ++store;
            }
        }
        buf[right] = buf[store];
        buf[store] = pivot;
        if (store == k_safe) break;
        if (store < k_safe) left = store + 1;
        else right = store - 1;
    }
    return buf[k_safe];
}

// ============================================================================
// ScanConfig Implementation
// ============================================================================

ScanConfig::ScanConfig() noexcept
    : mode(DEFAULT_SCANNING_MODE)
    , start_frequency(MIN_FREQUENCY_HZ)
    , end_frequency(MAX_FREQUENCY_HZ)
    , scan_interval_ms(SCAN_CYCLE_INTERVAL_MS)
    , rssi_threshold_dbm(RSSI_DETECTION_THRESHOLD_DBM)
    , stale_timeout_ms(DRONE_REMOVAL_TIMEOUT_MS)
    , sweep_start_freq(SWEEP_DEFAULT_START_HZ)
    , sweep_end_freq(SWEEP_DEFAULT_END_HZ)
    , sweep_step_freq(17813000)
    , dwell_enabled(true)
    , confirm_count_enabled(true)
    , noise_blacklist_enabled(true)
    , spectrum_detection_enabled(true)
    , median_enabled(true)
    , spectrum_margin(DEFAULT_SPECTRUM_MARGIN)
    , spectrum_min_width(DEFAULT_SPECTRUM_MIN_WIDTH)
    , spectrum_max_width(DEFAULT_SPECTRUM_MAX_WIDTH)
    , spectrum_peak_sharpness(DEFAULT_SPECTRUM_PEAK_SHARPNESS)
    , spectrum_peak_ratio(DEFAULT_SPECTRUM_PEAK_RATIO)
    , spectrum_valley_depth(DEFAULT_SPECTRUM_VALLEY_DEPTH)
    , spectrum_flatness(DEFAULT_SPECTRUM_FLATNESS)
    , spectrum_symmetry(DEFAULT_SPECTRUM_SYMMETRY)
    , neighbor_margin_db(DEFAULT_NEIGHBOR_MARGIN_DB)
    , rssi_variance_enabled(true)
    , confirm_count(DEFAULT_CONFIRM_COUNT)
    , pattern_matching_enabled(true)
    , pattern_similarity_threshold(DEFAULT_PATTERN_SIMILARITY_THRESHOLD) {
    // sweep2/3/4 fields use in-class defaults: disabled
    // mahalanobis_enabled uses in-class default (true, FPV-optimized)
}

ScanConfig::ScanConfig(ScanningMode m, FreqHz start, FreqHz end) noexcept
    : mode(m)
    , start_frequency(start)
    , end_frequency(end)
    , scan_interval_ms(SCAN_CYCLE_INTERVAL_MS)
    , rssi_threshold_dbm(RSSI_DETECTION_THRESHOLD_DBM)
    , stale_timeout_ms(DRONE_REMOVAL_TIMEOUT_MS)
    , sweep_start_freq(SWEEP_DEFAULT_START_HZ)
    , sweep_end_freq(SWEEP_DEFAULT_END_HZ)
    , sweep_step_freq(17813000)
    , dwell_enabled(true)
    , confirm_count_enabled(true)
    , noise_blacklist_enabled(true)
    , spectrum_detection_enabled(true)
    , median_enabled(true)
    , spectrum_margin(DEFAULT_SPECTRUM_MARGIN)
    , spectrum_min_width(DEFAULT_SPECTRUM_MIN_WIDTH)
    , spectrum_max_width(DEFAULT_SPECTRUM_MAX_WIDTH)
    , spectrum_peak_sharpness(DEFAULT_SPECTRUM_PEAK_SHARPNESS)
    , spectrum_peak_ratio(DEFAULT_SPECTRUM_PEAK_RATIO)
    , spectrum_valley_depth(DEFAULT_SPECTRUM_VALLEY_DEPTH)
    , spectrum_flatness(DEFAULT_SPECTRUM_FLATNESS)
    , spectrum_symmetry(DEFAULT_SPECTRUM_SYMMETRY)
    , neighbor_margin_db(DEFAULT_NEIGHBOR_MARGIN_DB)
    , rssi_variance_enabled(true)
    , confirm_count(DEFAULT_CONFIRM_COUNT)
    , pattern_matching_enabled(true)
    , pattern_similarity_threshold(DEFAULT_PATTERN_SIMILARITY_THRESHOLD) {
    // sweep2/3/4 fields use in-class defaults: disabled
    // mahalanobis_enabled uses in-class default (true, FPV-optimized)
}



// ========================================================================
// ScanStatistics Implementation
// ============================================================================

ScanStatistics::ScanStatistics() noexcept
    : total_scan_cycles(0)
    , successful_cycles(0)
    , failed_cycles(0)
    , drones_detected(0)
    , max_rssi_dbm(RSSI_NOISE_FLOOR_DBM) {
}

void ScanStatistics::reset() noexcept {
    total_scan_cycles = 0;
    successful_cycles = 0;
    failed_cycles = 0;
    drones_detected = 0;
    max_rssi_dbm = RSSI_NOISE_FLOOR_DBM;
}

// ============================================================================
// DroneScanner Implementation
// ============================================================================

DroneScanner::DroneScanner(DatabaseManager& database, HardwareController& hardware) noexcept
    : database_(database)
    , hardware_(hardware)
    , state_(ScannerState::IDLE)
    , config_()
    , freq_lock_count_{0}
    , locked_frequency_{0}
    , track_start_time_{0}
    , current_drone_type_{'\0', '\0', '\0', '\0', '\0'}
    , drone_type_valid_{false}
    , statistics_()
    , tracked_drones_()
    , tracked_count_{0}
    , current_frequency_{0}
    , pending_frequency_{0}
    , pending_count_{0}
    , missed_lock_count_{0}
    , noise_blacklist_{}
    , last_scan_time_{0}
    , scanning_active_()
    , alert_callback_(nullptr)
    , mutex_()
    , state_transition_allowed_()
    , force_resume_flag_()
    , dwell_request_()
    , dwell_cycles_{0}
    , lock_start_time_{0}
    , confirm_start_time_{0}
    , spectrum_sort_buf_{}
    , lock_timeout_count_{0}
    , sweep_usable_buf_{}
    , alert_callback_in_progress_()
    , rssi_detector_()
    , histogram_processor_()
    , rssi_median_filter_()
    , median_filter_enabled_{false}
    , neighbor_margin_checker_()
    , mahalanobis_detector_()
    , pattern_matcher_()
    , pattern_manager_() {

    // Initialize mutex
    chMtxInit(&mutex_);

    (void)rssi_detector_.initialize(RSSI_DETECTION_THRESHOLD_DBM);
    (void)histogram_processor_.initialize(HISTOGRAM_BIN_COUNT);
}

DroneScanner::~DroneScanner() noexcept {
    // Stop scanning if active
    if (scanning_active_.test()) {
        (void)stop_scanning();
    }

    // Note: Do NOT call chMtxDeinit - it doesn't exist in ChibiOS
}

ErrorCode DroneScanner::initialize() noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);

    if (state_ != ScannerState::IDLE) {
        return ErrorCode::INITIALIZATION_INCOMPLETE;
    }

    ErrorCode hw_result = hardware_.initialize();
    if (hw_result != ErrorCode::SUCCESS) {
        return hw_result;
    }

    // Try to get first frequency from database
    ErrorResult<FreqHz> freq_result = database_.get_next_frequency(0);
    if (freq_result.has_value()) {
        current_frequency_ = freq_result.value();
    } else {
        // Database empty or not loaded — scanner still works on default freq
        current_frequency_ = MIN_FREQUENCY_HZ;
    }

    state_ = ScannerState::IDLE;
    statistics_.reset();

    // Load patterns from SD card
    const ErrorCode patterns_err = pattern_manager_.load_patterns();
    if (patterns_err == ErrorCode::SUCCESS) {
        // Set patterns to matcher
        pattern_matcher_.set_patterns(
            pattern_manager_.get_patterns_array(),
            pattern_manager_.get_pattern_count()
        );
    }

    return ErrorCode::SUCCESS;
}

// ============================================================================
// Fast Scanner Integration Getters
// ============================================================================

uint32_t DroneScanner::get_freq_lock_count() const noexcept {
    MutexTryLock<LockOrder::DATA_MUTEX> lock(mutex_);
    if (!lock.is_locked()) {
        return __atomic_load_n(&freq_lock_count_, __ATOMIC_RELAXED);
    }
    return freq_lock_count_;
}

void DroneScanner::set_freq_lock_count(uint32_t count) noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    freq_lock_count_ = count;
}

FreqHz DroneScanner::get_locked_frequency() const noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    return locked_frequency_;
}

// ============================================================================
// Scanner Control Methods (Restored for functionality)
// ============================================================================

ErrorCode DroneScanner::start_scanning() noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);

    if (state_ == ScannerState::SCANNING) {
        return ErrorCode::SUCCESS;
    }

    ErrorCode hw_result = hardware_.start_spectrum_streaming();
    if (hw_result != ErrorCode::SUCCESS) {
        return hw_result;
    }

    state_ = ScannerState::SCANNING;
    scanning_active_.set();

    return ErrorCode::SUCCESS;
}

ErrorCode DroneScanner::stop_scanning() noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);

    if (state_ == ScannerState::IDLE) {
        return ErrorCode::SUCCESS;
    }

    state_ = ScannerState::IDLE;
    scanning_active_.clear();

    (void)hardware_.stop_spectrum_streaming();

    return ErrorCode::SUCCESS;
}

ErrorCode DroneScanner::pause_scanning() noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    
    if (state_ != ScannerState::SCANNING) {
        return ErrorCode::SUCCESS;
    }
    
    state_ = ScannerState::PAUSED;
    return ErrorCode::SUCCESS;
}

ErrorCode DroneScanner::resume_scanning() noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    
    if (state_ != ScannerState::PAUSED) {
        return ErrorCode::SUCCESS;
    }
    
    state_ = ScannerState::SCANNING;
    return ErrorCode::SUCCESS;
}

void DroneScanner::force_resume_scanning() noexcept {
    // Set flag — actual state transition happens inside perform_scan_cycle()
    // under mutex protection. This avoids data race on state_ between
    // scanner thread and UI thread.
    force_resume_flag_.set();
}

void DroneScanner::request_dwell() noexcept {
    // Set dwell request — scanner thread will check this BEFORE hopping frequency.
    // This is the critical link: UI detects signal → requests dwell → scanner holds.
    dwell_request_.set();
}

bool DroneScanner::try_consume_force_resume_flag() noexcept {
    if (!force_resume_flag_.test_and_set()) {
        return false;
    }
    force_resume_flag_.clear();
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    if (state_ == ScannerState::LOCKING || state_ == ScannerState::TRACKING) {
        state_ = ScannerState::SCANNING;
        // Clear lock timing when force-resuming (consistent with perform_scan_cycle_internal)
        lock_start_time_ = 0;
        confirm_start_time_ = 0;
        return true;
    }
    return false;
}

void DroneScanner::remove_drone_on_frequency(FreqHz frequency) noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    for (size_t i = 0; i < tracked_count_; ++i) {
        if (tracked_drones_[i].frequency == frequency) {
            tracked_count_--;
            if (i < tracked_count_) {
                tracked_drones_[i] = tracked_drones_[tracked_count_];
            }
            return;
        }
    }
}

void DroneScanner::increment_noise_count(FreqHz frequency) noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    increment_noise_count_internal(frequency);
}

void DroneScanner::increment_noise_count_internal(FreqHz frequency) noexcept {
    // Find existing entry or first empty slot
    // @pre Caller holds DATA_MUTEX
    size_t empty_slot = MAX_NOISE_ENTRIES;
    for (size_t i = 0; i < MAX_NOISE_ENTRIES; ++i) {
        if (noise_blacklist_[i].freq == frequency) {
            if (noise_blacklist_[i].count < 255) {
                noise_blacklist_[i].count++;
            }
            return;
        }
        if (noise_blacklist_[i].freq == 0 && empty_slot == MAX_NOISE_ENTRIES) {
            empty_slot = i;
        }
    }
    // New entry
    if (empty_slot < MAX_NOISE_ENTRIES) {
        noise_blacklist_[empty_slot].freq = frequency;
        noise_blacklist_[empty_slot].count = 1;
    }
}

void DroneScanner::reset_noise_count(FreqHz frequency) noexcept {
    for (size_t i = 0; i < MAX_NOISE_ENTRIES; ++i) {
        if (noise_blacklist_[i].freq == frequency) {
            noise_blacklist_[i].freq = 0;
            noise_blacklist_[i].count = 0;
            return;
        }
    }
}

bool DroneScanner::is_blacklisted(FreqHz frequency) const noexcept {
    for (size_t i = 0; i < MAX_NOISE_ENTRIES; ++i) {
        if (noise_blacklist_[i].freq == frequency) {
            return noise_blacklist_[i].count >= NOISE_BLACKLIST_THRESHOLD;
        }
    }
    return false;
}

ErrorCode DroneScanner::perform_scan_cycle() noexcept {
    if (!scanning_active_.test()) {
        return ErrorCode::SUCCESS;
    }
    
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    
    if (state_ == ScannerState::PAUSED || state_ == ScannerState::IDLE) {
        return ErrorCode::SUCCESS;
    }
    
    return perform_scan_cycle_internal();
}

ErrorCode DroneScanner::perform_scan_cycle_internal() noexcept {
    // Check force-resume flag (set when max dwell expires)
    if (force_resume_flag_.test_and_set()) {
        force_resume_flag_.clear();
        if (state_ == ScannerState::LOCKING || state_ == ScannerState::TRACKING) {
            state_ = ScannerState::SCANNING;
            // Clear lock timing when force-resuming
            lock_start_time_ = 0;
            confirm_start_time_ = 0;
        }
        dwell_cycles_ = 0;
        return ErrorCode::SUCCESS;
    }

    // ABSOLUTE LOCK TIMEOUT: Force exit from LOCKING/TRACKING after MAX_LOCK_DURATION_MS
    // This prevents infinite lock on noisy frequencies where signal intermittently exceeds threshold
    if (state_ == ScannerState::LOCKING || state_ == ScannerState::TRACKING) {
        if (lock_start_time_ != 0) {
            const uint32_t lock_duration = chTimeNow() - lock_start_time_;
            if (lock_duration >= MAX_LOCK_DURATION_MS) {
                // Lock timeout exceeded - force transition to SCANNING
                state_ = ScannerState::SCANNING;
                freq_lock_count_ = 0;
                locked_frequency_ = 0;
                lock_start_time_ = 0;
                confirm_start_time_ = 0;
                dwell_cycles_ = 0;
                lock_timeout_count_++;
                // Continue to frequency hop (fall through to dwell check)
            }
        } else {
            // Just entered LOCKING/TRACKING state - start timing
            lock_start_time_ = chTimeNow();
        }
    } else {
        // Not locked - clear timing
        lock_start_time_ = 0;
        confirm_start_time_ = 0;
    }

    // Check dwell request from UI thread (signal detected, hold frequency)
    if (dwell_request_.test_and_set()) {
        dwell_request_.clear();
        dwell_cycles_ = 1;  // Start at 1 so should_dwell triggers immediately
    }

    // Dwell: if UI requested hold or state is LOCKING/TRACKING, skip frequency hop
    const bool should_dwell = config_.dwell_enabled &&
        (dwell_cycles_ > 0 || state_ == ScannerState::LOCKING || state_ == ScannerState::TRACKING);

    if (should_dwell) {
        dwell_cycles_++;

        // Max dwell: 400ms total (8 cycles × 50ms).
        // Enough for median filter to warm up (4 samples) + 4 more frames for
        // reliable lock accumulation. At 60fps, 8 frames ≈ 8 increments of
        // freq_lock_count_ toward MAX_FREQ_LOCK (10) → TRACKING transition.
        static constexpr uint8_t LOCAL_MAX_DWELL_CYCLES = 8;
        const uint8_t max_dwell = config_.confirm_count_enabled
            ? LOCAL_MAX_DWELL_CYCLES : (LOCAL_MAX_DWELL_CYCLES / 2);

        if (dwell_cycles_ >= max_dwell) {
            // Max dwell reached — force resume scanning
            if (config_.noise_blacklist_enabled) {
                const FreqHz locked_freq = locked_frequency_;
                // Find or add noise entry
                size_t empty_slot = MAX_NOISE_ENTRIES;
                for (size_t i = 0; i < MAX_NOISE_ENTRIES; ++i) {
                    if (noise_blacklist_[i].freq == locked_freq) {
                        if (noise_blacklist_[i].count < 255) {
                            noise_blacklist_[i].count++;
                        }
                        break;
                    }
                    if (noise_blacklist_[i].freq == 0 && empty_slot == MAX_NOISE_ENTRIES) {
                        empty_slot = i;
                    }
                }
                if (empty_slot < MAX_NOISE_ENTRIES && locked_freq != 0) {
                    noise_blacklist_[empty_slot].freq = locked_freq;
                    noise_blacklist_[empty_slot].count = 1;
                }
                // Remove drone on this frequency
                for (size_t i = 0; i < tracked_count_; ++i) {
                    if (tracked_drones_[i].frequency == locked_freq) {
                        tracked_count_--;
                        if (i < tracked_count_) {
                            tracked_drones_[i] = tracked_drones_[tracked_count_];
                        }
                        break;
                    }
                }
            }
            force_resume_flag_.set();
            dwell_cycles_ = 0;
        }

        // Stay on current frequency — do NOT hop
        statistics_.successful_cycles++;
        return ErrorCode::SUCCESS;
    }

    // No dwell — normal frequency hop
    dwell_cycles_ = 0;
    statistics_.total_scan_cycles++;

    apply_rssi_decay();

    // Try to get next frequency from database
    ErrorResult<FreqHz> freq_result = database_.get_next_frequency(current_frequency_);

    if (freq_result.has_value()) {
        current_frequency_ = freq_result.value();

        // Skip blacklisted frequencies
        if (config_.noise_blacklist_enabled) {
            bool found_clean_freq = false;
            for (size_t skip_count = 0; skip_count < MAX_NOISE_ENTRIES; ++skip_count) {
                if (!is_blacklisted(current_frequency_)) {
                    found_clean_freq = true;
                    break;
                }
                freq_result = database_.get_next_frequency(current_frequency_);
                if (!freq_result.has_value()) {
                    break;  // Database exhausted
                }
                current_frequency_ = freq_result.value();
            }
            
            if (!found_clean_freq) {
                // All frequencies are blacklisted - fallback to minimum frequency
                current_frequency_ = MIN_FREQUENCY_HZ;
            }
        }
    } else {
        // Database empty — sweep through frequency range
        if (current_frequency_ < MIN_FREQUENCY_HZ || current_frequency_ >= MAX_FREQUENCY_HZ) {
            current_frequency_ = MIN_FREQUENCY_HZ;
        } else {
            current_frequency_ += FREQUENCY_STEP_HZ;
            if (current_frequency_ > MAX_FREQUENCY_HZ) {
                current_frequency_ = MIN_FREQUENCY_HZ;
            }
        }
    }

    ErrorCode tune_result = hardware_.tune_to_frequency(current_frequency_);
    if (tune_result != ErrorCode::SUCCESS) {
        statistics_.failed_cycles++;
        return tune_result;
    }

    rssi_median_filter_.reset();
    neighbor_margin_checker_.reset();

    if (current_frequency_ != pending_frequency_) {
        pending_frequency_ = 0;
        pending_count_ = 0;
    }

    statistics_.successful_cycles++;
    return ErrorCode::SUCCESS;
}

ErrorCode DroneScanner::update_tracked_drones(
    FreqHz frequency,
    RssiValue rssi,
    SystemTime timestamp
) noexcept {
    if (frequency < MIN_FREQUENCY_HZ || frequency > MAX_FREQUENCY_HZ) {
        return ErrorCode::INVALID_PARAMETER;
    }

    if (rssi < RSSI_MIN_DBM || rssi > RSSI_MAX_DBM) {
        return ErrorCode::INVALID_PARAMETER;
    }

    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);

    return update_tracked_drone_internal(frequency, rssi, timestamp);
}

ErrorResult<RssiValue> DroneScanner::process_spectrum_data(
    const ChannelSpectrum& spectrum,
    FreqHz current_frequency
) noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);

    if (current_frequency == 0) {
        return ErrorResult<RssiValue>::failure(ErrorCode::INVALID_PARAMETER);
    }

    const int32_t rssi = extract_rssi(spectrum);

    int32_t effective_rssi = rssi;
    bool signal_detected = false;

    if (config_.spectrum_detection_enabled) {
        // Spectrum-only: shape analysis gates detection. If the signal
        // fails all shape filters (margin, width, sharpness, flatness, symmetry, etc.)
        // it is NOT a drone — reject regardless of raw RSSI.
        int32_t spectrum_rssi = RSSI_MIN_DBM;
        if (analyze_spectrum_shape(spectrum, spectrum_rssi)) {
            // Shape passed — still enforce RSSI threshold (sens setting)
            if (rssi > config_.rssi_threshold_dbm) {
                signal_detected = true;
                if (spectrum_rssi > effective_rssi) effective_rssi = spectrum_rssi;
            }
        }
    } else {
        signal_detected = (rssi > config_.rssi_threshold_dbm);
    }

    if (signal_detected) {
        const ErrorCode err = update_tracked_drone_internal(
            current_frequency,
            effective_rssi,
            chTimeNow()
        );

        if (err != ErrorCode::SUCCESS) {
            return ErrorResult<RssiValue>::failure(err);
        }
    }

    return ErrorResult<RssiValue>::success(rssi);
}

FreqHz DroneScanner::get_spectrum_frequency() noexcept {
    MutexTryLock<LockOrder::DATA_MUTEX> lock(mutex_);
    if (lock.is_locked()) {
        return current_frequency_;
    }
    return 0;
}

ErrorCode DroneScanner::process_spectrum_message(const ChannelSpectrum& spectrum, FreqHz frequency) noexcept {
    MutexTryLock<LockOrder::DATA_MUTEX> lock(mutex_);

    if (!lock.is_locked()) {
        return ErrorCode::MUTEX_LOCK_FAILED;
    }

    if (frequency == 0) {
        return ErrorCode::INVALID_PARAMETER;
    }

    // ChannelSpectrum has fixed-size array (256 bins), no empty check needed
    // Feed spectrum to histogram processor for noise floor analysis
    (void)histogram_processor_.update_histogram(spectrum.db.data(), spectrum.db.size());

    const int32_t raw_rssi = extract_rssi(spectrum);

    // Median filter: reject single-sample noise spikes
    // Feed every sample; use median only when enabled and filter is warm
    rssi_median_filter_.add(raw_rssi);
    const int32_t rssi = (median_filter_enabled_ && rssi_median_filter_.is_warm())
        ? rssi_median_filter_.get_median()
        : raw_rssi;

    const SystemTime now = chTimeNow();

    // ----- RSSI Hysteresis (Schmitt trigger) -----
    // Reset hysteresis state when frequency changes (new hop or new lock target)
    if (frequency != last_hysteresis_freq_) {
        signal_present_ = false;
        last_hysteresis_freq_ = frequency;
    }

    // Effective threshold: 2 dB harder to turn ON, 2 dB easier to stay ON
    // Prevents signal toggling at the threshold boundary (8-bit FFT quantization
    // causes ±1 bin fluctuation = ~0.19 dB, enough to toggle detection)
    const int32_t rssi_threshold = signal_present_
        ? (config_.rssi_threshold_dbm - RSSI_HYSTERESIS_DB)
        : (config_.rssi_threshold_dbm + RSSI_HYSTERESIS_DB);

    int32_t effective_rssi = rssi;
    bool signal_detected = false;

    if (config_.spectrum_detection_enabled) {
        // Spectrum-only: shape analysis gates detection
        int32_t spectrum_rssi = RSSI_MIN_DBM;
        if (analyze_spectrum_shape(spectrum, spectrum_rssi)) {
            // Shape passed — enforce RSSI threshold with hysteresis
            if (rssi > rssi_threshold) {
                signal_detected = true;
                signal_present_ = true;
                if (spectrum_rssi > effective_rssi) effective_rssi = spectrum_rssi;
            } else if (signal_present_) {
                signal_present_ = false;
            }
        }
    } else {
        signal_detected = (rssi > rssi_threshold);
        if (signal_detected) {
            signal_present_ = true;
        } else if (signal_present_) {
            signal_present_ = false;
        }
    }

    // Pattern matching in normal scan mode: run after shape analysis passes
    PatternMatchResult normal_pattern_result = PatternMatchResult::no_match();
    if (signal_detected && config_.pattern_matching_enabled && pattern_manager_.get_pattern_count() > 0) {
        normal_pattern_result = try_match_pattern_internal(spectrum.db.data(), frequency);
    }

    if (signal_detected) {
        // Exception check: suppress drones at configured exclusion frequencies
        // Applies to both normal scanning and sweep detection paths
        {
            const FreqHz exc_radius = static_cast<FreqHz>(config_.exception_radius_mhz) * 1000000ULL;
            bool exc_match = false;
            for (uint8_t w = 0; w < 4 && !exc_match; ++w) {
                for (uint8_t i = 0; i < EXCEPTIONS_PER_WINDOW && !exc_match; ++i) {
                    const FreqHz exc = config_.sweep_exceptions[w][i];
                    if (exc == 0) continue;
                    const FreqHz lo = (exc > exc_radius) ? (exc - exc_radius) : 0;
                    const FreqHz hi = exc + exc_radius;
                    if (frequency >= lo && frequency <= hi) exc_match = true;
                }
            }
            if (exc_match) return ErrorCode::SUCCESS;
        }

        // Neighbor margin check (if enabled): center freq must dominate neighbors
        // This eliminates wideband noise false positives (WiFi, BT, microwave)
        if (config_.neighbor_margin_db > 0) {
            neighbor_margin_checker_.add(frequency, effective_rssi);
            if (!neighbor_margin_checker_.check_margin(frequency, effective_rssi, config_.neighbor_margin_db)) {
                // Current frequency not stronger than neighbors — wideband noise
                return ErrorCode::SUCCESS;
            }
        }

        // Feed RSSI detector only with above-threshold samples
        // to prevent noise from polluting trend analysis
        (void)rssi_detector_.process_rssi_sample(effective_rssi, now);

        bool should_update = true;

        if (config_.confirm_count_enabled) {
            // Confirm count: require configurable N detections on same frequency
            // before creating a drone. Prevents noise spikes from adding phantom drones.
            //
            // FIX: Added CONFIRM_TIMEOUT_MS to prevent infinite waiting on noisy frequencies.
            // On 2400 MHz with intermittent noise, pending_count_ may never reach
            // config_.confirm_count, causing scanner to wait forever.

            if (frequency != pending_frequency_) {
                // New frequency - start fresh confirmation
                pending_frequency_ = frequency;
                pending_count_ = 1;
                confirm_start_time_ = now;  // Start confirmation timer
            } else if (pending_count_ < config_.confirm_count) {
                // Still accumulating confirmations on same frequency
                pending_count_++;
            }

            // CONFIRM TIMEOUT: Force continue if taking too long to gather confirmations
            // This prevents indefinite blocking on frequencies with intermittent noise.
            if (confirm_start_time_ != 0) {
                const uint32_t confirm_elapsed = now - confirm_start_time_;
                if (confirm_elapsed >= CONFIRM_TIMEOUT_MS) {
                    // Timeout exceeded - give up on confirmation, allow frequency hop
                    // Blacklist this frequency to prevent re-triggering on intermittent noise
                    // NOTE: Must use _internal variant — DATA_MUTEX is already held by caller
                    if (config_.noise_blacklist_enabled && frequency != 0) {
                        increment_noise_count_internal(frequency);
                    }
                    pending_frequency_ = 0;
                    pending_count_ = 0;
                    confirm_start_time_ = 0;
                    // Continue to next part of logic (fall through to existing drone check)
                }
            }

            ErrorResult<size_t> existing = find_drone_by_frequency_internal(frequency);
            if (!existing.has_value() && pending_count_ < config_.confirm_count) {
                should_update = false;  // waiting for more confirmations
            }
        }

        if (should_update) {
            const ErrorCode err = update_tracked_drone_internal(
                frequency,
                effective_rssi,
                now
            );
            if (err != ErrorCode::SUCCESS) {
                return err;
            }
        }

        // Single post-update lookup (reused for pattern match + decay reset)
        const auto drone_idx = find_drone_by_frequency_internal(frequency);

        // Propagate pattern match result to tracked drone entry (normal mode)
        // NOTE: drone_type is NOT overwritten — database classification (DJI, FPV, etc.)
        // is preserved. Pattern match info is supplementary via pattern_matched_ fields.
        if (normal_pattern_result.matched && should_update && drone_idx.has_value()) {
            const SignalPattern* p = pattern_manager_.get_pattern(normal_pattern_result.pattern_index);
            if (p != nullptr && p->name[0] != '\0') {
                auto& d = tracked_drones_[drone_idx.value()];
                d.set_pattern_match(normal_pattern_result.score, p->name);
            }
        }

        // Reset decay counters for detected drone — signal confirmed present
        if (drone_idx.has_value()) {
            tracked_drones_[drone_idx.value()].reset_missed();
            tracked_drones_[drone_idx.value()].rssi_decrease_counter_ = 0;
        }

        // Update max RSSI statistic
        if (effective_rssi > statistics_.max_rssi_dbm) {
            statistics_.max_rssi_dbm = effective_rssi;
        }

        // Frequency lock state machine
        if (frequency == locked_frequency_) {
            // Same frequency as locked — accumulate persistence count
            missed_lock_count_ = 0;  // Reset miss counter on successful detection
            if (freq_lock_count_ < MAX_FREQ_LOCK) {
                freq_lock_count_++;
            }
            // Transition: LOCKING → TRACKING after sustained lock
            if (freq_lock_count_ >= MAX_FREQ_LOCK && state_ == ScannerState::LOCKING) {
                state_ = ScannerState::TRACKING;
            }
        } else {
            // Different frequency detected
            if (state_ == ScannerState::SCANNING) {
                // No active lock — jump to new frequency immediately
                locked_frequency_ = frequency;
                freq_lock_count_ = 1;
                missed_lock_count_ = 0;
                state_ = ScannerState::LOCKING;
                lock_start_time_ = chTimeNow();  // Start lock timing
                confirm_start_time_ = 0;  // Reset confirm timer (not applicable for new lock)
                // CRITICAL: Request scanner thread to hold frequency.
                // Without this, the scanner thread hops to the next DB entry
                // before the UI thread can transition state to LOCKING.
                dwell_request_.set();
            } else if (state_ == ScannerState::LOCKING || state_ == ScannerState::TRACKING) {
                // Already locked — don't jump to transient signal.
                // The current lock is more valuable than a momentary spike.
                // Continue accumulating on the locked frequency.
                // Note: The "different frequency" here is likely noise or interference
                // on the locked frequency, not a genuine signal on another frequency.
            }
            // No action needed — stay in current state
        }
    } else {
        // No signal on this frequency
        // Decay is handled by apply_rssi_decay() every CYC cycles.
        // Here we only handle lock cleanup.

        // Only break lock if we're tuned to the locked freq and it's gone
        if (locked_frequency_ != 0 && frequency == locked_frequency_) {
            // Use confirm_count × 2 as tolerance for intermittent signals (FHSS, burst, fading)
            // Default confirm_count=2 → 4 consecutive misses (~66ms) before breaking lock.
            // The ×2 factor gives a grace period for brief fades without dropping the lock.
            // Guard against confirm_count=0 (would break lock on every miss).
            const uint8_t miss_tolerance = (config_.confirm_count > 0)
                ? static_cast<uint8_t>(config_.confirm_count * 2)
                : 1;
            missed_lock_count_++;
            if (missed_lock_count_ >= miss_tolerance) {
                freq_lock_count_ = 0;
                locked_frequency_ = 0;
                missed_lock_count_ = 0;
                if (state_ == ScannerState::LOCKING || state_ == ScannerState::TRACKING) {
                    state_ = ScannerState::SCANNING;
                }
            }
        } else {
            missed_lock_count_ = 0;
        }
        // If tuned to a non-locked freq and no signal: normal scanning, ignore
    }

    return ErrorCode::SUCCESS;
}

ErrorCode DroneScanner::update_tracked_drone_internal(
    FreqHz frequency,
    RssiValue rssi,
    SystemTime timestamp
) noexcept {
    ErrorResult<size_t> index_result = find_drone_by_frequency_internal(frequency);
    
    if (index_result.has_value()) {
        // Existing drone — update and alert on threat increase
        size_t index = index_result.value();
        
        // RSSI variance rejection: noise has chaotic fluctuations
        // Real drones have stable signal (variance < 25), noise > 100
        if (config_.rssi_variance_enabled) {
            const uint32_t variance = tracked_drones_[index].calculate_rssi_variance();
            if (variance > static_cast<uint32_t>(DEFAULT_RSSI_VARIANCE_THRESHOLD)) {
                // RSSI too chaotic — likely noise, don't upgrade threat
                return ErrorCode::SUCCESS;
            }
        }

        // Update drone type from DB if it was UNKNOWN (DB may have loaded after first detection)
        if (tracked_drones_[index].drone_type == DroneType::UNKNOWN) {
            tracked_drones_[index].drone_type = determine_drone_type_internal(frequency);
        }

        ThreatLevel old_threat = tracked_drones_[index].get_threat();
        tracked_drones_[index].update_rssi(rssi, timestamp, ThreatThresholds{
            config_.threat_low_dbm, config_.threat_medium_dbm,
            config_.threat_high_dbm, config_.threat_critical_dbm});
        ThreatLevel new_threat = tracked_drones_[index].get_threat();
        
        if (new_threat > old_threat) {
            trigger_alert(new_threat);
            // Real signal confirmed — clear noise blacklist for this frequency
            reset_noise_count(frequency);
        }
    } else {
        // New drone — add and alert for its initial threat
        const size_t new_index = tracked_count_;
        ErrorCode add_result = add_tracked_drone_internal(frequency, rssi, timestamp);
        if (add_result != ErrorCode::SUCCESS) {
            return add_result;
        }
        // First detection: mark as increasing to prevent immediate decay
        // update_rssi() already sets last_rssi_ to the current RSSI
        tracked_drones_[new_index].rssi_increased_ = true;
        // Alert for newly added drone's actual threat level
        ThreatLevel new_threat = tracked_drones_[new_index].get_threat();
        if (new_threat > ThreatLevel::NONE) {
            trigger_alert(new_threat);
        }
    }

    return ErrorCode::SUCCESS;
}

ErrorResult<size_t> DroneScanner::find_drone_by_frequency_internal(
    FreqHz frequency
) const noexcept {
    for (size_t i = 0; i < tracked_count_; ++i) {
        if (tracked_drones_[i].frequency == frequency) {
            return ErrorResult<size_t>::success(i);
        }
    }
    
    return ErrorResult<size_t>::failure(ErrorCode::INVALID_PARAMETER);
}

ErrorCode DroneScanner::get_current_drone_type(char* buffer, size_t buffer_size) const noexcept {
    if (buffer == nullptr || buffer_size < 2) {
        return ErrorCode::INVALID_PARAMETER;
    }

    size_t copy_len = 0;
    for (size_t i = 0; i < 4 && i < (buffer_size - 1); ++i) {
        const char c = current_drone_type_[i];
        if (c != '\0') {
            buffer[i] = c;
            copy_len++;
        } else {
            break;
        }
    }

    buffer[copy_len] = '\0';
    return ErrorCode::SUCCESS;
}

ErrorCode DroneScanner::add_tracked_drone_internal(
    FreqHz frequency_hz,
    RssiValue rssi_dbm,
    SystemTime timestamp_ms
) noexcept {
    if (tracked_count_ >= MAX_TRACKED_DRONES) {
        return ErrorCode::BUFFER_FULL;
    }

    DroneType type = determine_drone_type_internal(frequency_hz);

    tracked_drones_[tracked_count_] = TrackedDrone(frequency_hz, type, ThreatLevel::NONE);
    tracked_drones_[tracked_count_].created_time_ = timestamp_ms;
    tracked_drones_[tracked_count_].last_increase_time_ = timestamp_ms;
    tracked_drones_[tracked_count_].update_rssi(rssi_dbm, timestamp_ms, ThreatThresholds{
        config_.threat_low_dbm, config_.threat_medium_dbm,
        config_.threat_high_dbm, config_.threat_critical_dbm});

    tracked_count_++;
    statistics_.drones_detected++;

    // NOTE: Do NOT call trigger_alert() here.
    // update_rssi() set the initial threat level inside the drone.
    // update_tracked_drone_internal() will compare and trigger the alert.

    return ErrorCode::SUCCESS;
}

DroneType DroneScanner::determine_drone_type_internal(FreqHz frequency) const noexcept {
    ErrorResult<FrequencyEntry> entry_result = database_.find_entry(frequency);
    
    if (entry_result.has_value()) {
        const FrequencyEntry& entry = entry_result.value();
        return entry.drone_type;
    }
    
    return DroneType::UNKNOWN;
}

size_t DroneScanner::get_tracked_drones(
    TrackedDrone* drones,
    size_t max_count
) const noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    
    if (drones == nullptr || max_count == 0) {
        return 0;
    }
    
    size_t copy_count = tracked_count_ < max_count ? tracked_count_ : max_count;
    
    for (size_t i = 0; i < copy_count; ++i) {
        drones[i] = tracked_drones_[i];
    }
    
    return copy_count;
}

ScannerState DroneScanner::get_state() const noexcept {
    MutexTryLock<LockOrder::DATA_MUTEX> lock(mutex_);
    if (lock.is_locked()) {
        return state_;
    }
    // Fallback: return last known state (atomic word-sized read is safe on Cortex-M4)
    return state_;
}

bool DroneScanner::is_scanning() const noexcept {
    return scanning_active_.test();
}

ScanConfig DroneScanner::get_config() const noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    return config_;
}

int32_t DroneScanner::get_threat_critical_dbm() const noexcept {
    MutexTryLock<LockOrder::DATA_MUTEX> lock(mutex_);
    if (!lock.is_locked()) {
        return RSSI_CRITICAL_THREAT_THRESHOLD_DBM;
    }
    return config_.threat_critical_dbm;
}

ErrorCode DroneScanner::set_config(const ScanConfig& config) noexcept {
    ErrorCode validate_result = validate_config_internal(config);
    if (validate_result != ErrorCode::SUCCESS) {
        return validate_result;
    }
    
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    config_ = config;
    median_filter_enabled_ = config.median_enabled;

    // Propagate configurable threat thresholds to RSSI detector
    rssi_detector_.set_threat_thresholds(ThreatThresholds{
        config.threat_low_dbm, config.threat_medium_dbm,
        config.threat_high_dbm, config.threat_critical_dbm});

    return ErrorCode::SUCCESS;
}

ErrorCode DroneScanner::validate_config_internal(const ScanConfig& config) const noexcept {
    // Validate against HARDWARE limits (not theoretical MAX_FREQUENCY_HZ)
    // HackRF One RFFC5072 mixer practical limit: 6 GHz
    if (config.start_frequency < HARDWARE_MIN_FREQ_HZ ||
        config.end_frequency > HARDWARE_MAX_FREQ_HZ ||
        config.start_frequency >= config.end_frequency) {
        return ErrorCode::INVALID_PARAMETER;
    }
    
    if (config.rssi_threshold_dbm < RSSI_MIN_DBM ||
        config.rssi_threshold_dbm > RSSI_MAX_DBM) {
        return ErrorCode::INVALID_PARAMETER;
    }
    
    // Validate scanning mode
    const uint8_t mode_value = static_cast<uint8_t>(config.mode);
    if (mode_value >= SCANNING_MODE_COUNT) {
        return ErrorCode::INVALID_PARAMETER;
    }

    // Validate sweep windows against hardware limits
    if (config.sweep_start_freq < HARDWARE_MIN_FREQ_HZ ||
        config.sweep_end_freq > HARDWARE_MAX_FREQ_HZ ||
        config.sweep_start_freq >= config.sweep_end_freq) {
        return ErrorCode::INVALID_PARAMETER;
    }
    if (config.sweep2_enabled) {
        if (config.sweep2_start_freq < HARDWARE_MIN_FREQ_HZ ||
            config.sweep2_end_freq > HARDWARE_MAX_FREQ_HZ ||
            config.sweep2_start_freq >= config.sweep2_end_freq) {
            return ErrorCode::INVALID_PARAMETER;
        }
    }
    if (config.sweep3_enabled) {
        if (config.sweep3_start_freq < HARDWARE_MIN_FREQ_HZ ||
            config.sweep3_end_freq > HARDWARE_MAX_FREQ_HZ ||
            config.sweep3_start_freq >= config.sweep3_end_freq) {
            return ErrorCode::INVALID_PARAMETER;
        }
    }
    if (config.sweep4_enabled) {
        if (config.sweep4_start_freq < HARDWARE_MIN_FREQ_HZ ||
            config.sweep4_end_freq > HARDWARE_MAX_FREQ_HZ ||
            config.sweep4_start_freq >= config.sweep4_end_freq) {
            return ErrorCode::INVALID_PARAMETER;
        }
    }

    // Validate spectrum shape filter parameters (prevent impossible detection configs)
    if (config.spectrum_min_width > config.spectrum_max_width) {
        return ErrorCode::INVALID_PARAMETER;
    }
    if (config.spectrum_min_width < 1 || config.spectrum_max_width < 2) {
        return ErrorCode::INVALID_PARAMETER;
    }
    if (config.confirm_count < CONFIRM_COUNT_MIN || config.confirm_count > CONFIRM_COUNT_MAX) {
        return ErrorCode::INVALID_PARAMETER;
    }
    if (config.cfar_ref_cells < CFAR_REF_CELLS_MIN || config.cfar_ref_cells > CFAR_REF_CELLS_MAX) {
        return ErrorCode::INVALID_PARAMETER;
    }
    if (config.cfar_guard_cells > CFAR_GUARD_CELLS_MAX) {
        return ErrorCode::INVALID_PARAMETER;
    }
    // CFAR ref cells must be >= guard cells + 2 for valid reference window
    if (config.cfar_ref_cells < config.cfar_guard_cells + 2) {
        return ErrorCode::INVALID_PARAMETER;
    }
    
    return ErrorCode::SUCCESS;
}

ScanStatistics DroneScanner::get_statistics() const noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    return statistics_;
}

void DroneScanner::reset_statistics() noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    statistics_.reset();
}

ErrorResult<FreqHz> DroneScanner::get_current_frequency() const noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    if (current_frequency_ == 0) {
        return ErrorResult<FreqHz>::failure(ErrorCode::HARDWARE_NOT_INITIALIZED);
    }
    return ErrorResult<FreqHz>::success(current_frequency_);
}

void DroneScanner::set_scan_frequency(FreqHz frequency) noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    current_frequency_ = frequency;
}

void DroneScanner::clear_lock_state() noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    freq_lock_count_ = 0;
    locked_frequency_ = 0;
    missed_lock_count_ = 0;
    // Clear lock timing timers for consistency
    lock_start_time_ = 0;
    confirm_start_time_ = 0;
    // Reset sweep frequency tracking so first sweep frame always resets median filter
    last_sweep_freq_ = 0;
    if (state_ == ScannerState::LOCKING || state_ == ScannerState::TRACKING) {
        state_ = ScannerState::SCANNING;
    }
}

size_t DroneScanner::get_tracked_count() const noexcept {
    MutexTryLock<LockOrder::DATA_MUTEX> lock(mutex_);
    if (!lock.is_locked()) {
        return tracked_count_;  // Atomic word-read fallback on Cortex-M4
    }
    return tracked_count_;
}

void DroneScanner::clear_tracked_drones() noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    tracked_count_ = 0;
}

void DroneScanner::reset_frequency() noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);

    // Get first frequency from new database
    ErrorResult<FreqHz> freq_result = database_.get_next_frequency(0);
    if (freq_result.has_value()) {
        current_frequency_ = freq_result.value();
    } else {
        current_frequency_ = MIN_FREQUENCY_HZ;
    }

    // Reset tracking state
    freq_lock_count_ = 0;
    locked_frequency_ = 0;
    missed_lock_count_ = 0;
    // Reset lock timing for consistency
    lock_start_time_ = 0;
    confirm_start_time_ = 0;
}

void DroneScanner::remove_stale_drones(SystemTime current_time) noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    remove_stale_drones_internal(current_time);
}

void DroneScanner::remove_stale_drones_internal(SystemTime current_time) noexcept {
    size_t write_index = 0;
    
    for (size_t read_index = 0; read_index < tracked_count_; ++read_index) {
        if (!tracked_drones_[read_index].is_stale(current_time, config_.stale_timeout_ms)) {
            if (write_index != read_index) {
                tracked_drones_[write_index] = tracked_drones_[read_index];
            }
            write_index++;
        }
    }
    
    tracked_count_ = write_index;
}

/**
 * @brief Sets the alert callback function
 * @param callback Function to call when alerts are triggered
 * @note The callback function MUST be thread-safe and reentrant-safe
 * @note The callback MUST NOT acquire any mutexes or perform blocking operations
 * @note The callback MUST execute quickly (preferably < 1ms) to avoid delaying scanner thread
 * @note Failure to meet timing requirements may result in missed alerts or buffer overflows
 * @warning Violating these constraints can cause system instability
 */
void DroneScanner::set_alert_callback(ThreatAlertCallback callback) noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    alert_callback_ = callback;
}

void DroneScanner::trigger_alert(ThreatLevel threat_level) noexcept {
    // Never alert for NONE level — this fires on threat decrease or initial NONE state
    if (threat_level == ThreatLevel::NONE) {
        return;
    }

    ThreatAlertCallback local_callback = alert_callback_;

    if (local_callback == nullptr) {
        return;
    }

    // Re-entrancy guard (AtomicFlag is lock-free)
    if (alert_callback_in_progress_.test_and_set()) {
        return;  // Already in progress
    }

    // Invoke callback outside any lock
    local_callback(threat_level);

    alert_callback_in_progress_.clear();
}

void DroneScanner::set_median_filter_enabled(bool enabled) noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    median_filter_enabled_ = enabled;
    config_.median_enabled = enabled;
    rssi_median_filter_.reset();
}

void DroneScanner::reset_neighbor_checker() noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    neighbor_margin_checker_.reset();
}

void DroneScanner::refresh_patterns() noexcept {
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);

    if (config_.pattern_matching_enabled) {
        pattern_matcher_.set_patterns(
            pattern_manager_.get_patterns_array(),
            pattern_manager_.get_pattern_count()
        );
    }
}

// ============================================================================
// Spectrum Shape Analysis — detect U/V peaks above flat noise floor
// ============================================================================

bool DroneScanner::analyze_spectrum_shape(const ChannelSpectrum& spectrum, int32_t& out_rssi) noexcept {
    // Step 1: Find noise floor via 25th percentile of usable bins.
    // 25th percentile (not median) is more robust when signal occupies >50% of
    // usable bins (WiFi-dense 2.4 GHz environment). Median becomes signal-biased
    // in that case, inflating the noise floor estimate and causing real drone
    // signals to fail the peak_margin check.
    uint8_t* sorted = spectrum_sort_buf_;
    size_t sort_count = 0;
    for (size_t i = FFT_EDGE_SKIP; i < FFT_BIN_COUNT - FFT_EDGE_SKIP; ++i) {
        if (i >= FFT_DC_SPIKE_START && i < FFT_DC_SPIKE_END) continue;
        sorted[sort_count++] = spectrum.db[i];
    }
    const uint8_t noise_floor = (sort_count > 0)
        ? quickselect_percentile(sorted, sort_count, 25)
        : 0;

    // Step 2: Find peak bin and peak value
    uint8_t peak_value = noise_floor;
    size_t peak_index = FFT_EDGE_SKIP;

    if (config_.cfar_mode != CFARMode::OFF) {
        const size_t cfar_peak = CFARDetector::find_peak_cfar(
            spectrum.db.data(), FFT_BIN_COUNT,
            config_.cfar_mode, config_.cfar_ref_cells, config_.cfar_guard_cells,
            config_.cfar_threshold_x10, FFT_EDGE_SKIP, FFT_EDGE_SKIP,
            config_.cfar_hybrid_alpha, config_.cfar_hybrid_beta, config_.cfar_hybrid_gamma,
            config_.os_cfar_k_percent, config_.vi_cfar_threshold_x10
        );

        if (cfar_peak >= FFT_BIN_COUNT) return false;
        peak_index = cfar_peak;
        peak_value = spectrum.db[cfar_peak];
    } else {
        for (size_t i = FFT_EDGE_SKIP; i < FFT_BIN_COUNT - FFT_EDGE_SKIP; ++i) {
            if (i >= FFT_DC_SPIKE_START && i < FFT_DC_SPIKE_END) continue;
            if (spectrum.db[i] > peak_value) {
                peak_value = spectrum.db[i];
                peak_index = i;
            }
        }
    }

    const int32_t total_gain = get_current_total_gain();
    return analyze_spectrum_shape_impl(spectrum, peak_index, peak_value, noise_floor, out_rssi, FFT_EDGE_SKIP, total_gain);
}

bool DroneScanner::analyze_spectrum_shape_impl(
    const ChannelSpectrum& spectrum,
    size_t peak_index,
    uint8_t raw_peak,
    uint8_t noise_floor,
    int32_t& out_rssi,
    size_t edge_skip,
    int32_t total_gain
) noexcept {
    return apply_shape_filters(
        spectrum.db.data(), peak_index, raw_peak, noise_floor,
        out_rssi, edge_skip, /*has_dc_gap=*/true, total_gain);
}

bool DroneScanner::analyze_spectrum_shape_lg(
    const uint8_t* lg_buffer,
    size_t peak_pixel,
    uint8_t noise_floor,
    int32_t& out_rssi,
    int32_t total_gain
) noexcept {
    static constexpr size_t LG_EDGE_SKIP_PX = 4;

    if (peak_pixel >= COMPOSITE_SIZE) return false;

    const uint8_t raw_peak = lg_buffer[peak_pixel];
    if (raw_peak <= noise_floor) return false;

    return apply_shape_filters(
        lg_buffer, peak_pixel, raw_peak, noise_floor,
        out_rssi, LG_EDGE_SKIP_PX, /*has_dc_gap=*/false, total_gain);
}

bool DroneScanner::apply_shape_filters(
    const uint8_t* data,
    size_t peak_idx,
    uint8_t raw_peak,
    uint8_t noise_floor,
    int32_t& out_rssi,
    size_t edge_skip,
    bool has_dc_gap,
    int32_t total_gain
) const noexcept {
    const uint8_t peak_margin = raw_peak - noise_floor;

    // Sensitivity-adaptive filter scaling:
    // At high sensitivity (low threshold), the RSSI gate is wide open and shape
    // filters must work harder to reject noise. We derive a sensitivity factor
    // from the RSSI threshold: 0 at default (-95 dBm), positive at high sensitivity.
    // At low sensitivity (strict threshold), the RSSI gate does most of the work
    // so shape filters stay at defaults (no loosening).
    const int32_t rssi_sens = -(config_.rssi_threshold_dbm + 95);

    // Step 3: Peak must be significantly above noise floor
    // Scale margin at high sensitivity: +1 unit per 2 sensitivity points above default.
    // At sens=75 (default): effective_margin = spectrum_margin (no change).
    // At sens=87 (rssi_sens=12): effective_margin = spectrum_margin + 6.
    const uint8_t effective_margin = (rssi_sens > 0)
        ? static_cast<uint8_t>(config_.spectrum_margin + rssi_sens / 2)
        : config_.spectrum_margin;
    if (peak_margin < effective_margin) return false;

    // Step 4: Count elevated bins around peak (signal width)
    // /3 instead of /4: for weak signals (peak_margin=20), /4 gives 5 units above
    // noise (~1 dB) where 1-bin quantization noise dominates width measurement.
    // /3 gives 7 units (~1.4 dB), providing more stable width for marginal signals.
    const uint8_t elevated_threshold = noise_floor + (peak_margin / 3);

    // Upper bound: FFT_BIN_COUNT for raw spectrum, COMPOSITE_SIZE for LG buffer
    const size_t data_size = has_dc_gap ? FFT_BIN_COUNT : COMPOSITE_SIZE;
    const size_t upper_limit = data_size - edge_skip;

    size_t left = peak_idx;
    while (left > edge_skip) {
        const size_t prev = left - 1;
        if (has_dc_gap && prev >= FFT_DC_SPIKE_START && prev < FFT_DC_SPIKE_END) {
            --left; continue;
        }
        if (data[prev] < elevated_threshold) break;
        --left;
    }

    size_t right = peak_idx;
    while (right < upper_limit - 1) {
        const size_t next = right + 1;
        if (has_dc_gap && next >= FFT_DC_SPIKE_START && next < FFT_DC_SPIKE_END) {
            ++right; continue;
        }
        if (data[next] < elevated_threshold) break;
        ++right;
    }

    const size_t signal_width = right - left + 1;

    // Step 5: Minimum width
    if (signal_width < config_.spectrum_min_width) return false;

    // Step 6: Maximum width
    if (signal_width > config_.spectrum_max_width) return false;

    // Step 7: Peak sharpness (enforce inverted-V shape)
    int32_t avg_margin = 0;
    if (config_.spectrum_peak_sharpness > 50) {
        int32_t margin_sum = 0;
        size_t count = 0;
        for (size_t i = left; i <= right; ++i) {
            if (has_dc_gap && i >= FFT_DC_SPIKE_START && i < FFT_DC_SPIKE_END) continue;
            if (data[i] > noise_floor) {
                margin_sum += (data[i] - noise_floor);
                ++count;
            }
        }
        if (count > 0) {
            avg_margin = margin_sum / static_cast<int32_t>(count);
        }

        if (avg_margin <= 0) return false;
        const int32_t sharpness = (static_cast<int32_t>(peak_margin) * 100) / avg_margin;
        if (sharpness < config_.spectrum_peak_sharpness) return false;
    }

    // Step 8: Peak ratio (tall+narrow = inverted-V)
    if (config_.spectrum_peak_ratio > 0) {
        const int32_t ratio = (static_cast<int32_t>(peak_margin) * 10) / static_cast<int32_t>(signal_width);
        if (ratio < config_.spectrum_peak_ratio) return false;
    }

    // Step 9: Valley depth (deep valleys flanking peak = V-shape)
    if (config_.spectrum_valley_depth > 0) {
        uint8_t left_valley_margin = 0;
        uint8_t right_valley_margin = 0;

        if (left > edge_skip) {
            const size_t lv = left - 1;
            const bool dc_blocked = has_dc_gap && lv >= FFT_DC_SPIKE_START && lv < FFT_DC_SPIKE_END;
            if (!dc_blocked && data[lv] > noise_floor) {
                left_valley_margin = data[lv] - noise_floor;
            }
        }
        if (right < upper_limit - 1) {
            const size_t rv = right + 1;
            const bool dc_blocked = has_dc_gap && rv >= FFT_DC_SPIKE_START && rv < FFT_DC_SPIKE_END;
            if (!dc_blocked && data[rv] > noise_floor) {
                right_valley_margin = data[rv] - noise_floor;
            }
        }

        const uint8_t max_valley = (left_valley_margin > right_valley_margin)
            ? left_valley_margin : right_valley_margin;
        if (max_valley >= config_.spectrum_valley_depth) return false;
    }

    // Step 10: Flatness (reject flat-top signals like WiFi/BT)
    // Guard: skip for weak signals where flatness measurement is unreliable.
    // At high sensitivity, lower the guard so flatness activates earlier —
    // this is critical because flatness is the primary WiFi/BT rejection filter.
    // At default sensitivity: guard = FLATNESS_MIN_PEAK_MARGIN (40, ~8 dB).
    // At sens=87 (rssi_sens=12): guard = max(15, 40-12) = 28 (~5.6 dB).
    // At sens=95 (rssi_sens=20): guard = max(15, 40-20) = 20 (~4 dB).
    const uint8_t effective_flatness_min = (rssi_sens > 0)
        ? static_cast<uint8_t>((FLATNESS_MIN_PEAK_MARGIN > static_cast<uint8_t>(rssi_sens))
            ? (FLATNESS_MIN_PEAK_MARGIN - rssi_sens)
            : 15)
        : FLATNESS_MIN_PEAK_MARGIN;
    if (config_.spectrum_flatness > 0 && peak_margin >= effective_flatness_min) {
        const uint8_t high_power_threshold = raw_peak * 9 / 10;
        size_t high_power_count = 0;

        for (size_t i = peak_idx; i > left && i > edge_skip; --i) {
            if (has_dc_gap && i >= FFT_DC_SPIKE_START && i < FFT_DC_SPIKE_END) continue;
            if (data[i] >= high_power_threshold) ++high_power_count;
            else break;
        }
        for (size_t i = peak_idx + 1; i <= right && i < upper_limit; ++i) {
            if (has_dc_gap && i >= FFT_DC_SPIKE_START && i < FFT_DC_SPIKE_END) continue;
            if (data[i] >= high_power_threshold) ++high_power_count;
            else break;
        }

        // Denominator: signal width excluding DC spike bins (if present)
        size_t effective_width = right - left + 1;
        if (has_dc_gap && left < FFT_DC_SPIKE_END && right >= FFT_DC_SPIKE_START) {
            const size_t dc_start = (left > FFT_DC_SPIKE_START) ? left : FFT_DC_SPIKE_START;
            const size_t dc_end = (right < FFT_DC_SPIKE_END) ? right : (FFT_DC_SPIKE_END - 1);
            effective_width -= (dc_end - dc_start + 1);
        }
        if (effective_width > 0) {
            const uint8_t flatness_pct = static_cast<uint8_t>((high_power_count * 100) / effective_width);
            if (flatness_pct > config_.spectrum_flatness) return false;
        }
    }

    // Step 11: Symmetry (V-shape must have similar left/right width)
    if (config_.spectrum_symmetry > 0 && signal_width > 1) {
        const size_t left_width = peak_idx - left;
        const size_t right_width = right - peak_idx;
        const size_t max_side = (left_width > right_width) ? left_width : right_width;
        const size_t min_side = (left_width < right_width) ? left_width : right_width;
        if (max_side > 0) {
            const uint8_t sym_pct = static_cast<uint8_t>((min_side * 100) / max_side);
            if (sym_pct < config_.spectrum_symmetry) return false;
        }
    }

    out_rssi = spectrum_value_to_dbm(raw_peak, total_gain);
    return true;
}

PatternMatchResult DroneScanner::try_match_pattern_internal(
    const uint8_t* spectrum,
    FreqHz current_freq
) noexcept {
    if (!config_.pattern_matching_enabled) {
        return PatternMatchResult::no_match();
    }

    if (pattern_manager_.get_pattern_count() == 0) {
        return PatternMatchResult::no_match();
    }

    return pattern_matcher_.match(spectrum, current_freq);
}

// ============================================================================
// apply_sweep_tracking — range check, exception filter, Mahalanobis gate,
//                        drone tracking, and pattern match assignment
// ============================================================================

void DroneScanner::apply_sweep_tracking(
    FreqHz peak_freq,
    int32_t peak_rssi,
    FreqHz center_freq,
    FreqHz f_min,
    FreqHz f_max,
    size_t highlight_bin,
    int8_t pattern_index,
    uint16_t pattern_correlation,
    bool pattern_matched
) noexcept {
    const int32_t cfg_rssi_thresh = config_.rssi_threshold_dbm;
    if (peak_rssi <= cfg_rssi_thresh) return;

    // Range check — prevent false positives outside sweep boundaries
    const FreqHz range_min = (f_min != 0) ? f_min : config_.sweep_start_freq;
    const FreqHz range_max = (f_max != 0) ? f_max : config_.sweep_end_freq;
    if (range_min != 0 && range_max != 0) {
        if (peak_freq < range_min || peak_freq > range_max) return;
    }

    // Exception frequency filter
    const FreqHz exc_radius = static_cast<FreqHz>(config_.exception_radius_mhz) * 1000000ULL;
    bool is_exception = false;
    for (uint8_t w = 0; w < 4 && !is_exception; ++w) {
        for (uint8_t i = 0; i < EXCEPTIONS_PER_WINDOW && !is_exception; ++i) {
            const FreqHz exc = config_.sweep_exceptions[w][i];
            if (exc == 0) continue;
            const FreqHz lo = (exc > exc_radius) ? (exc - exc_radius) : 0;
            const FreqHz hi = exc + exc_radius;
            if (peak_freq >= lo && peak_freq <= hi) is_exception = true;
        }
    }
    if (is_exception) return;

    // Mahalanobis gate
    bool mahalanobis_rejected = false;
    bool drone_created_here = false;
    if (config_.mahalanobis_enabled) {
        size_t drone_idx = 0;
        bool drone_found = false;
        for (uint8_t i = 0; i < tracked_count_; ++i) {
            if (tracked_drones_[i].frequency == peak_freq) {
                drone_idx = i;
                drone_found = true;
                break;
            }
        }

        if (drone_found) {
            MahalanobisStatistics& stats = tracked_drones_[drone_idx].get_mahalanobis_stats();
            if (!mahalanobis_detector_.validate(
                peak_rssi, center_freq, stats, config_.mahalanobis_threshold_x10
            )) {
                mahalanobis_rejected = true;
            } else {
                mahalanobis_detector_.update_statistics(stats, peak_rssi, center_freq, peak_freq);
            }
        } else if (tracked_count_ < MAX_TRACKED_DRONES) {
            const size_t new_idx = tracked_count_;
            tracked_drones_[new_idx] = TrackedDrone(peak_freq, DroneType::UNKNOWN, ThreatLevel::NONE);
            auto& drone = tracked_drones_[new_idx];
            drone.created_time_ = chTimeNow();
            drone.last_increase_time_ = chTimeNow();
            drone.mark_seen(chTimeNow());
            drone.update_rssi(peak_rssi, chTimeNow(), ThreatThresholds{
                config_.threat_low_dbm, config_.threat_medium_dbm,
                config_.threat_high_dbm, config_.threat_critical_dbm});
            drone.get_mahalanobis_stats().last_tuned_frequency = peak_freq;
            tracked_count_++;
            statistics_.drones_detected++;
            if (drone.threat_level > ThreatLevel::NONE) {
                trigger_alert(drone.threat_level);
            }
            drone_created_here = true;
        }
    }

    // Skip update_tracked_drone_internal when drone was just created above
    // to avoid double update_rssi() + double trigger_alert()
    if (!mahalanobis_rejected && !drone_created_here) {
        (void)update_tracked_drone_internal(peak_freq, peak_rssi, chTimeNow());

        // Single post-update lookup (reused for pattern match + mark_seen)
        const auto drone_idx = find_drone_by_frequency_internal(peak_freq);

        if (pattern_matched && drone_idx.has_value()) {
            const SignalPattern* pattern = pattern_manager_.get_pattern(pattern_index);
            if (pattern != nullptr && pattern->name[0] != '\0') {
                matched_pattern_index_ = pattern_index;
                matched_pattern_bin_ = highlight_bin;
                auto& drone = tracked_drones_[drone_idx.value()];
                drone.set_pattern_match(pattern_correlation, pattern->name);
            }
        }

        if (drone_idx.has_value()) {
            tracked_drones_[drone_idx.value()].mark_seen(chTimeNow());
        }
    }
}

void DroneScanner::process_spectrum_sweep(
    const ChannelSpectrum& spectrum,
    const uint8_t* lg_buffer,
    FreqHz center_freq,
    FreqHz f_min,
    FreqHz f_max
) noexcept {
    current_frequency_ = center_freq;

    clear_matched_pattern();

    // Per-frequency median filter reset: only reset when the tuned frequency changes.
    // In sweep mode, each FFT frame is a different frequency, so resetting every frame
    // makes the filter useless (never reaches warm state of 7 samples). By tracking
    // the last frequency, the filter accumulates across sweep CYCLES for the same freq,
    // providing meaningful RSSI smoothing after ~7 passes (~11 seconds at 1.6s/pass).
    if (center_freq != last_sweep_freq_) {
        rssi_median_filter_.reset();
        last_sweep_freq_ = center_freq;
    }

    // Hysteresis reset: only when frequency changes (matches normal mode pattern at :730)
    // Preserves signal_present_ state when revisiting the same frequency in sweep cycles
    if (center_freq != last_hysteresis_freq_) {
        signal_present_ = false;
        last_hysteresis_freq_ = center_freq;
    }

    const uint8_t cfg_margin = config_.spectrum_margin;

    // Step 1: CFAR or fixed-threshold peak detection on RAW FFT bins.
    // CFAR must run on raw bins to see the true noise structure.
    size_t peak_index = FFT_EDGE_SKIP_NARROW;
    uint8_t raw_peak = 0;
    uint8_t noise_floor = 0;

    if (config_.cfar_mode != CFARMode::OFF) {
        const size_t cfar_peak = CFARDetector::find_peak_cfar(
            spectrum.db.data(),
            FFT_BIN_COUNT,
            config_.cfar_mode,
            config_.cfar_ref_cells,
            config_.cfar_guard_cells,
            config_.cfar_threshold_x10,
            FFT_EDGE_SKIP_NARROW,
            FFT_EDGE_SKIP_NARROW,
            config_.cfar_hybrid_alpha,
            config_.cfar_hybrid_beta,
            config_.cfar_hybrid_gamma,
            config_.os_cfar_k_percent,
            config_.vi_cfar_threshold_x10
        );

        if (cfar_peak >= FFT_BIN_COUNT) return;
        peak_index = cfar_peak;
        raw_peak = spectrum.db[cfar_peak];

        // Noise floor from raw bins (CFAR path)
        uint8_t* usable = sweep_usable_buf_;
        size_t idx = 0;
        for (size_t i = FFT_EDGE_SKIP_NARROW; i < FFT_DC_SPIKE_START; ++i) {
            usable[idx++] = spectrum.db[i];
        }
        for (size_t i = FFT_DC_SPIKE_END; i < (FFT_BIN_COUNT - FFT_EDGE_SKIP_NARROW); ++i) {
            usable[idx++] = spectrum.db[i];
        }
        if (idx > 0) {
            noise_floor = quickselect_percentile(usable, idx, 25);
        }
    } else {
        uint8_t* usable = sweep_usable_buf_;
        size_t idx = 0;
        raw_peak = 0;
        peak_index = FFT_EDGE_SKIP_NARROW;

        for (size_t i = FFT_EDGE_SKIP_NARROW; i < FFT_DC_SPIKE_START; ++i) {
            usable[idx++] = spectrum.db[i];
            if (spectrum.db[i] > raw_peak) { raw_peak = spectrum.db[i]; peak_index = i; }
        }
        for (size_t i = FFT_DC_SPIKE_END; i < (FFT_BIN_COUNT - FFT_EDGE_SKIP_NARROW); ++i) {
            usable[idx++] = spectrum.db[i];
            if (spectrum.db[i] > raw_peak) { raw_peak = spectrum.db[i]; peak_index = i; }
        }

        if (idx == 0) return;
        noise_floor = quickselect_percentile(usable, idx, 25);

        const uint8_t peak_margin_fixed = raw_peak - noise_floor;
        if (peak_margin_fixed < cfg_margin) return;
    }

    // Convert FFT bin peak to LG pixel space. If the peak falls on the DC
    // spike (bins 120-135), fft_bin_to_lg_pixel() returns COMPOSITE_SIZE
    // and we reject — this is correct because the DC spike carries no
    // real signal energy.
    const size_t peak_pixel = fft_bin_to_lg_pixel(peak_index);
    if (peak_pixel >= COMPOSITE_SIZE) return;

    // Pattern matching on raw FFT (consistent normalization with saved patterns).
    // NOTE: Patterns are saved via normalize() which operates on raw 256-bin FFT
    // and skips DC spike bins within each 16-bin chunk. Using match_from_lg()
    // here would produce a different 16-bin waveform (different usable range,
    // no DC skip) causing false mismatches against saved patterns.
    size_t highlight_bin = 0;
    bool early_pattern_matched = false;
    int8_t early_pattern_index = -1;
    uint16_t early_pattern_correlation = 0;

    const PatternMatchResult early_result = try_match_pattern_internal(spectrum.db.data(), center_freq);
    if (early_result.matched) {
        early_pattern_matched = true;
        early_pattern_index = static_cast<int8_t>(early_result.pattern_index);
        highlight_bin = peak_index;
        early_pattern_correlation = early_result.score;
    }

    // Shape analysis on LG-reordered buffer (continuous, no DC gap).
    // The 25th percentile noise floor from raw bins is valid here because
    // the LG reordering does not change power values — only their order.
    const int32_t total_gain = get_current_total_gain();

    int32_t shape_rssi = RSSI_MIN_DBM;
    if (!analyze_spectrum_shape_lg(lg_buffer, peak_pixel, noise_floor, shape_rssi, total_gain)) {
        return;
    }

    // Median filter
    int32_t peak_rssi = shape_rssi;
    rssi_median_filter_.add(peak_rssi);
    if (median_filter_enabled_ && rssi_median_filter_.is_warm()) {
        peak_rssi = rssi_median_filter_.get_median();
    }

    apply_sweep_tracking(
        fft_bin_to_freq(center_freq, peak_index),
        peak_rssi, center_freq, f_min, f_max,
        highlight_bin, early_pattern_index,
        early_pattern_correlation, early_pattern_matched
    );
}

} // namespace drone_analyzer
