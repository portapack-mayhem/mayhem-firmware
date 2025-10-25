// ui_scanner_combined.cpp - Unified implementation for Enhanced Drone Analyzer Scanner App
// Combines implementations from: ui_drone_scanner.cpp, ui_drone_hardware.cpp, ui_drone_ui.cpp
// Combines all required implementations for scanner functionality

#include "ui_scanner_combined.hpp"
#include "ui_drone_audio.hpp"
#include "gradient.hpp"
#include <algorithm>
#include <sstream>
#include <mutex>

namespace ui::external_app::enhanced_drone_analyzer {

// ===========================================
// PART 1: DETECTION RING BUFFER IMPLEMENTATION (from ui_drone_scanner.cpp)
// ===========================================

DetectionRingBuffer global_detection_ring;
DetectionRingBuffer& local_detection_ring = global_detection_ring;

DetectionRingBuffer::DetectionRingBuffer()
    : detection_counts_{}, rssi_values_{}
{
    clear();
}

void DetectionRingBuffer::update_detection(size_t frequency_hash, uint8_t detection_count, int32_t rssi_value) {
    const size_t index = frequency_hash % DETECTION_TABLE_SIZE;
    detection_counts_[index] = detection_count;
    rssi_values_[index] = rssi_value;
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
    memset(detection_counts_, 0, sizeof(detection_counts_));
    memset(rssi_values_, 0, sizeof(rssi_values_));
    for (size_t i = 0; i < DETECTION_TABLE_SIZE; i++) {
        rssi_values_[i] = -120;
    }
}

// ===========================================
// PART 2: DRONE SCANNER IMPLEMENTATION (from ui_drone_scanner.cpp)
// ===========================================

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
      wideband_scan_data_()
{
    initialize_database_and_scanner();
    initialize_wideband_scanning();
}

DroneScanner::~DroneScanner() {
    stop_scanning();
    cleanup_database_and_scanner();
}

void DroneScanner::initialize_database_and_scanner() {
    std::filesystem::path db_path = get_freqman_path("DRONES");
    if (!drone_database_.open(db_path, true)) {
        // Continue without enhanced drone data
    }
}

void DroneScanner::cleanup_database_and_scanner() {
    if (scanning_thread_) {
        scanning_active_ = false;
        chThdWait(scanning_thread_);
        scanning_thread_ = nullptr;
    }
}

void DroneScanner::initialize_wideband_scanning() {
    wideband_scan_data_.reset();
    setup_wideband_range(WIDEBAND_DEFAULT_MIN, WIDEBAND_DEFAULT_MAX);
}

void DroneScanner::setup_wideband_range(Frequency min_freq, Frequency max_freq) {
    wideband_scan_data_.min_freq = min_freq;
    wideband_scan_data_.max_freq = max_freq;

    Frequency scanning_range = max_freq - min_freq;
    if (scanning_range > WIDEBAND_SLICE_WIDTH) {
        wideband_scan_data_.slices_nb = (scanning_range + WIDEBAND_SLICE_WIDTH - 1) / WIDEBAND_SLICE_WIDTH;
        if (wideband_scan_data_.slices_nb > WIDEBAND_MAX_SLICES) {
            wideband_scan_data_.slices_nb = WIDEBAND_MAX_SLICES;
        }
        Frequency slices_span = wideband_scan_data_.slices_nb * WIDEBAND_SLICE_WIDTH;
        Frequency offset = ((scanning_range - slices_span) / 2) + (WIDEBAND_SLICE_WIDTH / 2);
        Frequency center_frequency = min_freq + offset;

        std::generate_n(wideband_scan_data_.slices,
                       wideband_scan_data_.slices_nb,
                       [&center_frequency]() mutable -> WidebandSlice {
                           WidebandSlice slice;
                           slice.center_frequency = center_frequency;
                           center_frequency += WIDEBAND_SLICE_WIDTH;
                           return slice;
                       });
    } else {
        wideband_scan_data_.slices[0].center_frequency = (max_freq + min_freq) / 2;
        wideband_scan_data_.slices_nb = 1;
    }
    wideband_scan_data_.slice_counter = 0;
}

void DroneScanner::start_scanning() {
    if (scanning_active_ || scanning_thread_ != nullptr) return;

    scanning_active_ = true;
    scan_cycles_ = 0;
    total_detections_ = 0;

    scanning_thread_ = chThdCreateFromHeap(nullptr, SCAN_THREAD_STACK_SIZE,
                                          "drone_scanner", NORMALPRIO,
                                          scanning_thread_function, this);
    if (!scanning_thread_) {
        scanning_active_ = false;
    }
}

void DroneScanner::stop_scanning() {
    if (!scanning_active_) return;

    scanning_active_ = false;
    if (scanning_thread_) {
        chThdWait(scanning_thread_);
        scanning_thread_ = nullptr;
    }
    remove_stale_drones();

    if (detection_logger_.is_session_active()) {
        detection_logger_.end_session();
    }
}

msg_t DroneScanner::scanning_thread_function(void* arg) {
    auto* self = static_cast<DroneScanner*>(arg);
    return self->scanning_thread();
}

msg_t DroneScanner::scanning_thread() {
    while (scanning_active_ && !chThdShouldTerminateX()) {
        chThdSleepMilliseconds(MIN_SCAN_INTERVAL_MS);
        scan_cycles_++;
    }
    scanning_active_ = false;
    scanning_thread_ = nullptr;
    chThdExit(MSG_OK);
    return MSG_OK;
}

bool DroneScanner::load_frequency_database() {
    try {
        if (!freq_db_.is_open()) {
            return false;
        }
        if (freq_db_.entry_count() == 0) {
            return false;
        }
        current_db_index_ = 0;
        last_scanned_frequency_ = 0;

        if (freq_db_.entry_count() > 100) {
            handle_scan_error("Large database loaded");
        }
        scan_init_from_loaded_frequencies();
        return true;
    } catch (...) {
        return false;
    }
}

size_t DroneScanner::get_database_size() const {
    return freq_db_.is_open() ? freq_db_.entry_count() : 0;
}

void DroneScanner::set_scanning_mode(ScanningMode mode) {
    scanning_mode_ = mode;
    stop_scanning();
    reset_scan_cycles();

    if (scanning_mode_ == ScanningMode::DATABASE || scanning_mode_ == ScanningMode::HYBRID) {
        load_frequency_database();
    }
}

std::string DroneScanner::scanning_mode_name() const {
    switch (scanning_mode_) {
        case ScanningMode::DATABASE: return "Database Scan";
        case ScanningMode::WIDEBAND_CONTINUOUS: return "Wideband Monitor";
        case ScanningMode::HYBRID: return "Hybrid Discovery";
        default: return "Unknown";
    }
}

void DroneScanner::perform_scan_cycle(DroneHardwareController& hardware) {
    if (!scanning_active_) return;

    switch (scanning_mode_) {
        case ScanningMode::DATABASE:
            perform_database_scan_cycle(hardware);
            break;
        case ScanningMode::WIDEBAND_CONTINUOUS:
            perform_wideband_scan_cycle(hardware);
            break;
        case ScanningMode::HYBRID:
            perform_hybrid_scan_cycle(hardware);
            break;
    }
    scan_cycles_++;
}

void DroneScanner::perform_database_scan_cycle(DroneHardwareController& hardware) {
    if (!freq_db_.is_open() || freq_db_.entry_count() == 0) {
        if (scan_cycles_ % 50 == 0) {
            handle_scan_error("No frequency database loaded");
            scanning_active_ = false;
        }
        return;
    }

    const size_t total_entries = freq_db_.entry_count();
    if (current_db_index_ >= total_entries) {
        current_db_index_ = 0;
    }

    const auto& entry_opt = freq_db_.get_entry(current_db_index_);
    if (entry_opt && entry_opt->frequency_hz > 0) {
        Frequency target_freq_hz = entry_opt->frequency_hz;
        if (target_freq_hz >= 50000000 && target_freq_hz <= 6000000000) {
            if (hardware.tune_to_frequency(target_freq_hz)) {
                int32_t real_rssi = hardware.get_real_rssi_from_hardware(target_freq_hz);
                process_rssi_detection(*entry_opt, real_rssi);
                last_scanned_frequency_ = target_freq_hz;
            }
        }
        current_db_index_ = (current_db_index_ + 1) % total_entries;
    } else {
        current_db_index_ = (current_db_index_ + 1) % total_entries;
    }
}

void DroneScanner::perform_wideband_scan_cycle(DroneHardwareController& hardware) {
    if (wideband_scan_data_.slices_nb == 0) {
        setup_wideband_range(WIDEBAND_DEFAULT_MIN, WIDEBAND_DEFAULT_MAX);
    }

    if (wideband_scan_data_.slice_counter >= wideband_scan_data_.slices_nb) {
        wideband_scan_data_.slice_counter = 0;
    }

    const WidebandSlice& current_slice = wideband_scan_data_.slices[wideband_scan_data_.slice_counter];
    if (hardware.tune_to_frequency(current_slice.center_frequency)) {
        int32_t slice_rssi = hardware.get_real_rssi_from_hardware(current_slice.center_frequency);
        if (slice_rssi > DEFAULT_RSSI_THRESHOLD_DB) {
            freqman_entry fake_entry{
                .frequency_a = current_slice.center_frequency,
                .frequency_b = current_slice.center_frequency,
                .type = static_cast<uint8_t>(freqman_type::Single),
                .modulation = freqman_invalid_index,
                .bandwidth = freqman_invalid_index,
                .step = freqman_invalid_index,
                .description = "Wideband Detection"
            };
            wideband_detection_override(fake_entry, slice_rssi, WIDEBAND_RSSI_THRESHOLD_DB);
        }
        last_scanned_frequency_ = current_slice.center_frequency;
    } else {
        if (scan_cycles_ % 100 == 0) {
            handle_scan_error("Hardware tuning failed in wideband mode");
        }
    }
    wideband_scan_data_.slice_counter = (wideband_scan_data_.slice_counter + 1) % wideband_scan_data_.slices_nb;
}

void DroneScanner::wideband_detection_override(const freqman_entry& entry, int32_t rssi, int32_t threshold_override) {
    if (rssi >= threshold_override) {
        freqman_entry wideband_entry = entry;
        wideband_entry.description = "Wideband Enhanced Detection";
        process_wideband_detection_with_override(wideband_entry, rssi, DEFAULT_RSSI_THRESHOLD_DB, threshold_override);
    }
}

void DroneScanner::process_wideband_detection_with_override(const freqman_entry& entry, int32_t rssi,
                                                           int32_t original_threshold, int32_t wideband_threshold) {
    if (!SimpleDroneValidation::validate_rssi_signal(rssi, ThreatLevel::UNKNOWN) ||
        !SimpleDroneValidation::validate_frequency_range(entry.frequency_a)) {
        return;
    }

    ThreatLevel threat_level;
    if (rssi > -70) {
        threat_level = ThreatLevel::HIGH;
    } else if (rssi > -80) {
        threat_level = ThreatLevel::LOW;
    } else {
        threat_level = ThreatLevel::UNKNOWN;
    }

    if (entry.frequency_a >= 2'400'000'000 && entry.frequency_a <= 2'500'000'000) {
        threat_level = std::max(threat_level, ThreatLevel::MEDIUM);
    }

    total_detections_++;
    DroneType detected_type = DroneType::UNKNOWN;

    size_t freq_hash = entry.frequency_a;
    if (local_detection_ring.get_rssi_value(freq_hash) < wideband_threshold) {
        effective_threshold = wideband_threshold + HYSTERESIS_MARGIN_DB;
    }

    if (rssi >= effective_threshold) {
        uint8_t current_count = local_detection_ring.get_detection_count(freq_hash);
        current_count = std::min(static_cast<uint8_t>(current_count + 1), static_cast<uint8_t>(255));
        local_detection_ring.update_detection(freq_hash, current_count, rssi);

        if (current_count >= MIN_DETECTION_COUNT) {
            DetectionLogEntry log_entry{
                .timestamp = chTimeNow(),
                .frequency_hz = static_cast<uint32_t>(entry.frequency_a),
                .rssi_db = rssi,
                .threat_level = threat_level,
                .drone_type = detected_type,
                .detection_count = current_count,
                .confidence_score = 0.6f
            };

            if (detection_logger_.is_session_active()) {
                detection_logger_.log_detection(log_entry);
            }
            update_tracked_drone(detected_type, entry.frequency_a, rssi, threat_level);
        }
    } else {
        local_detection_ring.update_detection(freq_hash, 0, -120);
    }
}

void DroneScanner::perform_hybrid_scan_cycle(DroneHardwareController& hardware) {
    if (scan_cycles_ % 2 == 0) {  // Alternate every cycle
        perform_wideband_scan_cycle(hardware);
    } else {
        perform_database_scan_cycle(hardware);
    }
}

void DroneScanner::process_rssi_detection(const freqman_entry& entry, int32_t rssi) {
    if (!SimpleDroneValidation::validate_rssi_signal(rssi, ThreatLevel::UNKNOWN)) {
        return;
    }

    if (!SimpleDroneValidation::validate_frequency_range(entry.frequency_a)) {
        return;
    }

    if (!validate_detection_simple(rssi, ThreatLevel::UNKNOWN)) {
        return;
    }

    int32_t detection_threshold = -90;
    const auto* db_entry = drone_database_.lookup_frequency(entry.frequency_a);
    if (db_entry) {
        detection_threshold = db_entry->rssi_threshold_db;
    }

    ThreatLevel validated_threat = SimpleDroneValidation::classify_signal_strength(rssi);
    ThreatLevel threat_level = ThreatLevel::LOW;

    DroneType detected_type = DroneType::UNKNOWN;
    if (db_entry) {
        detected_type = db_entry->drone_type;
        threat_level = db_entry->threat_level;
        if (validated_threat > threat_level) {
            threat_level = validated_threat;
        }
    } else {
        threat_level = validated_threat;
    }

    if (rssi < detection_threshold) {
        return;
    }

    total_detections_++;
    if (db_entry) {
        detected_type = db_entry->drone_type;
    }

    size_t freq_hash = entry.frequency_a;
    int32_t effective_threshold = detection_threshold;
    if (local_detection_ring.get_rssi_value(freq_hash) < detection_threshold) {
        effective_threshold = detection_threshold + HYSTERESIS_MARGIN_DB;
    }

    if (rssi >= effective_threshold) {
        uint8_t current_count = local_detection_ring.get_detection_count(freq_hash);
        current_count = std::min(static_cast<uint8_t>(current_count + 1), static_cast<uint8_t>(255));
        local_detection_ring.update_detection(freq_hash, current_count, rssi);

        if (current_count >= MIN_DETECTION_COUNT) {
            chThdSleepMilliseconds(DETECTION_DELAY);

            DetectionLogEntry log_entry{
                .timestamp = chTimeNow(),
                .frequency_hz = static_cast<uint32_t>(entry.frequency_a),
                .rssi_db = rssi,
                .threat_level = threat_level,
                .drone_type = detected_type,
                .detection_count = current_count,
                .confidence_score = 0.85f
            };

            if (detection_logger_.is_session_active()) {
                detection_logger_.log_detection(log_entry);
            }
            update_tracked_drone(detected_type, entry.frequency_a, rssi, threat_level);
        }
    } else {
        local_detection_ring.update_detection(freq_hash, 0, -120);
    }
}

void DroneScanner::update_tracked_drone(DroneType type, Frequency frequency, int32_t rssi, ThreatLevel threat_level) {
    for (size_t i = 0; i < MAX_TRACKED_DRONES; i++) {
        TrackedDrone& drone = tracked_drones_[i];
        if (drone.frequency == static_cast<uint32_t>(frequency) && drone.update_count > 0) {
            drone.add_rssi(static_cast<int16_t>(rssi), chTimeNow());
            drone.drone_type = static_cast<uint8_t>(type);
            drone.threat_level = static_cast<uint8_t>(threat_level);
            update_tracking_counts();
            return;
        }
        if (drone.update_count == 0) {
            drone.frequency = static_cast<uint32_t>(frequency);
            drone.drone_type = static_cast<uint8_t>(type);
            drone.threat_level = static_cast<uint8_t>(threat_level);
            drone.add_rssi(static_cast<int16_t>(rssi), chTimeNow());
            tracked_drones_count_++;
            update_tracking_counts();
            return;
        }
    }

    // Replace oldest drone
    size_t oldest_index = 0;
    systime_t oldest_time = tracked_drones_[0].last_seen;
    for (size_t i = 1; i < MAX_TRACKED_DRONES; i++) {
        if (tracked_drones_[i].last_seen < oldest_time) {
            oldest_time = tracked_drones_[i].last_seen;
            oldest_index = i;
        }
    }

    tracked_drones_[oldest_index] = TrackedDrone();
    tracked_drones_[oldest_index].frequency = static_cast<uint32_t>(frequency);
    tracked_drones_[oldest_index].drone_type = static_cast<uint8_t>(type);
    tracked_drones_[oldest_index].threat_level = static_cast<uint8_t>(threat_level);
    tracked_drones_[oldest_index].add_rssi(static_cast<int16_t>(rssi), chTimeNow());
    update_tracking_counts();
}

void DroneScanner::remove_stale_drones() {
    const systime_t STALE_TIMEOUT = 30000;
    systime_t current_time = chTimeNow();

    size_t write_idx = 0;
    for (size_t read_idx = 0; read_idx < MAX_TRACKED_DRONES; read_idx++) {
        const TrackedDrone& drone = tracked_drones_[read_idx];
        if (drone.update_count == 0) continue;

        bool is_stale = (current_time - drone.last_seen) > STALE_TIMEOUT;
        if (!is_stale) {
            if (write_idx != read_idx) {
                tracked_drones_[write_idx] = drone;
            }
            write_idx++;
        } else {
            tracked_drones_[read_idx] = TrackedDrone();
        }
    }

    tracked_drones_count_ = write_idx;
    update_tracking_counts();
}

void DroneScanner::update_tracking_counts() {
    approaching_count_ = 0;
    receding_count_ = 0;
    static_count_ = 0;

    for (size_t i = 0; i < MAX_TRACKED_DRONES; i++) {
        const TrackedDrone& drone = tracked_drones_[i];
        if (drone.update_count < 2) continue;

        MovementTrend trend = drone.get_trend();
        switch (trend) {
            case MovementTrend::APPROACHING: approaching_count_++; break;
            case MovementTrend::RECEDING: receding_count_++; break;
            case MovementTrend::STATIC:
            case MovementTrend::UNKNOWN:
            default: static_count_++; break;
        }
    }
    update_trends_compact_display();
}

void DroneScanner::update_trends_compact_display() {
    // Placeholder - implementation moved to UI layer
}

bool DroneScanner::validate_detection_simple(int32_t rssi_db, ThreatLevel threat) {
    return SimpleDroneValidation::validate_rssi_signal(rssi_db, threat);
}

Frequency DroneScanner::get_current_scanning_frequency() const {
    if (freq_db_.is_open() && current_db_index_ < freq_db_.entry_count()) {
        const auto& entry_opt = freq_db_.get_entry(current_db_index_);
        if (entry_opt) {
            return entry_opt->frequency_hz;
        }
    }
    return 433000000;
}

const TrackedDrone& DroneScanner::getTrackedDrone(size_t index) const {
    return (index < MAX_TRACKED_DRONES) ? tracked_drones_[index] : tracked_drones_[0];
}

std::string DroneScanner::get_session_summary() const {
    return detection_logger_.format_session_summary(get_scan_cycles(), get_total_detections());
}

void DroneScanner::handle_scan_error(const char* error_msg) {
    (void)error_msg;
}

// DroneScanner::DroneDetectionLogger implementations
inline DroneScanner::DroneDetectionLogger::DroneDetectionLogger()
    : session_active_(false), session_start_(0), logged_count_(0), header_written_(false) {
    start_session();
}

inline DroneScanner::DroneDetectionLogger::~DroneDetectionLogger() {
    end_session();
}

inline void DroneScanner::DroneDetectionLogger::start_session() {
    if (session_active_) return;
    session_active_ = true;
    session_start_ = chTimeNow();
    logged_count_ = 0;
    header_written_ = false;
}

inline void DroneScanner::DroneDetectionLogger::end_session() {
    if (!session_active_) return;
    session_active_ = false;
}

inline bool DroneScanner::DroneDetectionLogger::log_detection(const DetectionLogEntry& entry) {
    if (!session_active_) return false;
    if (!ensure_csv_header()) return false;

    std::string csv_entry = format_csv_entry(entry);
    auto error = csv_log_.append(generate_log_filename().string());
    if (error.is_error()) return false;
    error = csv_log_.write_raw(csv_entry);
    if (!error.is_error()) {
        logged_count_++;
        return true;
    }
    return false;
}

inline bool DroneScanner::DroneDetectionLogger::ensure_csv_header() {
    if (header_written_) return true;
    const char* header = "timestamp_ms,frequency_hz,rssi_db,threat_level,drone_type,detection_count,confidence\n";
    auto error = csv_log_.append(generate_log_filename());
    if (error.is_error()) return false;
    error = csv_log_.write_raw(header);
    if (!error.is_error()) {
        header_written_ = true;
        return true;
    }
    return false;
}

inline std::string DroneScanner::DroneDetectionLogger::format_csv_entry(const DetectionLogEntry& entry) {
    char buffer[128];
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer) - 1,
             "%u,%u,%d,%u,%u,%u,%.2f\n",
             entry.timestamp, entry.frequency_hz, entry.rssi_db,
             static_cast<uint8_t>(entry.threat_level),
             static_cast<uint8_t>(entry.drone_type),
             entry.detection_count, entry.confidence_score);
    return std::string(buffer);
}

inline std::filesystem::path DroneScanner::DroneDetectionLogger::generate_log_filename() const {
    return std::filesystem::path("EDA_LOG.CSV");
}

inline std::string DroneScanner::DroneDetectionLogger::format_session_summary(size_t scan_cycles, size_t total_detections) const {
    uint32_t session_duration_ms = chTimeNow() - session_start_;
    float avg_detections_per_cycle = scan_cycles > 0 ? static_cast<float>(total_detections) / scan_cycles : 0.0f;
    float detections_per_second = session_duration_ms > 0 ?
        static_cast<float>(total_detections) * 1000.0f / session_duration_ms : 0.0f;

    char summary_buffer[512];
    memset(summary_buffer, 0, sizeof(summary_buffer));
    int ret = snprintf(summary_buffer, sizeof(summary_buffer) - 1,
        "SCANNING SESSION COMPLETE\n========================\n\nSESSION STATISTICS:\nDuration: %.1f seconds\nScan Cycles: %zu\nTotal Detections: %zu\n\nPERFORMANCE:\nAvg. detections/cycle: %.2f\nDetection rate: %.1f/sec\nLogged entries: %lu\n\nEnhanced Drone Analyzer v0.3",
        static_cast<float>(session_duration_ms) / 1000.0f, scan_cycles, total_detections,
        avg_detections_per_cycle, detections_per_second, logged_count_);

    if (ret < 0 || ret >= static_cast<int>(sizeof(summary_buffer))) {
        return std::string("SCANNING COMPLETE\nCycles: ") + std::to_string(scan_cycles) +
               "\nDetections: " + std::to_string(total_detections);
    }
    return std::string(summary_buffer);
}

// ===========================================
// PART 3: HARDWARE CONTROLLER IMPLEMENTATION (from ui_drone_hardware.cpp)
// ===========================================

DroneHardwareController::DroneHardwareController(SpectrumMode mode)
    : spectrum_mode_(mode), center_frequency_(2400000000ULL), bandwidth_hz_(24000000),
      spectrum_streaming_active_(false), last_valid_rssi_(-120)
{
}

DroneHardwareController::~DroneHardwareController() {
    shutdown_hardware();
}

void DroneHardwareController::initialize_hardware() {
    initialize_radio_state();
    initialize_spectrum_collector();
}

void DroneHardwareController::on_hardware_show() {
    initialize_hardware();
}

void DroneHardwareController::on_hardware_hide() {
    cleanup_spectrum_collector();
}

void DroneHardwareController::shutdown_hardware() {
    stop_spectrum_streaming();
    cleanup_spectrum_collector();
}

void DroneHardwareController::initialize_radio_state() {
    // Radio state initialization
}

void DroneHardwareController::initialize_spectrum_collector() {
    message_handler_spectrum_config_ = MessageHandlerRegistration(
        Message::ID::ChannelSpectrumConfigChange,
        [this](const Message* const p) { handle_channel_spectrum_config((const ChannelSpectrumConfigMessage*)p); });

    message_handler_frame_sync_ = MessageHandlerRegistration(
        Message::ID::DisplayFrameSync,
        [this](const Message* const p) { (void)p; process_channel_spectrum_data({}); });
}

void DroneHardwareController::cleanup_spectrum_collector() {
    spectrum_streaming_active_ = false;
}

void DroneHardwareController::set_spectrum_mode(SpectrumMode mode) {
    spectrum_mode_ = mode;
}

uint32_t DroneHardwareController::get_spectrum_bandwidth() const {
    return bandwidth_hz_;
}

void DroneHardwareController::set_spectrum_bandwidth(uint32_t bandwidth_hz) {
    bandwidth_hz_ = bandwidth_hz;
}

Frequency DroneHardwareController::get_spectrum_center_frequency() const {
    return center_frequency_;
}

void DroneHardwareController::set_spectrum_center_frequency(Frequency center_freq) {
    center_frequency_ = center_freq;
}

bool DroneHardwareController::tune_to_frequency(Frequency frequency_hz) {
    radio_state_.configure_tuning(frequency_hz);
    return true;
}

void DroneHardwareController::start_spectrum_streaming() {
    if (spectrum_streaming_active_) return;
    spectrum_streaming_active_ = true;
    radio_state_.start_sampling();
}

void DroneHardwareController::stop_spectrum_streaming() {
    spectrum_streaming_active_ = false;
    radio_state_.stop_sampling();
}

int32_t DroneHardwareController::get_real_rssi_from_hardware(Frequency target_frequency) {
    // Simulated RSSI for demo
    return last_valid_rssi_;
}

void DroneHardwareController::handle_channel_spectrum_config(const ChannelSpectrumConfigMessage* const message) {
    (void)message;
    // Handle spectrum config messages
}

void DroneHardwareController::process_channel_spectrum_data(const ChannelSpectrum& spectrum) {
    (void)spectrum;
    // Process spectrum data
}

int32_t DroneHardwareController::get_configured_sampling_rate() const {
    return bandwidth_hz_;
}

int32_t DroneHardwareController::get_configured_bandwidth() const {
    return bandwidth_hz_;
}

// ===========================================
// PART 4: UI IMPLEMENTATIONS (from ui_drone_ui.cpp)
// ===========================================

SmartThreatHeader::SmartThreatHeader(Rect parent_rect)
    : View(parent_rect) {
    add_children({&threat_progress_bar_, &threat_status_main_, &threat_frequency_});
    update(ThreatLevel::NONE, 0, 0, 0, 2400000000ULL, false);
}

void SmartThreatHeader::update(ThreatLevel max_threat, size_t approaching, size_t static_count,
                               size_t receding, Frequency current_freq, bool is_scanning) {
    current_threat_ = max_threat;
    is_scanning_ = is_scanning;
    current_freq_ = current_freq;
    approaching_count_ = approaching;
    static_count_ = static_count;
    receding_count_ = receding;

    size_t total_drones = approaching + static_count + receding;
    threat_progress_bar_.set_value(total_drones * 10);
    threat_progress_bar_.set_style(get_threat_bar_color(max_threat));

    char buffer[64];
    std::string threat_name = get_threat_icon_text(max_threat);
    if (total_drones > 0) {
        snprintf(buffer, sizeof(buffer), "THREAT: %s | ▲%zu ■%zu ▼%zu",
                threat_name.c_str(), approaching, static_count, receding);
    } else if (is_scanning) {
        snprintf(buffer, sizeof(buffer), "SCANNING: ▲%zu ■%zu ▼%zu",
                approaching, static_count, receding);
    } else {
        snprintf(buffer, sizeof(buffer), "READY: No Threats Detected");
    }
    threat_status_main_.set(buffer);
    threat_status_main_.set_style(get_threat_text_color(max_threat));

    if (current_freq > 0) {
        float freq_mhz = static_cast<float>(current_freq) / 1000000.0f;
        if (freq_mhz >= 1000) {
            freq_mhz /= 1000;
            if (is_scanning) {
                snprintf(buffer, sizeof(buffer), "%.2fGHz SCANNING", freq_mhz);
            } else {
                snprintf(buffer, sizeof(buffer), "%.2fGHz READY", freq_mhz);
            }
        } else {
            if (is_scanning) {
                snprintf(buffer, sizeof(buffer), "%.1fMHz SCANNING", freq_mhz);
            } else {
                snprintf(buffer, sizeof(buffer), "%.1fMHz READY", freq_mhz);
            }
        }
        threat_frequency_.set(buffer);
    } else {
        threat_frequency_.set("NO SIGNAL");
    }
    threat_frequency_.set_style(get_threat_text_color(max_threat));
    set_dirty();
}

void SmartThreatHeader::set_max_threat(ThreatLevel threat) {
    if (threat != current_threat_) {
        update(threat, approaching_count_, static_count_, receding_count_,
               current_freq_, is_scanning_);
    }
}

void SmartThreatHeader::set_movement_counts(size_t approaching, size_t static_count, size_t receding) {
    update(current_threat_, approaching, static_count, receding,
           current_freq_, is_scanning_);
}

void SmartThreatHeader::set_current_frequency(Frequency freq) {
    if (freq != current_freq_) {
        update(current_threat_, approaching_count_, static_count_, receding_count_,
               freq, is_scanning_);
    }
}

void SmartThreatHeader::set_scanning_state(bool is_scanning) {
    if (is_scanning != is_scanning_) {
        update(current_threat_, approaching_count_, static_count_, receding_count_,
               current_freq_, is_scanning);
    }
}

void SmartThreatHeader::set_color_scheme(bool use_dark_theme) {
    (void)use_dark_theme;
}

Color SmartThreatHeader::get_threat_bar_color(ThreatLevel level) const {
    switch (level) {
        case ThreatLevel::CRITICAL: return Color::red();
        case ThreatLevel::HIGH: return Color(255, 140, 0);
        case ThreatLevel::MEDIUM: return Color::yellow();
        case ThreatLevel::LOW: return Color::green();
        case ThreatLevel::NONE:
        default: return Color::blue();
    }
}

Color SmartThreatHeader::get_threat_text_color(ThreatLevel level) const {
    switch (level) {
        case ThreatLevel::CRITICAL: return Color::red();
        case ThreatLevel::HIGH: return Color(255, 165, 0);
        case ThreatLevel::MEDIUM: return Color::yellow();
        case ThreatLevel::LOW: return Color::green();
        case ThreatLevel::NONE:
        default: return Color::white();
    }
}

std::string SmartThreatHeader::get_threat_icon_text(ThreatLevel level) const {
    switch (level) {
        case ThreatLevel::CRITICAL: return "CRITICAL 🔴";
        case ThreatLevel::HIGH: return "HIGH 🟠";
        case ThreatLevel::MEDIUM: return "MEDIUM 🟡";
        case ThreatLevel::LOW: return "LOW 🟢";
        case ThreatLevel::NONE:
        default: return "CLEAR ✅";
    }
}

void SmartThreatHeader::paint(Painter& painter) {
    View::paint(painter);
    if (current_threat_ >= ThreatLevel::HIGH) {
        static uint32_t pulse_timer = 0;
        pulse_timer++;
        uint8_t alpha = (pulse_timer % 20) < 10 ? 50 : 100;
        Color pulse_color = get_threat_bar_color(current_threat_);
        pulse_color = Color(pulse_color.r, pulse_color.g, pulse_color.b, alpha);
        painter.fill_rectangle({parent_rect_.left(), parent_rect_.top(), parent_rect_.width(), 4}, pulse_color);
    }
}

ThreatCard::ThreatCard(size_t card_index, Rect parent_rect)
    : View(parent_rect), card_index_(card_index) {
    add_children({&card_text_});
}

void ThreatCard::update_card(const DisplayDroneEntry& drone) {
    is_active_ = true;
    frequency_ = drone.frequency;
    threat_ = drone.threat;
    rssi_ = drone.rssi;
    last_seen_ = drone.last_seen;
    threat_name_ = drone.type_name;
    trend_ = MovementTrend::STATIC;

    card_text_.set(render_compact());
    card_text_.set_style(get_card_text_color());
    set_dirty();
}

void ThreatCard::clear_card() {
    is_active_ = false;
    card_text_.set("");
    set_dirty();
}

std::string ThreatCard::render_compact() const {
    if (!is_active_) return "";

    char buffer[32];
    const char* trend_symbol = (trend_ == MovementTrend::APPROACHING) ? "▲" :
                              (trend_ == MovementTrend::RECEDING) ? "▼" : "■";
    const char* threat_abbr = (threat_ == ThreatLevel::CRITICAL) ? "CRIT" :
                             (threat_ == ThreatLevel::HIGH) ? "HIGH" :
                             (threat_ == ThreatLevel::MEDIUM) ? "MED" :
                             (threat_ == ThreatLevel::LOW) ? "LOW" : "NONE";

    float freq_mhz = static_cast<float>(frequency_) / 1000000.0f;
    if (freq_mhz >= 1000) {
        freq_mhz /= 1000;
        snprintf(buffer, sizeof(buffer), "🛰️ %s │ %.1fG │ %s %s │ %ddB",
                threat_name_.c_str(), freq_mhz, trend_symbol, threat_abbr, rssi_);
    } else {
        snprintf(buffer, sizeof(buffer), "🛰️ %s │ %.0fM │ %s %s │ %ddB",
                threat_name_.c_str(), freq_mhz, trend_symbol, threat_abbr, rssi_);
    }
    return std::string(buffer);
}

Color ThreatCard::get_card_bg_color() const {
    if (!is_active_) return Color::black();
    switch (threat_) {
        case ThreatLevel::CRITICAL: return Color(64, 0, 0);
        case ThreatLevel::HIGH: return Color(64, 32, 0);
        case ThreatLevel::MEDIUM: return Color(32, 32, 0);
        case ThreatLevel::LOW: return Color(0, 32, 0);
        case ThreatLevel::NONE:
        default: return Color(0, 0, 64);
    }
}

Color ThreatCard::get_card_text_color() const {
    if (!is_active_) return Color::white();
    switch (threat_) {
        case ThreatLevel::CRITICAL: return Color::red();
        case ThreatLevel::HIGH: return Color(255, 140, 0);
        case ThreatLevel::MEDIUM: return Color::yellow();
        case ThreatLevel::LOW: return Color::green();
        case ThreatLevel::NONE:
        default: return Color::white();
    }
}

void ThreatCard::paint(Painter& painter) {
    View::paint(painter);
    if (is_active_) {
        Color bg_color = get_card_bg_color();
        painter.fill_rectangle({parent_rect_.left(), parent_rect_.top(), parent_rect_.width(), 2}, bg_color);
    }
}

ConsoleStatusBar::ConsoleStatusBar(size_t bar_index, Rect parent_rect)
    : View(parent_rect), bar_index_(bar_index) {
    add_children({&progress_text_, &alert_text_, &normal_text_});
    set_display_mode(DisplayMode::NORMAL);
}

void ConsoleStatusBar::update_scanning_progress(uint32_t progress_percent, uint32_t total_cycles, uint32_t detections) {
    set_display_mode(DisplayMode::SCANNING);

    char progress_bar[9] = "░░░░░░░░";
    uint8_t filled = (progress_percent * 8) / 100;
    for (uint8_t i = 0; i < filled; i++) {
        progress_bar[i] = '█';
    }

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s %u%% C:%u D:%u",
            progress_bar, progress_percent, total_cycles, detections);
    progress_text_.set(buffer);
    progress_text_.set_style(Theme::getInstance()->fg_blue);

    if (detections > 0) {
        set_display_mode(DisplayMode::ALERT);
        snprintf(buffer, sizeof(buffer), "⚠️ DETECTED: %u threats found!", detections);
        alert_text_.set(buffer);
        alert_text_.set_style(Theme::getInstance()->fg_red);
    }
    set_dirty();
}

void ConsoleStatusBar::update_alert_status(ThreatLevel threat, size_t total_drones, const std::string& alert_msg) {
    set_display_mode(DisplayMode::ALERT);

    const char* icons[5] = {"ℹ️", "⚠️", "🟠", "🔴", "🚨"};
    size_t icon_idx = std::min(static_cast<size_t>(threat), size_t(4));

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%s ALERT: %zu drones | %s",
            icons[icon_idx], total_drones, alert_msg.c_str());

    alert_text_.set(buffer);
    alert_text_.set_style((threat >= ThreatLevel::CRITICAL) ? Theme::getInstance()->fg_red : Theme::getInstance()->fg_yellow());
    set_dirty();
}

void ConsoleStatusBar::update_normal_status(const std::string& primary, const std::string& secondary) {
    set_display_mode(DisplayMode::NORMAL);

    char buffer[64];
    if (secondary.empty()) {
        snprintf(buffer, sizeof(buffer), "%s", primary.c_str());
    } else {
        snprintf(buffer, sizeof(buffer), "%s | %s", primary.c_str(), secondary.c_str());
    }
    normal_text_.set(buffer);
    normal_text_.set_style(Theme::getInstance()->fg_light->foreground);
    set_dirty();
}

void ConsoleStatusBar::set_display_mode(DisplayMode mode) {
    if (mode_ == mode) return;
    mode_ = mode;

    progress_text_.hidden(true);
    alert_text_.hidden(true);
    normal_text_.hidden(true);

    switch (mode) {
        case DisplayMode::SCANNING: progress_text_.hidden(false); break;
        case DisplayMode::ALERT: alert_text_.hidden(false); break;
        case DisplayMode::NORMAL: default: normal_text_.hidden(false); break;
    }
    set_dirty();
}

void ConsoleStatusBar::paint(Painter& painter) {
    View::paint(painter);
    if (mode_ == DisplayMode::ALERT) {
        painter.fill_rectangle({parent_rect_.left(), parent_rect_.top(), parent_rect_.width(), 2}, Color(32, 0, 0));
    }
}

DroneDisplayController::DroneDisplayController(NavigationView& nav)
    : nav_(nav), spectrum_gradient_{}
{
    if (!spectrum_gradient_.load_file(default_gradient_file)) {
        spectrum_gradient_.set_default();
    }
    initialize_mini_spectrum();

    message_handler_spectrum_config_ = MessageHandlerRegistration(
        Message::ID::ChannelSpectrumConfig,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const ChannelSpectrumConfigMessage*>(p);
            this->spectrum_fifo_ = message.fifo;
        });

    message_handler_frame_sync_ = MessageHandlerRegistration(
        Message::ID::DisplayFrameSync,
        [this](const Message* const p) {
            (void)p;
            if (this->spectrum_fifo_) {
                ChannelSpectrum channel_spectrum;
                while (spectrum_fifo_->out(channel_spectrum)) {
                    this->process_mini_spectrum_data(channel_spectrum);
                }
                this->render_mini_spectrum();
            }
        });
}

void DroneDisplayController::update_detection_display(const DroneScanner& scanner) {
    if (scanner.is_scanning_active()) {
        Frequency current_freq = scanner.get_current_scanning_frequency();
        if (current_freq > 0) {
            char freq_buffer[32];
            float freq_mhz = static_cast<float>(current_freq) / 1000000.0f;
            if (freq_mhz < 1000.0f) {
                snprintf(freq_buffer, sizeof(freq_buffer), "%.1f MHz", freq_mhz);
            } else {
                freq_mhz /= 1000.0f;
                snprintf(freq_buffer, sizeof(freq_buffer), "%.2f GHz", freq_mhz);
            }
            big_display_.set(freq_buffer);
        } else {
            big_display_.set("SCANNING...");
        }
    } else {
        big_display_.set("READY");
    }

    size_t total_freqs = scanner.get_database_size();
    if (total_freqs > 0 && scanner.is_scanning_active()) {
        uint32_t progress_percent = 50;
        scanning_progress_.set_value(std::min(progress_percent, (uint32_t)100));
    } else {
        scanning_progress_.set_value(0);
    }

    ThreatLevel max_threat = scanner.get_max_detected_threat();
    bool has_detections = (scanner.get_approaching_count() + scanner.get_receding_count() + scanner.get_static_count()) > 0;

    if (has_detections) {
        char summary_buffer[64];
        snprintf(summary_buffer, sizeof(summary_buffer), "THREAT: %s | ▲%zu ■%zu ▼%zu",
                get_threat_level_name(max_threat), scanner.get_approaching_count(),
                scanner.get_static_count(), scanner.get_receding_count());
        text_threat_summary_.set(summary_buffer);
        text_threat_summary_.set_style(Theme::getInstance()->fg_red);
    } else {
        text_threat_summary_.set("THREAT: NONE | All clear");
        text_threat_summary_.set_style(Theme::getInstance()->fg_green);
    }

    char status_buffer[64];
    if (scanner.is_scanning_active()) {
        std::string mode_str = scanner.is_real_mode() ? "REAL" : "DEMO";
        snprintf(status_buffer, sizeof(status_buffer), "%s - Detections: %u",
                mode_str.c_str(), scanner.get_total_detections());
    } else {
        snprintf(status_buffer, sizeof(status_buffer), "Ready - Enhanced Drone Analyzer");
    }
    text_status_info_.set(status_buffer);

    size_t loaded_freqs = scanner.get_database_size();
    char stats_buffer[64];
    if (scanner.is_scanning_active() && loaded_freqs > 0) {
        size_t current_idx = 0;
        snprintf(stats_buffer, sizeof(stats_buffer), "Freq: %zu/%zu | Cycle: %u",
                current_idx + 1, loaded_freqs, scanner.get_scan_cycles());
    } else if (loaded_freqs > 0) {
        snprintf(stats_buffer, sizeof(stats_buffer), "Loaded: %zu frequencies", loaded_freqs);
    } else {
        snprintf(stats_buffer, sizeof(stats_buffer), "No database loaded");
    }
    text_scanner_stats_.set(stats_buffer);

    if (max_threat >= ThreatLevel::HIGH) {
        big_display_.set_style(Theme::getInstance()->fg_red);
    } else if (max_threat >= ThreatLevel::MEDIUM) {
        big_display_.set_style(Theme::getInstance()->fg_yellow);
    } else if (has_detections) {
        big_display_.set_style(Theme::getInstance()->fg_orange);
    } else if (scanner.is_scanning_active()) {
        big_display_.set_style(Theme::getInstance()->fg_green);
    } else {
        big_display_.set_style(Theme::getInstance()->bg_darkest);
    }
}

void DroneDisplayController::add_detected_drone(Frequency freq, DroneType type, ThreatLevel threat, int32_t rssi) {
    systime_t now = chTimeNow();
    auto it = std::find_if(detected_drones_.begin(), detected_drones_.end(),
                          [freq](const DisplayDroneEntry& entry) {
                              return entry.frequency == freq;
                          });
    if (it != detected_drones_.end()) {
        it->rssi = rssi;
        it->threat = threat;
        it->type = type;
        it->last_seen = now;
        it->type_name = get_drone_type_name(type);
        it->display_color = get_drone_type_color(type);
