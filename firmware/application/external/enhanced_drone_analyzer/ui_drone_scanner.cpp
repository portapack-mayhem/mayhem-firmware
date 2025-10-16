// ui_drone_scanner.cpp
// Implementation of scanning algorithms and detection logic

#include "ui_drone_scanner.hpp"
#include "ui_drone_hardware.hpp"
#include "ui_drone_validation.hpp"  // ADD: Include for SimpleDroneValidation

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
      last_valid_rssi_(-120)
{
    initialize_database_and_scanner();
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
}

void DroneScanner::cleanup_database_and_scanner() {
    // PHASE 3: Proper thread cleanup with synchronization (FIXED - removed duplicate chThdWait)
    if (scanning_thread_) {
        scanning_active_ = false;  // Signal thread to stop first
        chThdWait(scanning_thread_);  // Wait for thread to actually exit
        scanning_thread_ = nullptr;
    }
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
        chThdSleepMilliseconds(750); // PHASE 3: Increased scan interval for stability (750ms)

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
    // WIDEBAND MODE: Continuous spectrum monitoring (Search-style approach)
    // This mode relies on spectrum callbacks for continuous monitoring
    // RSSI processed in handle_channel_spectrum() callbacks

    // In wideband mode, no active tuning needed - spectrum monitoring is continuous
    // Just ensure hardware is set up for wide frequency range
    if (scan_cycles_ % 100 == 0) {  // Less frequent checks
        hardware.tune_to_frequency(433000000);  // Default center frequency
    }
}

void DroneScanner::perform_hybrid_scan_cycle(DroneHardwareController& hardware) {
    // HYBRID MODE: Discovery + Database validation (Recon-style hybrid)
    // Combine wideband monitoring with database validation

    // First phase: Wideband discovery (every 10 cycles)
    if (scan_cycles_ % 10 == 0) {
        perform_wideband_scan_cycle(hardware);
    }

    // Second phase: Database validation scanning (normal database scan)
    else {
        perform_database_scan_cycle(hardware);
    }
}

void DroneScanner::process_rssi_detection(const freqman_entry& entry, int32_t rssi) {
    // STEP 1: Validate signal strength before processing
    if (rssi < -120 || rssi > -10) {
        return; // Invalid RSSI reading
    }

    // STEP 2: CHECK DATABASE THRESHOLD (fixed from Recon pattern)
    // Database has RSSI threshold per frequency - check against it
    int32_t detection_threshold = -90; // Default if no database entry

    if (database_) {
        const auto* db_entry = database_->lookup_frequency(entry.frequency_a);
        if (db_entry) {
            detection_threshold = db_entry->rssi_threshold_db;
        }
    }

    // Only count as detection if RSSI exceeds threshold (Recon pattern: if (db > squelch))
    if (rssi < detection_threshold) {
        return; // Below detection threshold
    }

    total_detections_++;  // Increment total signal detections counter

    // STEP 3: GET DRONE INFO FROM DATABASE (Search app pattern)
    DroneType detected_type = DroneType::UNKNOWN;
    ThreatLevel threat_level = ThreatLevel::LOW;

    if (database_) {
        const auto* db_entry = database_->lookup_frequency(entry.frequency_a);
        if (db_entry) {
            detected_type = db_entry->drone_type;
            threat_level = db_entry->threat_level;
        }
    }

    // STEP 5: MINIMUM DETECTION DELAY (Search pattern DETECTION_DELAY) + HYSTERESIS
    // Require multiple consecutive detections for stability + RSSI hysteresis
    static std::array<uint8_t, 512> detection_counts = {};  // EMBEDDED OPTIMIZATION: Reduced from 1024 to save memory
    static std::array<int32_t, 512> prev_rssi_values = {};  // Remember previous RSSI for hysteresis
    size_t freq_hash = entry.frequency_a % detection_counts.size();

    // HYSTERESIS PROTECTION: Require RSSI to be consistently above threshold by margin
    // Now using consolidated config constant
    int32_t effective_threshold = detection_threshold;
    if (prev_rssi_values[freq_hash] < detection_threshold) {
        effective_threshold = detection_threshold + HYSTERESIS_MARGIN_DB;  // Higher threshold for initially weak signals
    }
    prev_rssi_values[freq_hash] = rssi;  // Update for next cycle

    if (rssi >= detection_threshold) {
        detection_counts[freq_hash] = std::min((uint8_t)(detection_counts[freq_hash] + 1), (uint8_t)255);
        if (detection_counts[freq_hash] >= DETECTION_DELAY) {
            // STEP 4: UPDATE DRONE TRACKING (Search/Looking Glass pattern)
            update_tracked_drone(detected_type, entry.frequency_a, rssi, threat_level);
        }
    } else {
        detection_counts[freq_hash] = 0;  // Reset on signal loss
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

    // Update trends display callback - moved to UI layer
    // update_trends_compact_display();
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

} // namespace ui::external_app::enhanced_drone_analyzer
