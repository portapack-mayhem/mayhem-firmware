#include <cstdint>

#include "ch.h"

#include "scanner.hpp"
#include "receiver_model.hpp"
#include "portapack_persistent_memory.hpp"
#include "mahalanobis_gate.hpp"

namespace drone_analyzer {

/**
 * @brief RSSI extraction — scan usable bins with edge skip (skip DC spike + filter rolloff)
 * @param spectrum Channel spectrum data (256 bins, 0-255 each)
 * @return RSSI in dBm (peak from bins FFT_EDGE_SKIP to DC_SPIKE_START and DC_SPIKE_END to end)
 * @note EDGE_SKIP avoids filter rolloff artifacts at edge bins.
 * @note The HackRF baseband produces spectrum.db[i] = clamp(dBV*5 + 255, 0, 255).
 *       Center bins 120-135 contain the DC spike from FFT zero-frequency component.
 */
static int32_t extract_rssi(const ChannelSpectrum& spectrum) noexcept {
    uint8_t peak = 0;

    // Lower sideband: bins EDGE_SKIP to DC_SPIKE_START
    for (size_t i = FFT_EDGE_SKIP; i < FFT_DC_SPIKE_START; ++i) {
        if (spectrum.db[i] > peak) peak = spectrum.db[i];
    }
    // Upper sideband: bins DC_SPIKE_END to (BIN_COUNT - EDGE_SKIP)
    for (size_t i = FFT_DC_SPIKE_END; i < (FFT_BIN_COUNT - FFT_EDGE_SKIP); ++i) {
        if (spectrum.db[i] > peak) peak = spectrum.db[i];
    }

    // Clamp to valid dBm range
    int32_t rssi = static_cast<int32_t>(peak) - FFT_DBM_OFFSET;
    if (rssi < RSSI_MIN_DBM) rssi = RSSI_MIN_DBM;
    if (rssi > RSSI_MAX_DBM) rssi = RSSI_MAX_DBM;
    return rssi;
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
    , sweep_step_freq(20000000)
    , dwell_enabled(true)           // Stay on freq when signal detected
    , confirm_count_enabled(true)   // Require confirmations to reduce noise
    , noise_blacklist_enabled(true) // Skip persistent noise frequencies
    , spectrum_detection_enabled(true) // Use spectrum shape analysis
    , median_enabled(true)        // Median filter for RSSI spike rejection (ON by default)
    , spectrum_margin(DEFAULT_SPECTRUM_MARGIN)
    , spectrum_min_width(DEFAULT_SPECTRUM_MIN_WIDTH)
    , spectrum_max_width(DEFAULT_SPECTRUM_MAX_WIDTH)
    , spectrum_peak_sharpness(DEFAULT_SPECTRUM_PEAK_SHARPNESS)
    , spectrum_peak_ratio(DEFAULT_SPECTRUM_PEAK_RATIO)
    , spectrum_valley_depth(DEFAULT_SPECTRUM_VALLEY_DEPTH)
    , spectrum_flatness(DEFAULT_SPECTRUM_FLATNESS)
    , spectrum_symmetry(DEFAULT_SPECTRUM_SYMMETRY)
    , neighbor_margin_db(DEFAULT_NEIGHBOR_MARGIN_DB)
    , rssi_variance_enabled(false)  // Off by default (experimental)
    , confirm_count(DEFAULT_CONFIRM_COUNT)
{
    // sweep2/3/4 fields use in-class defaults: disabled
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
    , sweep_step_freq(20000000)
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
    , spectrum_symmetry(DEFAULT_SPECTRUM_SYMMETRY) {
    // sweep2/3/4 fields use in-class defaults: disabled
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
    , current_drone_type_{'\0', '\0', '\0', '\0'}
    , drone_type_valid_{false}
    , statistics_()
    , tracked_drones_()
    , tracked_count_{0}
    , current_frequency_{0}
    , last_scan_time_{0}
    , scanning_active_()
    , alert_callback_(nullptr)
    , mutex_()
    , state_transition_allowed_()
    , force_resume_flag_()
    , dwell_request_()
    , spectrum_sort_buf_{}
    , sweep_usable_buf_{}
    , alert_callback_in_progress_()
    , rssi_detector_()
    , histogram_processor_()
    , rssi_median_filter_()
    , neighbor_margin_checker_()
    , mahalanobis_detector_() {

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
    // Find existing entry or first empty slot
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
    static constexpr uint8_t NOISE_BLACKLIST_THRESHOLD = 3;
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
        }
        dwell_cycles_ = 0;
    }

    // Check dwell request from UI thread (signal detected, hold frequency)
    if (dwell_request_.test_and_set()) {
        dwell_request_.clear();
        // Only reset dwell_cycles_ if we're tuned to the locked frequency
        // If current frequency differs, scanner will hop to next frequency
        // and the dwell will be handled naturally on the locked frequency
        if (current_frequency_ == locked_frequency_) {
            dwell_cycles_ = 1;  // Start at 1 so should_dwell triggers immediately
        }
    }

    // Dwell: if UI requested hold or state is LOCKING/TRACKING, skip frequency hop
    const bool should_dwell = config_.dwell_enabled &&
        (dwell_cycles_ > 0 || state_ == ScannerState::LOCKING || state_ == ScannerState::TRACKING);

    if (should_dwell) {
        dwell_cycles_++;

        // Max dwell: 200ms total (4 cycles × 50ms).
        // Enough time for 2 confirmations at 60fps (~33ms each).
        static constexpr uint8_t MAX_DWELL_CYCLES = 4;
        const uint8_t max_dwell = config_.confirm_count_enabled
            ? MAX_DWELL_CYCLES : (MAX_DWELL_CYCLES / 2);

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
            for (size_t skip = 0; skip < MAX_NOISE_ENTRIES && is_blacklisted(current_frequency_); ++skip) {
                freq_result = database_.get_next_frequency(current_frequency_);
                if (freq_result.has_value()) {
                    current_frequency_ = freq_result.value();
                } else {
                    break;
                }
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

ErrorCode DroneScanner::process_spectrum_message(const ChannelSpectrum& spectrum) noexcept {
    return process_spectrum_message(spectrum, current_frequency_);
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

    int32_t effective_rssi = rssi;
    bool signal_detected = false;

    if (config_.spectrum_detection_enabled) {
        // Spectrum-only: shape analysis gates detection
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
            if (frequency != pending_frequency_) {
                pending_frequency_ = frequency;
                pending_count_ = 1;
            } else if (pending_count_ < config_.confirm_count) {
                pending_count_++;
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

        // Reset decay counters for detected drone — signal confirmed present
        ErrorResult<size_t> idx = find_drone_by_frequency_internal(frequency);
        if (idx.has_value()) {
            tracked_drones_[idx.value()].reset_missed();
            tracked_drones_[idx.value()].rssi_decrease_counter_ = 0;
        }

        // Update max RSSI statistic
        if (effective_rssi > static_cast<int32_t>(statistics_.max_rssi_dbm)) {
            statistics_.max_rssi_dbm = static_cast<uint32_t>(effective_rssi);
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
            // Only jump lock if currently in SCANNING (no active lock)
            // If already LOCKING/TRACKING, the current lock is more valuable
            // than a transient signal on another frequency
            if (state_ == ScannerState::SCANNING) {
                locked_frequency_ = frequency;
                freq_lock_count_ = 1;
                missed_lock_count_ = 0;
                state_ = ScannerState::LOCKING;
                // CRITICAL: Request scanner thread to hold frequency.
                // Without this, the scanner thread hops to the next DB entry
                // before the UI thread can transition state to LOCKING.
                dwell_request_.set();
            }
            // If LOCKING/TRACKING and different freq detected:
            // Don't jump. Continue accumulating on locked freq.
        }
    } else {
        // No signal on this frequency
        // Decay is handled by apply_rssi_decay() every CYC cycles.
        // Here we only handle lock cleanup.

        // Only break lock if we're tuned to the locked freq and it's gone
        if (locked_frequency_ != 0 && frequency == locked_frequency_) {
            // Use confirm_count as tolerance for intermittent signals (FHSS, burst, fading)
            // Default confirm_count=2 means 2 consecutive misses break the lock
            // Guard against confirm_count=0 (would break lock on every miss)
            const uint8_t miss_tolerance = (config_.confirm_count > 0) ? config_.confirm_count : 1;
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

        // ====================================================================
        // Mahalanobis gate validation
        // ====================================================================
        if (config_.mahalanobis_enabled) {
            MahalanobisStatistics& stats = tracked_drones_[index].get_mahalanobis_stats();

            if (!mahalanobis_detector_.validate(
                rssi,
                frequency,
                stats,
                config_.mahalanobis_threshold_x10
            )) {
                increment_noise_count(frequency);
                return ErrorCode::SUCCESS;
            }

            mahalanobis_detector_.update_statistics(
                stats,
                rssi,
                frequency,
                frequency
            );
        }

        // Update drone type from DB if it was UNKNOWN (DB may have loaded after first detection)
        if (tracked_drones_[index].drone_type == DroneType::UNKNOWN) {
            tracked_drones_[index].drone_type = determine_drone_type_internal(frequency);
        }

        ThreatLevel old_threat = tracked_drones_[index].get_threat();
        tracked_drones_[index].update_rssi(rssi, timestamp);
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
    tracked_drones_[tracked_count_].update_rssi(rssi_dbm, timestamp_ms);

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

ErrorCode DroneScanner::set_config(const ScanConfig& config) noexcept {
    ErrorCode validate_result = validate_config_internal(config);
    if (validate_result != ErrorCode::SUCCESS) {
        return validate_result;
    }
    
    MutexLock<LockOrder::DATA_MUTEX> lock(mutex_);
    config_ = config;
    median_filter_enabled_ = config.median_enabled;

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

// ============================================================================
// process_spectrum_sweep — moved from header to reduce code bloat
// ============================================================================

// ============================================================================
// Spectrum Shape Analysis — detect U/V peaks above flat noise floor
// ============================================================================

bool DroneScanner::analyze_spectrum_shape(const ChannelSpectrum& spectrum, int32_t& out_rssi) noexcept {
    // CFAR detection: if enabled, use adaptive threshold instead of fixed margin
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
        
        if (cfar_peak >= FFT_BIN_COUNT) {
            return false;  // No signal detected by CFAR
        }
        
        // CFAR detected a signal — compute RSSI from peak bin
        out_rssi = static_cast<int32_t>(spectrum.db[cfar_peak]) - FFT_DBM_OFFSET;
        if (out_rssi > RSSI_MAX_DBM) out_rssi = RSSI_MAX_DBM;
        if (out_rssi < RSSI_MIN_DBM) out_rssi = RSSI_MIN_DBM;
        
        // Still apply shape filters (width, sharpness, etc.) for drone discrimination
        // Fall through to shape analysis below using CFAR peak as the detected peak
    }

    // Step 1: Find noise floor = median of usable bins
    // Skip edges AND DC spike (same bins extract_rssi skips)
    // Uses class member buffer (mutable) — no static in method
    uint8_t* sorted = spectrum_sort_buf_;
    size_t sort_count = 0;
    for (size_t i = FFT_EDGE_SKIP_NARROW; i < FFT_BIN_COUNT - FFT_EDGE_SKIP_NARROW; ++i) {
        if (i >= FFT_DC_SPIKE_START && i < FFT_DC_SPIKE_END) continue;
        sorted[sort_count++] = spectrum.db[i];
    }
    // Insertion sort
    for (size_t i = 1; i < sort_count; ++i) {
        const uint8_t key = sorted[i];
        size_t j = i;
        while (j > 0 && sorted[j - 1] > key) {
            sorted[j] = sorted[j - 1];
            --j;
        }
        sorted[j] = key;
    }
    const uint8_t noise_floor = sorted[sort_count / 2];

    // Step 2: Find peak bin and peak value (skip edges AND DC spike)
    uint8_t peak_value = noise_floor;
    size_t peak_index = FFT_EDGE_SKIP_NARROW;
    for (size_t i = FFT_EDGE_SKIP_NARROW; i < FFT_BIN_COUNT - FFT_EDGE_SKIP_NARROW; ++i) {
        if (i >= FFT_DC_SPIKE_START && i < FFT_DC_SPIKE_END) continue;
        if (spectrum.db[i] > peak_value) {
            peak_value = spectrum.db[i];
            peak_index = i;
        }
    }

    // Step 3: Peak must be significantly above noise floor (configurable)
    // If CFAR is enabled, skip this check (CFAR already validated the peak)
    const uint8_t peak_margin = peak_value - noise_floor;
    if (config_.cfar_mode == CFARMode::OFF) {
        const uint8_t min_margin = config_.spectrum_margin;
        if (peak_margin < min_margin) {
            return false;
        }
    }

    // Step 4: Count elevated bins around peak (signal width)
    const uint8_t elevated_threshold = noise_floor + (peak_margin / 4);

    size_t left = peak_index;
    while (left > FFT_EDGE_SKIP_NARROW) {
        size_t prev = left - 1;
        if (prev >= FFT_DC_SPIKE_START && prev < FFT_DC_SPIKE_END) { --left; continue; }
        if (spectrum.db[prev] < elevated_threshold) break;
        --left;
    }
    size_t right = peak_index;
    while (right < FFT_BIN_COUNT - FFT_EDGE_SKIP_NARROW - 1) {
        size_t next = right + 1;
        if (next >= FFT_DC_SPIKE_START && next < FFT_DC_SPIKE_END) { ++right; continue; }
        if (spectrum.db[next] < elevated_threshold) break;
        ++right;
    }

    const size_t signal_width = right - left + 1;

    // Step 5: Signal width must meet minimum (configurable)
    const size_t min_width = config_.spectrum_min_width;
    if (signal_width < min_width) {
        return false;
    }

    // Step 6: Signal width must not exceed maximum (reject flat U/I shapes)
    const size_t max_width = config_.spectrum_max_width;
    if (signal_width > max_width) {
        return false;
    }

    // Step 7+10: Compute average margin ONCE for both sharpness and flatness checks
    // sharpness = (peak_margin * 100) / avg_margin
    // flatness  = (peak_margin * 100) / avg_margin
    // V-shape (drone video link): sharpness >> 100, flatness >> 200
    // U/I shape (flat noise):     sharpness ~ 100, flatness ~ 100-120
    int32_t avg_margin = 0;
    if (config_.spectrum_peak_sharpness > 50 || config_.spectrum_flatness > 50) {
        int32_t margin_sum = 0;
        size_t bin_count = 0;
        for (size_t i = left; i <= right; ++i) {
            if (i >= FFT_DC_SPIKE_START && i < FFT_DC_SPIKE_END) continue;
            if (spectrum.db[i] > noise_floor) {
                margin_sum += (spectrum.db[i] - noise_floor);
                ++bin_count;
            }
        }
        if (bin_count > 0) {
            avg_margin = margin_sum / static_cast<int32_t>(bin_count);
        }
    }

    // Step 7: Peak sharpness check — enforce inverted-V shape
    if (config_.spectrum_peak_sharpness > 50 && avg_margin > 0) {
        const int32_t sharpness = (static_cast<int32_t>(peak_margin) * 100) / avg_margin;
        if (sharpness < config_.spectrum_peak_sharpness) {
            return false;  // Flat-topped signal, not V-shaped
        }
    }

    // Step 8: Peak ratio check — tall + narrow = inverted-V (drone video link)
    // ratio = (peak_margin * 10) / signal_width
    // Inverted-V: ratio > 50 (tall, narrow peak)
    // Flat U/I:   ratio < 20 (wide, short signal)
    // Needle:     ratio > 100 (very tall, very narrow)
    if (config_.spectrum_peak_ratio > 0) {
        const int32_t ratio = (static_cast<int32_t>(peak_margin) * 10) / static_cast<int32_t>(signal_width);
        if (ratio < config_.spectrum_peak_ratio) {
            return false;  // Signal too flat relative to width
        }
    }

    // Step 9: Valley depth check — deep valleys flanking peak = V-shape
    // Measures margin of bins immediately adjacent to the signal
    // Inverted-V: deep valleys (flanking bins have margin < 5)
    // Flat U/I:   shallow valleys (flanking bins still elevated)
    if (config_.spectrum_valley_depth > 0) {
        uint8_t left_valley_margin = 0;
        uint8_t right_valley_margin = 0;

        // Check bin immediately to the left of signal
        if (left > FFT_EDGE_SKIP_NARROW) {
            size_t lv = left - 1;
            if (!(lv >= FFT_DC_SPIKE_START && lv < FFT_DC_SPIKE_END)) {
                if (spectrum.db[lv] > noise_floor) {
                    left_valley_margin = spectrum.db[lv] - noise_floor;
                }
            }
        }

        // Check bin immediately to the right of signal
        if (right < FFT_BIN_COUNT - FFT_EDGE_SKIP_NARROW - 1) {
            size_t rv = right + 1;
            if (!(rv >= FFT_DC_SPIKE_START && rv < FFT_DC_SPIKE_END)) {
                if (spectrum.db[rv] > noise_floor) {
                    right_valley_margin = spectrum.db[rv] - noise_floor;
                }
            }
        }

        // Both flanking bins must have margin < valley_depth (deep valleys)
        const uint8_t max_valley = (left_valley_margin > right_valley_margin) ? left_valley_margin : right_valley_margin;
        if (max_valley >= config_.spectrum_valley_depth) {
            return false;  // Valleys too shallow — flat U/I shape
        }
    }

    // Step 10: Flatness check — peak must dominate average margin (reuses avg_margin from step 7)
    // flatness = (peak_margin * 100) / avg_margin
    // WiFi/BT flat-top: flatness ~ 100-120 (peak barely above average elevation)
    // Drone V-shape: flatness >> 200 (peak towers above flanking bins)
    // Plateau: flatness ~ 100 (all bins at same level)
    if (config_.spectrum_flatness > 50 && avg_margin > 0) {
        const int32_t flatness = (static_cast<int32_t>(peak_margin) * 100) / avg_margin;
        if (flatness < config_.spectrum_flatness) {
            return false;  // Flat-top signal — peak does not dominate (WiFi, FM, BT)
        }
    }

    // Step 11: Symmetry check — V-shape signal must have similar left/right width
    // symmetry = min(left_w, right_w) * 100 / max(left_w, right_w)
    // Drone video V-shape: symmetry > 50% (both sides of peak are similar)
    // Noise burst: symmetry < 25% (one side dominates)
    // Disabled when signal_width <= 1 (single bin, no sides to compare)
    if (config_.spectrum_symmetry > 0 && signal_width > 1) {
        const size_t left_width = peak_index - left;
        const size_t right_width = right - peak_index;
        const size_t max_side = (left_width > right_width) ? left_width : right_width;
        const size_t min_side = (left_width < right_width) ? left_width : right_width;
        if (max_side > 0) {
            const uint8_t sym_pct = static_cast<uint8_t>((min_side * 100) / max_side);
            if (sym_pct < config_.spectrum_symmetry) {
                return false;  // Asymmetric signal — likely noise burst
            }
        }
    }

    // RSSI from actual peak bin value (same conversion as extract_rssi)
    out_rssi = static_cast<int32_t>(peak_value) - FFT_DBM_OFFSET;
    if (out_rssi > RSSI_MAX_DBM) out_rssi = RSSI_MAX_DBM;
    if (out_rssi < RSSI_MIN_DBM) out_rssi = RSSI_MIN_DBM;

    return true;
}

// ============================================================================
// process_spectrum_sweep — moved from header to reduce code bloat
// ============================================================================

void DroneScanner::process_spectrum_sweep(const ChannelSpectrum& spectrum, FreqHz center_freq, FreqHz f_min, FreqHz f_max) noexcept {
    current_frequency_ = center_freq;

    // Cache config values locally (avoid repeated member access in hot loop)
    const uint8_t cfg_margin = config_.spectrum_margin;
    const uint8_t cfg_min_width = config_.spectrum_min_width;
    const uint8_t cfg_max_width = config_.spectrum_max_width;
    const uint8_t cfg_sharpness = config_.spectrum_peak_sharpness;
    const uint8_t cfg_ratio = config_.spectrum_peak_ratio;
    const uint8_t cfg_valley = config_.spectrum_valley_depth;
    const uint8_t cfg_flatness = config_.spectrum_flatness;
    const uint8_t cfg_symmetry = config_.spectrum_symmetry;
    const int32_t cfg_rssi_thresh = config_.rssi_threshold_dbm;

    // CFAR detection: if enabled, use adaptive threshold instead of fixed margin
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
        
        if (cfar_peak >= FFT_BIN_COUNT) return;  // No signal detected by CFAR
        
        peak_index = cfar_peak;
        raw_peak = spectrum.db[cfar_peak];
        
        // Compute noise floor for shape analysis (still needed for width/sharpness checks)
        uint8_t* usable = sweep_usable_buf_;
        size_t idx = 0;
        for (size_t i = FFT_EDGE_SKIP_NARROW; i < FFT_DC_SPIKE_START; ++i) {
            usable[idx++] = spectrum.db[i];
        }
        for (size_t i = FFT_DC_SPIKE_END; i < (FFT_BIN_COUNT - FFT_EDGE_SKIP_NARROW); ++i) {
            usable[idx++] = spectrum.db[i];
        }
        if (idx > 0) {
            const size_t k = idx / 2;
            uint8_t qs_left = 0;
            uint8_t qs_right = static_cast<uint8_t>(idx) - 1;
            while (qs_left < qs_right) {
                const uint8_t pivot_idx = qs_left + (qs_right - qs_left) / 2;
                uint8_t pivot = usable[pivot_idx];
                usable[pivot_idx] = usable[qs_right];
                usable[qs_right] = pivot;
                uint8_t store = qs_left;
                for (uint8_t i = qs_left; i < qs_right; ++i) {
                    if (usable[i] < pivot) {
                        uint8_t t = usable[store];
                        usable[store] = usable[i];
                        usable[i] = t;
                        store++;
                    }
                }
                {
                    uint8_t t = usable[store];
                    usable[store] = usable[qs_right];
                    usable[qs_right] = t;
                }
                if (store == k) break;
                if (store < k) qs_left = store + 1;
                else qs_right = store - 1;
            }
            noise_floor = usable[k];
        }
    } else {
        // Original fixed-threshold detection
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

        // Guard: no usable bins (all in DC spike or edge skip)
        if (idx == 0) return;

        // Quickselect median O(n) for noise floor
        const size_t k = idx / 2;
        uint8_t qs_left = 0;
        uint8_t qs_right = static_cast<uint8_t>(idx) - 1;

        while (qs_left < qs_right) {
            const uint8_t pivot_idx = qs_left + (qs_right - qs_left) / 2;
            uint8_t pivot = usable[pivot_idx];
            usable[pivot_idx] = usable[qs_right];
            usable[qs_right] = pivot;
            uint8_t store = qs_left;
            for (uint8_t i = qs_left; i < qs_right; ++i) {
                if (usable[i] < pivot) {
                    uint8_t t = usable[store];
                    usable[store] = usable[i];
                    usable[i] = t;
                    store++;
                }
            }
            {
                uint8_t t = usable[store];
                usable[store] = usable[qs_right];
                usable[qs_right] = t;
            }
            if (store == k) break;
            if (store < k) qs_left = store + 1;
            else qs_right = store - 1;
        }

        noise_floor = usable[k];

        // Step 2: Margin check
        const uint8_t peak_margin_fixed = raw_peak - noise_floor;
        if (peak_margin_fixed < cfg_margin) return;
    }

    // Compute peak_margin (always needed for shape analysis)
    const uint8_t peak_margin = raw_peak - noise_floor;

    // Step 3: Signal width measurement (scan left/right from peak)
    const uint8_t elevated_threshold = noise_floor + (peak_margin / 4);

    size_t sig_left = peak_index;
    while (sig_left > FFT_EDGE_SKIP_NARROW) {
        size_t prev = sig_left - 1;
        if (prev >= FFT_DC_SPIKE_START && prev < FFT_DC_SPIKE_END) { --sig_left; continue; }
        if (spectrum.db[prev] < elevated_threshold) break;
        --sig_left;
    }
    size_t sig_right = peak_index;
    while (sig_right < FFT_BIN_COUNT - FFT_EDGE_SKIP_NARROW - 1) {
        size_t next = sig_right + 1;
        if (next >= FFT_DC_SPIKE_START && next < FFT_DC_SPIKE_END) { ++sig_right; continue; }
        if (spectrum.db[next] < elevated_threshold) break;
        ++sig_right;
    }

    const size_t signal_width = sig_right - sig_left + 1;

    // Step 4-5: Width checks
    if (signal_width < cfg_min_width) return;
    if (signal_width > cfg_max_width) return;

    // Step 6+8b: Compute average margin ONCE for both sharpness and flatness checks
    int32_t avg_margin = 0;
    if (cfg_sharpness > 50 || cfg_flatness > 50) {
        int32_t margin_sum = 0;
        size_t bin_count = 0;
        for (size_t i = sig_left; i <= sig_right; ++i) {
            if (i >= FFT_DC_SPIKE_START && i < FFT_DC_SPIKE_END) continue;
            if (spectrum.db[i] > noise_floor) {
                margin_sum += (spectrum.db[i] - noise_floor);
                ++bin_count;
            }
        }
        if (bin_count > 0) {
            avg_margin = margin_sum / static_cast<int32_t>(bin_count);
        }
    }

    // Step 6: Peak sharpness check (V-shape)
    if (cfg_sharpness > 50 && avg_margin > 0) {
        const int32_t sharpness = (static_cast<int32_t>(peak_margin) * 100) / avg_margin;
        if (sharpness < cfg_sharpness) return;
    }

    // Step 7: Peak ratio check
    if (cfg_ratio > 0) {
        const int32_t ratio_val = (static_cast<int32_t>(peak_margin) * 10) / static_cast<int32_t>(signal_width);
        if (ratio_val < cfg_ratio) return;
    }

    // Step 8: Valley depth check
    if (cfg_valley > 0) {
        uint8_t left_valley = 0;
        uint8_t right_valley = 0;

        if (sig_left > FFT_EDGE_SKIP_NARROW) {
            size_t lv = sig_left - 1;
            if (!(lv >= FFT_DC_SPIKE_START && lv < FFT_DC_SPIKE_END) && spectrum.db[lv] > noise_floor) {
                left_valley = spectrum.db[lv] - noise_floor;
            }
        }
        if (sig_right < FFT_BIN_COUNT - FFT_EDGE_SKIP_NARROW - 1) {
            size_t rv = sig_right + 1;
            if (!(rv >= FFT_DC_SPIKE_START && rv < FFT_DC_SPIKE_END) && spectrum.db[rv] > noise_floor) {
                right_valley = spectrum.db[rv] - noise_floor;
            }
        }

        const uint8_t max_valley = (left_valley > right_valley) ? left_valley : right_valley;
        if (max_valley >= cfg_valley) return;
    }

    // Step 8b: Flatness check — peak must dominate average margin (reuses avg_margin from step 6)
    if (cfg_flatness > 50 && avg_margin > 0) {
        const int32_t flat = (static_cast<int32_t>(peak_margin) * 100) / avg_margin;
        if (flat < cfg_flatness) return;
    }

    // Step 8c: Symmetry check — V-shape must have similar left/right width
    if (cfg_symmetry > 0 && signal_width > 1) {
        const size_t left_w = peak_index - sig_left;
        const size_t right_w = sig_right - peak_index;
        const size_t max_s = (left_w > right_w) ? left_w : right_w;
        const size_t min_s = (left_w < right_w) ? left_w : right_w;
        if (max_s > 0) {
            const uint8_t sym_pct = static_cast<uint8_t>((min_s * 100) / max_s);
            if (sym_pct < cfg_symmetry) return;
        }
    }

    // Step 9: Convert peak to dBm
    int32_t peak_rssi = static_cast<int32_t>(raw_peak) - FFT_DBM_OFFSET;

    // Step 10: Median filter
    rssi_median_filter_.add(peak_rssi);
    if (median_filter_enabled_ && rssi_median_filter_.is_warm()) {
        peak_rssi = rssi_median_filter_.get_median();
    }

    // Step 10b: Exception check + tracking
    if (peak_rssi > cfg_rssi_thresh) {
        // Convert FFT bin index to ACTUAL RF frequency using Looking Glass mapping.
        // The pixel formula (f_min + pixel_index * step) is WRONG for FFT bins
        // because Looking Glass reorders bins. Each bin has a fixed frequency offset
        // from f_center that depends on its position in the reordering.
        const FreqHz peak_freq = fft_bin_to_freq(center_freq, peak_index);

        // Check if peak frequency is within sweep range (prevent false positives outside range)
        // This prevents detections at 6017 MHz when sweep ends at 6000 MHz
        // Fallback to config sweep range if f_min/f_max are uninitialized (0)
        const FreqHz range_min = (f_min != 0) ? f_min : config_.sweep_start_freq;
        const FreqHz range_max = (f_max != 0) ? f_max : config_.sweep_end_freq;
        if (range_min != 0 && range_max != 0) {
            if (peak_freq < range_min || peak_freq > range_max) {
                return;  // Peak outside sweep range — reject to prevent false positives
            }
        }

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

        (void)update_tracked_drone_internal(peak_freq, peak_rssi, chTimeNow());
    }
}

} // namespace drone_analyzer
