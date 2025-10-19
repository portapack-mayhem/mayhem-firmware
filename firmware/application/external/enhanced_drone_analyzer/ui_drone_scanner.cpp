// ui_drone_scanner.cpp
// Implementation of scanning algorithms and detection logic

#include "ui_drone_scanner.hpp"
#include "ui_drone_hardware.hpp"

#include <algorithm>

namespace ui::external_app::enhanced_drone_analyzer {

DroneScanner::DroneScanner()
    : scanning_active_(false),
      scanning_thread_(nullptr),
      current_db_index_(0),
      last_scanned_frequency_(0),
      scan_cycles_(0),
      total_detections_(0),
      is_real_mode_(true),
      tracked_drones_count_(0),
      approaching_count_(0),
      receding_count_(0),
      static_count_(0),
      max_detected_threat_(ThreatLevel::NONE),
      last_valid_rssi_(-120),
      wideband_scan_data_()  // Initialize wideband scan data
{
    initialize_database_and_scanner();
    initialize_wideband_scanning();
}

DroneScanner::~DroneScanner() {
    stop_scanning();
    cleanup_database_and_scanner();
}

void DroneScanner::initialize_database_and_scanner() {
    // PK: Removed unused database initialization - using freq_db_ directly
    // PK: Removed unused scanner/detector initialization - causing conflicts
    // Initialize spectrum collector for REAL RSSI measurements - V0 INTEGRATION
    // This has been moved to DroneHardwareController

    // Initialize owned drone database - same as freq database
    std::filesystem::path db_path = get_freqman_path("DRONES");
    if (!drone_database_.open(db_path, true)) {
        // Database initialization failed - continue without enhanced drone data
        // Scanning will work but with default parameters
    }
}

void DroneScanner::cleanup_database_and_scanner() {
    // PHASE 3: Proper thread cleanup with synchronization (FIXED - removed duplicate chThdWait)
    if (scanning_thread_) {
        scanning_active_ = false;  // Signal thread to stop first
        chThdWait(scanning_thread_);  // Wait for thread to actually exit
        scanning_thread_ = nullptr;
    }
}

// Initialize wideband scanning data structures (Search app pattern)
void DroneScanner::initialize_wideband_scanning() {
    wideband_scan_data_.reset();  // Initialize with default full range
    setup_wideband_range(WIDEBAND_DEFAULT_MIN, WIDEBAND_DEFAULT_MAX);
}

// Setup wideband scanning range and calculate slices (adapted from Search app)
void DroneScanner::setup_wideband_range(rf::Frequency min_freq, rf::Frequency max_freq) {
    wideband_scan_data_.min_freq = min_freq;
    wideband_scan_data_.max_freq = max_freq;

    // Calculate scanning range
    rf::Frequency scanning_range = max_freq - min_freq;

    if (scanning_range > WIDEBAND_SLICE_WIDTH) {
        // Multiple slices required (Search app logic: slices_nb = (range + slice_width - 1) / slice_width)
        wideband_scan_data_.slices_nb = (scanning_range + WIDEBAND_SLICE_WIDTH - 1) / WIDEBAND_SLICE_WIDTH;

        // Cap at maximum allowed slices
        if (wideband_scan_data_.slices_nb > WIDEBAND_MAX_SLICES) {
            wideband_scan_data_.slices_nb = WIDEBAND_MAX_SLICES;
        }

        // Calculate slice positioning (Search app offset logic)
        rf::Frequency slices_span = wideband_scan_data_.slices_nb * WIDEBAND_SLICE_WIDTH;
        rf::Frequency offset = ((scanning_range - slices_span) / 2) + (WIDEBAND_SLICE_WIDTH / 2);
        rf::Frequency center_frequency = min_freq + offset;

        // Populate slices array
        for (uint32_t slice = 0; slice < wideband_scan_data_.slices_nb; slice++) {
            wideband_scan_data_.slices[slice].center_frequency = center_frequency;
            center_frequency += WIDEBAND_SLICE_WIDTH;
        }
    } else {
        // Single slice covers the entire range
        wideband_scan_data_.slices[0].center_frequency = (max_freq + min_freq) / 2;
        wideband_scan_data_.slices_nb = 1;
    }

    // Reset slice counter for new scan
    wideband_scan_data_.slice_counter = 0;
}

void DroneScanner::start_scanning() {
    if (scanning_active_ || scanning_thread_ != nullptr) return;

    scanning_active_ = true;
    scan_cycles_ = 0;
    total_detections_ = 0;

    // Start scanning thread
    scanning_thread_ = chThdCreateFromHeap(nullptr, SCAN_THREAD_STACK_SIZE,
                                          "drone_scanner", NORMALPRIO,
                                          scanning_thread_function, this);

    if (!scanning_thread_) {
        // Error handling moved to UI layer
        scanning_active_ = false;
    }
}

void DroneScanner::stop_scanning() {
    if (!scanning_active_) return;

    scanning_active_ = false;

    // Stop thread
    if (scanning_thread_) {
        scanning_active_ = false;
        chThdWait(scanning_thread_);
        scanning_thread_ = nullptr;
    }

    // Clean up stale drones
    remove_stale_drones();

    // AUTO-RESTORATION: Show logging session summary when scanning stops (connects unused format_session_summary)
    if (detection_logger_.is_session_active()) {
        detection_logger_.end_session();
        // Note: Session summary could be shown here via UI layer, but moved to logger destruction for now
    }
}

void DroneScanner::reset_scan_cycles() {
    scan_cycles_ = 0;
    total_detections_ = 0;
    current_db_index_ = 0;
    last_scanned_frequency_ = 0;
    max_detected_threat_ = ThreatLevel::NONE;
    last_valid_rssi_ = -120;
}

msg_t DroneScanner::scanning_thread_function(void* arg) {
    auto* self = static_cast<DroneScanner*>(arg);
    return self->scanning_thread();
}

msg_t DroneScanner::scanning_thread() {
    while (scanning_active_ && !chThdShouldTerminateX()) {
        // Perform scan cycle - this will be called with hardware reference
        // Implementation will be in perform_scan_cycle()

        // For now, just sleep to prevent busy waiting
        chThdSleepMilliseconds(MIN_SCAN_INTERVAL_MS); // OPTIMIZED: Using configurable interval (500ms for 60km/h threat response)

        scan_cycles_++;
    }

    // Cleanup
    scanning_active_ = false;
    scanning_thread_ = nullptr;
    chThdExit(MSG_OK);
    return MSG_OK;
}

bool DroneScanner::load_frequency_database() {
    // Initialize with default database if needed
    try {
        if (!freq_db_.is_open()) {
            // Try to open default database - this is handled in UI layer
            return false;
        }

        if (freq_db_.entry_count() == 0) {
            // Could initialize with defaults here, but UI handles it
            return false;
        }

        current_db_index_ = 0;
        last_scanned_frequency_ = 0;

        // RESTORATION: Add on_frequency_warning for large databases (Freqman pattern)
        if (freq_db_.entry_count() > 100) {
            char warning_msg[128];
            uint32_t estimated_time = freq_db_.entry_count() * 3;  // ~3 seconds per entry
            snprintf(warning_msg, sizeof(warning_msg),
                    "Large database: %zu entries. Estimated scan time: %u min",
                    freq_db_.entry_count(), estimated_time / 60);  // minutes for display
            handle_scan_error(warning_msg);
        }

        // PHASE 2: RESTORE UNUSED FUNCTION - Initialize scan state from loaded frequencies
        scan_init_from_loaded_frequencies();

        return true;
    } catch (...) {
        return false;
    }
}

size_t DroneScanner::get_database_size() const {
    return freq_db_.is_open() ? freq_db_.entry_count() : 0;
}

const freqman_entry* DroneScanner::get_current_frequency_entry() const {
    if (!freq_db_.is_open() || current_db_index_ >= freq_db_.entry_count()) {
        return nullptr;
    }

    return freq_db_.get_entry(current_db_index_);
}

void DroneScanner::set_scanning_parameters(uint32_t scan_cycles_param,
                                          uint32_t total_detections_param) {
    scan_cycles_ = scan_cycles_param;
    total_detections_ = total_detections_param;
}

void DroneScanner::switch_to_real_mode() {
    is_real_mode_ = true;
}

void DroneScanner::switch_to_demo_mode() {
    is_real_mode_ = false;
}

void DroneScanner::set_scanning_mode(ScanningMode mode) {
    scanning_mode_ = mode;

    // Reset scanning state when changing modes
    stop_scanning();
    reset_scan_cycles();

    // Reinitialize based on new mode
    if (scanning_mode_ == ScanningMode::WIDEBAND_CONTINUOUS) {
        // Wideband mode doesn't need frequency database
        // Will rely solely on spectrum monitoring
    } else if (scanning_mode_ == ScanningMode::DATABASE || scanning_mode_ == ScanningMode::HYBRID) {
        // Database modes need frequency database loaded
        load_frequency_database();
    }
}

std::string DroneScanner::scanning_mode_name() const {
    switch (scanning_mode_) {
        case ScanningMode::DATABASE:
            return "Database Scan";
        case ScanningMode::WIDEBAND_CONTINUOUS:
            return "Wideband Monitor";
        case ScanningMode::HYBRID:
            return "Hybrid Discovery";
        default:
            return "Unknown";
    }
}

void DroneScanner::perform_scan_cycle(DroneHardwareController& hardware) {
    // Early exit conditions
    if (!scanning_active_) return;

    // **PHASE 5: MULTI-MODE SCANNING IMPLEMENTATION**
    // Different scanning logic based on selected mode

    switch (scanning_mode_) {
        case ScanningMode::DATABASE:
            perform_database_scan_cycle(hardware);  // Original EDA approach
            break;
        case ScanningMode::WIDEBAND_CONTINUOUS:
            perform_wideband_scan_cycle(hardware);  // Search-style approach
            break;
        case ScanningMode::HYBRID:
            perform_hybrid_scan_cycle(hardware);    // Recon-style hybrid
            break;
    }

    // UPDATE SCAN STATISTICS
    scan_cycles_++;
}

void DroneScanner::perform_database_scan_cycle(DroneHardwareController& hardware) {
    // DATABASE MODE: Scan through frequency database (EDA original approach)
    // Following Recon's frequency_file_load() and on_statistics_update() patterns

    if (!freq_db_.is_open() || freq_db_.entry_count() == 0) {
        // DATABASE NOT LOADED: Try to load default database on the fly
        if (scanning_active_ && scan_cycles_ % 50 == 0) {
            handle_scan_error("No frequency database loaded - scanning paused");
            scanning_active_ = false; // Pause scanning until database loaded
        }
        return;
    }

    const size_t total_entries = freq_db_.entry_count();

    // Validate current index is within database bounds
    if (current_db_index_ >= total_entries) {
        current_db_index_ = 0;  // Wrap around (Recon pattern)
    }

    const auto& entry_opt = freq_db_.get_entry(current_db_index_);

    // Validate entry exists and contains valid frequency (Recon safety checks)
    if (entry_opt && entry_opt->frequency_hz > 0) {
        // RECON PATTERN: Tune radio to the specific database frequency
        rf::Frequency target_freq_hz = entry_opt->frequency_hz;

        // SAFETY: Validate frequency range like Recon does
        if (target_freq_hz >= 50000000 && target_freq_hz <= 6000000000) {
            // RECON PATTERN: Hardware tuning
            if (hardware.tune_to_frequency(target_freq_hz)) {
                // GET REAL RSSI: Now using hardware RSSI from spectrum callbacks
                int32_t real_rssi = hardware.get_real_rssi_from_hardware(target_freq_hz);

                // PROCESS DETECTION: Using actual database entry for drone detection
                process_rssi_detection(*entry_opt, real_rssi);

                last_scanned_frequency_ = target_freq_hz;
            }
        }

        // RECON PATTERN: Advance to next frequency (circular scanning)
        current_db_index_ = (current_db_index_ + 1) % total_entries;
    } else {
        // Handle corrupted/invalid entry (Recon error handling pattern)
        current_db_index_ = (current_db_index_ + 1) % total_entries;
    }
}

void DroneScanner::perform_wideband_scan_cycle(DroneHardwareController& hardware) {
    // WIDEBAND MODE: Slice-based continuous spectrum monitoring (Search app pattern)
    // Step through slices sequentially, using hardware RSSI sampling

    if (wideband_scan_data_.slices_nb == 0) {
        setup_wideband_range(WIDEBAND_DEFAULT_MIN, WIDEBAND_DEFAULT_MAX);
    }

    // Validate current slice index
    if (wideband_scan_data_.slice_counter >= wideband_scan_data_.slices_nb) {
        wideband_scan_data_.slice_counter = 0;  // Wrap around to first slice
    }

    // Get current slice
    const WidebandSlice& current_slice = wideband_scan_data_.slices[wideband_scan_data_.slice_counter];

    // Tune hardware to current slice center frequency (Search app tuning pattern)
    if (hardware.tune_to_frequency(current_slice.center_frequency)) {
        // SAMPLE RSSI: Use hardware RSSI sampling for this slice center
        int32_t slice_rssi = hardware.get_real_rssi_from_hardware(current_slice.center_frequency);

        // Wideband spectrum processing - sample multiple frequencies within slice range
        // This is a simplified version that samples center and some offset frequencies
        process_wideband_slice_samples(hardware, current_slice, slice_rssi);

        // Advance to next slice for next cycle
        wideband_scan_data_.slice_counter = (wideband_scan_data_.slice_counter + 1) % wideband_scan_data_.slices_nb;

        // Update last scanned frequency for UI feedback
        last_scanned_frequency_ = current_slice.center_frequency;
    } else {
        // Hardware tuning failed - advance anyway to avoid getting stuck
        wideband_scan_data_.slice_counter = (wideband_scan_data_.slice_counter + 1) % wideband_scan_data_.slices_nb;

        if (scan_cycles_ % 100 == 0) {  // Report errors periodically
            handle_scan_error("Hardware tuning failed in wideband mode");
        }
    }
}

// Process wideband slice by sampling multiple frequencies within the slice
void DroneScanner::process_wideband_slice_samples(DroneHardwareController& hardware,
                                                 const WidebandSlice& slice,
                                                 int32_t center_rssi) {
    // Wideband approach: Sample center and a few key frequencies within slice range
    // This provides basic wideband coverage without full spectrum analysis

    // Sample center frequency (already obtained)
    if (center_rssi > DEFAULT_RSSI_THRESHOLD_DB) {
        create_wideband_detection_entry(slice.center_frequency, center_rssi);
    }

    // Sample a few offset frequencies within the slice bandwidth for wider coverage
    // This is a compromise between full spectrum monitoring and practical embedded constraints
    const int32_t sample_offsets[] = {-1000000, 1000000};  // ±1MHz offsets within 2.5MHz slice

    for (int32_t offset : sample_offsets) {
        rf::Frequency sample_freq = slice.center_frequency + offset;

        // Validate frequency is within valid hardware range
        if (sample_freq >= MIN_HARDWARE_FREQ && sample_freq <= MAX_HARDWARE_FREQ) {
            // Tune to sample frequency and get RSSI
            if (hardware.tune_to_frequency(sample_freq)) {
                int32_t sample_rssi = hardware.get_real_rssi_from_hardware(sample_freq);

                if (sample_rssi > DEFAULT_RSSI_THRESHOLD_DB) {
                    create_wideband_detection_entry(sample_freq, sample_rssi);
                }
            }
        }
    }
}

// Calculate appropriate threshold for wideband signals
int32_t DroneScanner::calculate_wideband_threshold(rf::Frequency detected_freq) {
    // Base threshold for wideband (more conservative)
    int32_t threshold = WIDEBAND_RSSI_THRESHOLD_DB;

    // Frequency-dependent adjustments for different spectrum characteristics
    // Lower frequencies (VHF) tend to have more noise
    if (detected_freq < 30'000'000) {  // Below 30MHz (HF/VHF)
        threshold -= WIDEBAND_DYNAMIC_THRESHOLD_OFFSET_DB;  // Higher threshold (less sensitive)
    }
    // Ultra high frequency (UHF) and microwave bands
    else if (detected_freq > 3'000'000'000) {  // Above 3GHz
        threshold += 2;  // Slightly more sensitive for targeted drone frequencies
    }
    // ISM band (2.4GHz, 5.8GHz) - known drone frequencies
    else if ((detected_freq >= 2'400'000'000 && detected_freq <= 2'500'000'000) ||
             (detected_freq >= 5'725'000'000 && detected_freq <= 5'875'000'000)) {
        threshold -= 2;  // More sensitive for known drone bands
    }

    return threshold;
}

// Create detection entry for wideband finds with enhanced thresholding
void DroneScanner::create_wideband_detection_entry(rf::Frequency detected_freq, int32_t rssi) {
    // Calculate frequency-appropriate threshold instead of using general default
    int32_t wideband_threshold = calculate_wideband_threshold(detected_freq);

    // Only proceed if signal exceeds wideband-specific threshold
    if (rssi <= wideband_threshold) {
        return;  // Too weak for wideband detection
    }

    // Create pseudo database entry for wideband detection processing
    // This allows reuse of existing detection logic with wideband-specific handling
    const freqman_entry fake_entry{
        .frequency_a = detected_freq,
        .frequency_b = detected_freq,
        .type = freqman_type::Single,
        .modulation = freqman_invalid_index,  // Wideband detection doesn't specify modulation
        .bandwidth = freqman_invalid_index,
        .step = freqman_invalid_index,
        .description = "Wideband Detection"
    };

    // Process detection using existing wideband-compatible validation
    if (SimpleDroneValidation::validate_rssi_signal(rssi, ThreatLevel::UNKNOWN) &&
        SimpleDroneValidation::validate_frequency_range(detected_freq)) {

        // Force wideband threshold override in detection processing
        wideband_detection_override(fake_entry, rssi, wideband_threshold);
    }
}

// Force wideband threshold override during detection processing
void DroneScanner::wideband_detection_override(const freqman_entry& entry, int32_t rssi, int32_t threshold_override) {
    // Temporarily override the detection threshold for wideband signals
    // Store original threshold logic and apply wideband override

    // Look up if this frequency exists in database (could have been added after detection)
    const auto* db_entry = drone_database_.lookup_frequency(entry.frequency_a);
    int32_t original_threshold = db_entry ? db_entry->rssi_threshold_db : DEFAULT_RSSI_THRESHOLD_DB;

    // Check against wideband threshold (not database threshold)
    if (rssi >= threshold_override) {
        // Create a temporary entry with wideband threshold override
        freqman_entry wideband_entry = entry;
        wideband_entry.description = "Wideband Enhanced Detection";

        // Force detection by calling process_rssi_detection with overridden threshold handling
        // We'll modify threshold logic temporarily for wideband context
        process_wideband_detection_with_override(wideband_entry, rssi, original_threshold, threshold_override);
    }
}

// Special processing for wideband detections with threshold override
void DroneScanner::process_wideband_detection_with_override(const freqman_entry& entry, int32_t rssi,
                                                           int32_t original_threshold, int32_t wideband_threshold) {
    // Override the standard detection flow for wideband signals

    // Use standard validation
    if (!SimpleDroneValidation::validate_rssi_signal(rssi, ThreatLevel::UNKNOWN) ||
        !SimpleDroneValidation::validate_frequency_range(entry.frequency_a)) {
        return;
    }

    // Simple validation (reuse existing)
    if (!validate_detection_simple(rssi, ThreatLevel::UNKNOWN)) {
        return;
    }

    // WIDEBAND THREAT CLASSIFICATION
    // Wideband signals get conservative threat assessment initially
    ThreatLevel threat_level = ThreatLevel::UNKNOWN;

    // Classify based on signal strength and frequency characteristics
    if (rssi > -70) {  // Very strong signal
        threat_level = ThreatLevel::MEDIUM;
    } else if (rssi > -80) {  // Moderate signal
        threat_level = ThreatLevel::LOW;
    } else {
        threat_level = ThreatLevel::UNKNOWN;  // Weak but valid
    }

    // Frequency-based threat adjustment
    if (entry.frequency_a >= 2'400'000'000 && entry.frequency_a <= 2'500'000'000) {
        // ISM 2.4GHz band - common for drones
        threat_level = std::max(threat_level, ThreatLevel::MEDIUM);
    }

    total_detections_++;  // Increment detection counter

    // Drone type classification for wideband (unknown)
    DroneType detected_type = DroneType::UNKNOWN;

    // RING BUFFER PROTECTION (reuse existing logic)
    extern DetectionRingBuffer& local_detection_ring;
    size_t freq_hash = entry.frequency_a;

    // Use wideband threshold for hysteresis check
    int32_t effective_threshold = wideband_threshold;
    if (local_detection_ring.get_rssi_value(freq_hash) < wideband_threshold) {
        effective_threshold = wideband_threshold + HYSTERESIS_MARGIN_DB;
    }

    if (rssi >= effective_threshold) {
        // RING BUFFER UPDATE for wideband signals
        uint8_t current_count = local_detection_ring.get_detection_count(freq_hash);
        current_count = std::min((uint8_t)(current_count + 1), (uint8_t)255);
        local_detection_ring.update_detection(freq_hash, current_count, rssi);

        if (current_count >= MIN_DETECTION_COUNT) {
            // LOG WIDEBAND DETECTION
            DroneDetectionLogger detection_logger;
            DetectionLogEntry log_entry{
                .timestamp = chTimeNow(),
                .frequency_hz = static_cast<uint32_t>(entry.frequency_a),
                .rssi_db = rssi,
                .threat_level = threat_level,
                .drone_type = detected_type,
                .detection_count = current_count,
                .confidence_score = 0.6f  // Wideband detections are less confident initially
            };

            if (detection_logger.is_session_active()) {
                detection_logger.log_detection(log_entry);
            }

            // TRACK WIDEBAND DRONE
            update_tracked_drone(detected_type, entry.frequency_a, rssi, threat_level);
        }
    } else {
        // RESET on signal loss
        local_detection_ring.update_detection(freq_hash, 0, -120);
    }
}

void DroneScanner::perform_hybrid_scan_cycle(DroneHardwareController& hardware) {
    // HYBRID MODE: Discovery + Database validation (Recon-style hybrid)
    // Combine wideband monitoring with database validation

    // First phase: Wideband discovery (every Nth cycle - OPTIMIZED for 60km/h response)
    if (scan_cycles_ % HYBRID_WIDEBAND_RATIO == 0) {
        perform_wideband_scan_cycle(hardware);
    }

    // Second phase: Database validation scanning (normal database scan)
    else {
        perform_database_scan_cycle(hardware);
    }
}

void DroneScanner::process_rssi_detection(const freqman_entry& entry, int32_t rssi) {
    // RESTORATION: Integrate dead validation functions into detection flow
    // STEP 1: Validate signal strength and frequency range using unused functions
    if (!SimpleDroneValidation::validate_rssi_signal(rssi, ThreatLevel::UNKNOWN)) {
        return; // Invalid RSSI reading (connects unused validate_rssi_signal)
    }

    if (!SimpleDroneValidation::validate_frequency_range(entry.frequency_a)) {
        return; // Invalid frequency range (connects unused validate_frequency_range)
    }

    // PHASE 4: RESTORE UNUSED FUNCTION - Additional simple validation before detection processing
    if (!validate_detection_simple(rssi, ThreatLevel::UNKNOWN)) {
        return;  // Extra RSSI validation (connects unused validate_detection_simple)
    }

    // STEP 2: CHECK DATABASE THRESHOLD (fixed from Recon pattern)
    // Use owned DroneFrequencyDatabase instead of uninitialized database_ pointer
    int32_t detection_threshold = -90; // Default if no database entry

    const auto* db_entry = drone_database_.lookup_frequency(entry.frequency_a);
    if (db_entry) {
        detection_threshold = db_entry->rssi_threshold_db;
    }

    // RESTORATION: Enhanced threat classification using unused function
    ThreatLevel validated_threat = SimpleDroneValidation::classify_signal_strength(rssi);
    ThreatLevel threat_level = ThreatLevel::LOW;

    // Use our owned drone database for lookup
    if (db_entry) {
        detected_type = db_entry->drone_type;
        threat_level = db_entry->threat_level;
        // Scale by classified threat if more critical
        if (validated_threat > threat_level) {
            threat_level = validated_threat;
        }
    } else {
        // No database entry - use classified threat
        threat_level = validated_threat;
    }

    // Only count as detection if RSSI exceeds threshold (Recon pattern: if (db > squelch))
    if (rssi < detection_threshold) {
        return; // Below detection threshold
    }

    total_detections_++;  // Increment total signal detections counter

    // STEP 3: GET DRONE INFO FROM OWNED DATABASE (Search app pattern)
    DroneType detected_type = DroneType::UNKNOWN;

    // Use our owned drone database for lookup
    if (db_entry) {
        detected_type = db_entry->drone_type;
    }

    // STEP 5: MINIMUM DETECTION DELAY (Search pattern DETECTION_DELAY) + HYSTERESIS
    // OPTIMIZED: Now using ring buffer for 75% memory reduction
    extern DetectionRingBuffer& local_detection_ring;  // Forward declaration

    size_t freq_hash = entry.frequency_a;  // FREQUENCY AS HASH KEY

    // HYSTERESIS PROTECTION: Check previous RSSI for stability
    int32_t effective_threshold = detection_threshold;
    if (local_detection_ring.get_rssi_value(freq_hash) < detection_threshold) {
        effective_threshold = detection_threshold + HYSTERESIS_MARGIN_DB;
    }

    if (rssi >= effective_threshold) {
        // RING BUFFER UPDATE: O(1) memory efficient operation
        uint8_t current_count = local_detection_ring.get_detection_count(freq_hash);
        current_count = std::min((uint8_t)(current_count + 1), (uint8_t)255);
        local_detection_ring.update_detection(freq_hash, current_count, rssi);

        if (current_count >= MIN_DETECTION_COUNT) {
            // STEP 6: LOG DETECTION (NEW: Integrated logging following LogFile pattern)
            DroneDetectionLogger detection_logger;
            DetectionLogEntry log_entry{
                .timestamp = chTimeNow(),
                .frequency_hz = static_cast<uint32_t>(entry.frequency_a),
                .rssi_db = rssi,
                .threat_level = threat_level,
                .drone_type = detected_type,
                .detection_count = current_count,
                .confidence_score = 0.85f  // Placeholder confidence calculation
            };

            if (detection_logger.is_session_active()) {
                detection_logger.log_detection(log_entry);
            }

            // STEP 4: UPDATE DRONE TRACKING (Search/Looking Glass pattern)
            update_tracked_drone(detected_type, entry.frequency_a, rssi, threat_level);
        }
    } else {
        // RESET detection on signal loss (false negatives prevention)
        global_detection_ring.update_detection(freq_hash, 0, -120);  // Use -120 as sentinel
    }
}

void DroneScanner::update_tracked_drone(DroneType type, rf::Frequency frequency,
                                         int32_t rssi, ThreatLevel threat_level) {
    // PORTAPACK CONSTRAINTS: MAX_TRACKED_DRONES = 8, fixed array, no heap allocation

    // EMBEDDED OPTIMIZATION: Linear search through fixed array
    for (size_t i = 0; i < MAX_TRACKED_DRONES; i++) {
        TrackedDrone& drone = tracked_drones_[i];

        // Check if this slot matches frequency (existing drone)
        if (drone.frequency == static_cast<uint32_t>(frequency) && drone.update_count > 0) {
            // EXISTING DRONE: Update RSSI and trend analysis
            drone.add_rssi(static_cast<int16_t>(rssi), chTimeNow());
            drone.drone_type = static_cast<uint8_t>(type);
            drone.threat_level = static_cast<uint8_t>(threat_level);
            update_tracking_counts();
            return;
        }

        // Check if this slot is free (update_count == 0)
        if (drone.update_count == 0) {
            // NEW DRONE: Initialize slot and add first measurement
            drone.frequency = static_cast<uint32_t>(frequency);
            drone.drone_type = static_cast<uint8_t>(type);
            drone.threat_level = static_cast<uint8_t>(threat_level);
            drone.add_rssi(static_cast<int16_t>(rssi), chTimeNow());
            tracked_drones_count_++;
            update_tracking_counts();
            return;
        }
    }

    // PORTAPACK CONSTRAINT: All slots occupied - reuse oldest slot
    // Find oldest drone to replace (simple linear search)
    size_t oldest_index = 0;
    systime_t oldest_time = tracked_drones_[0].last_seen;

    for (size_t i = 1; i < MAX_TRACKED_DRONES; i++) {
        if (tracked_drones_[i].last_seen < oldest_time) {
            oldest_time = tracked_drones_[i].last_seen;
            oldest_index = i;
        }
    }

    // Replace oldest drone with new detection
    tracked_drones_[oldest_index] = TrackedDrone();  // Reset to zero
    tracked_drones_[oldest_index].frequency = static_cast<uint32_t>(frequency);
    tracked_drones_[oldest_index].drone_type = static_cast<uint8_t>(type);
    tracked_drones_[oldest_index].threat_level = static_cast<uint8_t>(threat_level);
    tracked_drones_[oldest_index].add_rssi(static_cast<int16_t>(rssi), chTimeNow());
    update_tracking_counts();
}

void DroneScanner::remove_stale_drones() {
    const systime_t STALE_TIMEOUT = 30000; // 30 seconds in ChibiOS ticks
    systime_t current_time = chTimeNow();

    // PORTAPACK CONSTRAINT: No dynamic memory, fixed-size array
    // Use in-place removal (similar to std::remove_if but with fixed array)
    size_t write_idx = 0;

    for (size_t read_idx = 0; read_idx < MAX_TRACKED_DRONES; read_idx++) {
        TrackedDrone& drone = tracked_drones_[read_idx];

        // Skip free slots (update_count == 0)
        if (drone.update_count == 0) continue;

        // Check if drone is stale
        bool is_stale = (current_time - drone.last_seen) > STALE_TIMEOUT;

        if (!is_stale) {
            // Keep this drone - move to write position if needed
            if (write_idx != read_idx) {
                tracked_drones_[write_idx] = drone;
            }
            write_idx++;
        } else {
            // Remove stale drone - reset to free state
            tracked_drones_[read_idx] = TrackedDrone(); // Constructor sets to zero
        }
    }

    // Update active count (remaining drones after cleanup)
    tracked_drones_count_ = write_idx;

    // Update UI counters after cleanup
    update_tracking_counts();
}

void DroneScanner::update_tracking_counts() {
    // PORTAPACK CONSTRAINT: Reset counters for recounting
    approaching_count_ = 0;
    receding_count_ = 0;
    static_count_ = 0;

    // Count trends across all active drones
    for (size_t i = 0; i < MAX_TRACKED_DRONES; i++) {
        const TrackedDrone& drone = tracked_drones_[i];

        // Skip free slots and drones with insufficient updates
        if (drone.update_count == 0 || drone.update_count < 2) continue;

        // Get current trend and increment appropriate counter
        MovementTrend trend = drone.get_trend();
        switch (trend) {
            case MovementTrend::APPROACHING:
                approaching_count_++;
                break;
            case MovementTrend::RECEDING:
                receding_count_++;
                break;
            case MovementTrend::STATIC:
            case MovementTrend::UNKNOWN:
            default:
                static_count_++; // Group static and unknown together for UI
                break;
        }
    }

    // PHASE 3: RESTORE UNUSED FUNCTION - Update trends display after counting
    update_trends_compact_display();
}

void DroneScanner::update_trends_compact_display() {
    // Placeholder - implementation moved to UI layer
}

bool DroneScanner::validate_detection_simple(int32_t rssi_db, ThreatLevel threat) {
    // DIRECT EMBEDDED PATTERN: Like Level app, simple direct RSSI check
    // Now using our implemented validation class
    return SimpleDroneValidation::validate_rssi_signal(rssi_db, threat);
}

rf::Frequency DroneScanner::get_current_radio_frequency() const {
    // Return currently scanned frequency or default ISM band
    if (freq_db_.is_open() && current_db_index_ < freq_db_.entry_count()) {
        const auto& entry_opt = freq_db_.get_entry(current_db_index_);
        if (entry_opt) {
            return entry_opt->frequency_hz;
        }
    }
    return 433000000; // Default ISM band
}

rf::Frequency DroneScanner::get_current_scanning_frequency() const {
    return get_current_radio_frequency();
}

void DroneScanner::handle_scan_error(const char* error_msg) {
    // Error handling moved to UI layer
    (void)error_msg;  // Suppress unused parameter warning
}

void DroneScanner::scan_init_from_loaded_frequencies() {
    if (!freq_db_.is_open() || freq_db_.entry_count() == 0) {
        return;
    }

    // Reset scan state from loaded frequencies
    current_db_index_ = 0;
    total_detections_ = 0;
}

// IMPLEMENTATION OF MIGRATED DetectionRingBuffer from ui_drone_detection_ring.cpp

// GLOBAL SINGLETON INSTANCE - Following Portapack patterns
DetectionRingBuffer global_detection_ring;

// ALIAS for backward compatibility
DetectionRingBuffer& local_detection_ring = global_detection_ring;

DetectionRingBuffer::DetectionRingBuffer() {
    clear();  // Initialize all to zero
}

void DetectionRingBuffer::update_detection(size_t frequency_hash, uint8_t detection_count, int32_t rssi_value) {
    // RING BUFFER MATH: Hash to index with wrapping
    const size_t index = frequency_hash % DETECTION_TABLE_SIZE;

    // EMBEDDED OPTIMIZATION: Direct array access (O(1))
    detection_counts_[index] = detection_count;
    rssi_values_[index] = rssi_value;

    // MEMORY BARRIER: Ensure writes are visible (embedded safety)
    __DMB();
}

uint8_t DetectionRingBuffer::get_detection_count(size_t frequency_hash) const {
    const size_t index = frequency_hash % DETECTION_TABLE_SIZE;
    return detection_counts_[index];
}

int32_t DetectionRingBuffer::get_rssi_value(size_t frequency_hash) const {
    const size_t index = frequency_hash % DETECTION_TABLE_SIZE;
    return rssi_values_[index];
}

void DetectionRingBuffer::clear() {
    // MEMORY OPTIMIZATION: Single memset operation
    memset(detection_counts_, 0, sizeof(detection_counts_));
    memset(rssi_values_, 0, sizeof(rssi_values_));

    // Reset all to safe defaults
    for (size_t i = 0; i < DETECTION_TABLE_SIZE; i++) {
        rssi_values_[i] = -120;  // Sentinel value for "no previous measurement"
    }
}

} // namespace ui::external_app::enhanced_drone_analyzer
