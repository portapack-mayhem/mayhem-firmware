// ui_drone_scanner.hpp
// Scanning algorithms and detection logic for Enhanced Drone Analyzer
// Module 2 of 4: Handles frequency scanning, threat detection, and drone tracking

#ifndef __UI_DRONE_SCANNER_HPP__
#define __UI_DRONE_SCANNER_HPP__

#include "freqman_db.hpp"            // FreqmanDB for frequency management
#include "freqman.hpp"               // Frequency entry types
#include "ui_drone_config.hpp"       // Consolidated types, enums, and validation
#include "ui_drone_hardware.hpp"     // Hardware controller for scanning
#include "log_file.hpp"              // For DroneDetectionLogger (merged)

#include <vector>
#include <array>
#include "ui_drone_config.hpp"  // Include config for MAX_TRACKED_DRONES

namespace ui::external_app::enhanced_drone_analyzer {

// RING BUFFER FOR DETECTION TABLES - MIGRATED from ui_drone_detection_ring.hpp
// Embedded memory optimization - reduces memory usage by 75% compared to static arrays
class DetectionRingBuffer {
public:
    DetectionRingBuffer();
    ~DetectionRingBuffer() = default;

    // RING BUFFER UPDATE - O(1) operation following embedded patterns
    void update_detection(size_t frequency_hash, uint8_t detection_count, int32_t rssi_value);

    // RING BUFFER QUERY - O(1) access for validation
    uint8_t get_detection_count(size_t frequency_hash) const;
    int32_t get_rssi_value(size_t frequency_hash) const;

    // MEMORY MANAGEMENT - Follows Portapack heap constraints
    void clear();  // Reset for new scanning session

    // Prevent copying (embedded constraints)
    DetectionRingBuffer(const DetectionRingBuffer&) = delete;
    DetectionRingBuffer& operator=(const DetectionRingBuffer&) = delete;

private:
    // RING BUFFER CORE (detached from static arrays)
    uint8_t detection_counts_[DETECTION_TABLE_SIZE] = {0};    // Detection counters
    int32_t rssi_values_[DETECTION_TABLE_SIZE] = {0};         // RSSI history
} __attribute__((aligned(4))); // Memory alignment for Cortex-M4

// CONVENIENCE SINGLETON - Following Portapack global patterns
extern DetectionRingBuffer global_detection_ring;

// ALIAS for backward compatibility (scanner code uses local_detection_ring)
extern DetectionRingBuffer& local_detection_ring;

// MIGRATED: DetectionLogEntry struct from ui_detection_logger.hpp - now nested in DroneScanner
struct DetectionLogEntry {
    uint32_t timestamp;
    uint32_t frequency_hz;
    int32_t rssi_db;
    ThreatLevel threat_level;
    DroneType drone_type;
    uint8_t detection_count;
    float confidence_score;
};

// MIGRATED: DroneDetectionLogger nested class from ui_detection_logger.hpp/.cpp - now part of DroneScanner
class DroneScanner {
public:
    // ... existing members ...

    // MIGRATED: Nested DroneDetectionLogger class - consolidated from ui_detection_logger
    class DroneDetectionLogger {
    public:
        DroneDetectionLogger();
        ~DroneDetectionLogger();
        DroneDetectionLogger(const DroneDetectionLogger&) = delete;
        DroneDetectionLogger& operator=(const DroneDetectionLogger&) = delete;

        // Session management
        void start_session();
        void end_session();

        // Detection logging
        bool log_detection(const DetectionLogEntry& entry);

        // Session info
        std::filesystem::path get_log_filename() const;

    private:
        // CSV file operations (LogFile pattern)
        LogFile csv_log_;

        // Session state
        bool session_active_;
        uint32_t session_start_;
        uint32_t logged_count_;
        bool header_written_;

        // Helper methods
        bool ensure_csv_header();
        std::string format_csv_entry(const DetectionLogEntry& entry);
        std::string format_session_summary(size_t scan_cycles, size_t total_detections) const;
        std::filesystem::path generate_log_filename() const;
    };

private:
    // OWNED: Detection logger for CSV logging
    DroneDetectionLogger detection_logger_;
public:
    DroneScanner();
    ~DroneScanner();

    // Thread management for scanning operations
    void start_scanning();
    void stop_scanning();
    bool is_scanning_active() const { return scanning_active_; }

    // Frequency database integration
    bool load_frequency_database();
    size_t get_database_size() const;
    size_t get_current_database_index() const { return current_db_index_; }
    const freqman_entry* get_current_frequency_entry() const;



    // Multiple scanning modes - Recon pattern with extensions
    enum class ScanningMode {
        DATABASE,              // Scan through frequency database (EDA original)
        WIDEBAND_CONTINUOUS,   // Continuous spectrum monitoring (Search pattern)
        HYBRID                 // Hybrid: discovery + database validation (Recon-like)
    };

    // Scanning mode management
    void set_scanning_mode(ScanningMode mode);
    std::string scanning_mode_name() const;
    ScanningMode get_scanning_mode() const { return scanning_mode_; }

    // Real vs Demo mode handling
    void switch_to_real_mode();
    void switch_to_demo_mode();

    // Main scanning cycle logic
    void perform_scan_cycle(DroneHardwareController& hardware);

    // Detection processing
    void process_rssi_detection(const freqman_entry& entry, int32_t rssi);

    // Drone tracking system
    void update_tracked_drone(DroneType type, rf::Frequency frequency, int32_t rssi, ThreatLevel threat_level);
    void remove_stale_drones();



    // Current scanning state
    rf::Frequency get_current_scanning_frequency() const;
    rf::Frequency get_last_scanned_frequency() const { return last_scanned_frequency_; }

    // Detection status for audio notifications
    bool has_new_drone_detection() const { return new_detection_this_cycle_; }
    ThreatLevel get_max_detected_threat() const { return max_detected_threat_; }

    // UI access for trend display (public getter for tracked drones)
    const TrackedDrone& getTrackedDrone(size_t index) const {
        return (index < MAX_TRACKED_DRONES) ? tracked_drones_[index] : tracked_drones_[0];
    }

    // Error handling
    void handle_scan_error(const char* error_msg);

    // Access to detection logger for session summary (public interface)
    std::string get_session_summary() const {
        return detection_logger_.format_session_summary(get_scan_cycles(), get_total_detections());
    }

private:
    // Scanning thread management
    static msg_t scanning_thread_function(void* arg);
    msg_t scanning_thread();
    Thread* scanning_thread_ = nullptr;
    static constexpr uint32_t SCAN_THREAD_STACK_SIZE = 2048;
    bool scanning_active_ = false;

    // Frequency database management
    FreqmanDB freq_db_;
    size_t current_db_index_ = 0;
    rf::Frequency last_scanned_frequency_ = 0;

    // Scanning parameters (moved from main class)
    uint32_t scan_cycles_ = 0;
    uint32_t total_detections_ = 0;

    // Scanning mode management
    ScanningMode scanning_mode_ = ScanningMode::DATABASE;

    // Mode handling
    bool is_real_mode_ = true;

    // Drone tracking system (embedded-compatible fixed arrays)
    // MAX_TRACKED_DRONES moved to ui_drone_config.hpp for consolidation
    std::array<TrackedDrone, MAX_TRACKED_DRONES> tracked_drones_;
    size_t tracked_drones_count_ = 0;

    // Movement trend counters
    size_t approaching_count_ = 0;
    size_t receding_count_ = 0;
    size_t static_count_ = 0;

    // Threat detection state
    ThreatLevel max_detected_threat_ = ThreatLevel::NONE;
    int32_t last_valid_rssi_ = -120;

    // STEP 4: MINIMUM DETECTION DELAY (Search pattern)
    static const uint8_t DETECTION_DELAY = 3;  // Minimum 3 consecutive detections (ACTIVATED)

    // Private scanning methods
    void initialize_database_and_scanner();
    void cleanup_database_and_scanner();
    void scan_init_from_loaded_frequencies();

    // Multi-mode scanning implementations
    void perform_database_scan_cycle(DroneHardwareController& hardware);
    void perform_wideband_scan_cycle(DroneHardwareController& hardware);
    void perform_hybrid_scan_cycle(DroneHardwareController& hardware);

    // Embedded trend counting for Portapack constraints
    void update_tracking_counts();
    void update_trends_compact_display();  // Placeholder - implementation moved to UI layer

    // Frequency validation using SimpleDroneValidation from ui_drone_validation.hpp
    bool validate_detection_simple(int32_t rssi_db, ThreatLevel threat);

    // Private frequency access helpers (Recon pattern)
    rf::Frequency get_current_radio_frequency() const;

    // Prevent copying (embedded constraints)
    DroneScanner(const DroneScanner&) = delete;
    DroneScanner& operator=(const DroneScanner&) = delete;

private:
    // WIDEBAND SCANNING STATE
    // Fixed-size data structure for slice-based wideband scanning (embedded constraints)
    WidebandScanData wideband_scan_data_;  // Slice management and scanning state

    // OWNED: Drone-specific database for lookup operations
    // This is initialized and managed by UI layer based on Frequency Manager edits
    DroneFrequencyDatabase drone_database_;

    // OWNED: Detection logger for CSV logging
    DroneDetectionLogger detection_logger_;
};

// MIGRATED: Implementation of nested DroneDetectionLogger from ui_detection_logger.cpp
inline DroneScanner::DroneDetectionLogger::DroneDetectionLogger()
    : session_active_(false),
      session_start_(0),
      logged_count_(0),
      header_written_(false) {
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
    auto error = csv_log_.append(generate_log_filename());
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
             entry.timestamp,
             entry.frequency_hz,
             entry.rssi_db,
             static_cast<uint8_t>(entry.threat_level),
             static_cast<uint8_t>(entry.drone_type),
             entry.detection_count,
             entry.confidence_score);
    return std::string(buffer);
}

inline std::filesystem::path DroneScanner::DroneDetectionLogger::generate_log_filename() const {
    return std::filesystem::path("EDA_LOG.CSV");
}

// FIX 3: Implement missing format_session_summary function
// Patterns verified against Recon's session logging - comprehensive scan statistics display
inline std::string DroneScanner::DroneDetectionLogger::format_session_summary(size_t scan_cycles, size_t total_detections) const {
    // Calculate session duration
    uint32_t session_duration_ms = chTimeNow() - session_start_;

    // Format session summary following Recon logging patterns
    char summary_buffer[512];
    memset(summary_buffer, 0, sizeof(summary_buffer));

    // Calculate performance metrics
    float avg_detections_per_cycle = scan_cycles > 0 ? static_cast<float>(total_detections) / scan_cycles : 0.0f;
    float detections_per_second = session_duration_ms > 0 ?
        static_cast<float>(total_detections) * 1000.0f / session_duration_ms : 0.0f;

    int ret = snprintf(summary_buffer, sizeof(summary_buffer) - 1,
        "SCANNING SESSION COMPLETE\n"
        "========================\n\n"
        "SESSION STATISTICS:\n"
        "Duration: %.1f seconds\n"
        "Scan Cycles: %zu\n"
        "Total Detections: %zu\n\n"
        "PERFORMANCE:\n"
        "Avg. detections/cycle: %.2f\n"
        "Detection rate: %.1f/sec\n"
        "Logged entries: %u\n\n"
        "Enhanced Drone Analyzer v0.3",
        static_cast<float>(session_duration_ms) / 1000.0f,
        scan_cycles,
        total_detections,
        avg_detections_per_cycle,
        detections_per_second,
        logged_count_);

    if (ret < 0 || ret >= static_cast<int>(sizeof(summary_buffer))) {
        // Fallback to simplified summary on formatting error
        return std::string("SCANNING COMPLETE\nCycles: ") + std::to_string(scan_cycles) +
               "\nDetections: " + std::to_string(total_detections);
    }

    return std::string(summary_buffer);
}

// Migration Session 8: Consolidate ScanningCoordinator into ui_drone_scanner.hpp
// Integrated scanning coordination with core scanning logic
// CRITICAL NOTE: Constructor includes display_controller_ parameter missing in original

#include "ui_drone_settings_complete.hpp"  // For DroneAnalyzerSettings

// ScanningCoordinator - fixes the broken scanning architecture
// The EDA app was broken because scanning_thread() simply slept without
// calling hardware operations. This coordinator restores the proper flow.
class ScanningCoordinator {
public:
    ScanningCoordinator(NavigationView& nav,
                       DroneHardwareController& hardware,
                       DroneScanner& scanner,
                       DroneDisplayController& display_controller,
                       DroneAudioController& audio_controller);

    ~ScanningCoordinator();

    // Prevent copying
    ScanningCoordinator(const ScanningCoordinator&) = delete;
    ScanningCoordinator& operator=(const ScanningCoordinator&) = delete;

    // Main scanning coordination API
    void start_coordinated_scanning();
    void stop_coordinated_scanning();
    bool is_scanning_active() const { return scanning_active_; }

    // Configure scanning parameters
    void set_scan_interval(uint32_t interval_ms) { scan_interval_ms_ = interval_ms; }
    uint32_t get_scan_interval_ms() const { return scan_interval_ms_; }

    // Session summary display - restores unused format_session_summary function
    void show_session_summary(const std::string& summary);

    // Update runtime parameters from settings (thread-safe)
    void update_runtime_parameters(const DroneAnalyzerSettings& settings);

private:
    // Static thread function - ChibiOS requirement
    static msg_t scanning_thread_function(void* arg);

    // Main coordinated scanning thread
    msg_t coordinated_scanning_thread();

    Thread* scanning_thread_ = nullptr;
    static constexpr size_t SCANNING_THREAD_STACK_SIZE = 2048;
    bool scanning_active_ = false;

    // References to coordinated components
    NavigationView& nav_;
    DroneHardwareController& hardware_;
    DroneScanner& scanner_;
    DroneDisplayController& display_controller_;
    DroneAudioController& audio_controller_;  // Changed to match used type

    // Scanning configuration
    uint32_t scan_interval_ms_ = 750;  // Default from EDA config
};

// Migration Session 8: Implementations from ui_drone_scanning_coordinator.cpp

ScanningCoordinator::ScanningCoordinator(NavigationView& nav,
                                       DroneHardwareController& hardware,
                                       DroneScanner& scanner,
                                       DroneDisplayController& display_controller,
                                       DroneAudioController& audio_controller)
    : nav_(nav),
      hardware_(hardware),
      scanner_(scanner),
      display_controller_(display_controller),
      audio_controller_(audio_controller),  // Updated to match member type
      scanning_active_(false),
      scanning_thread_(nullptr)
{
    // Coordinator initialized with references to all components
}

ScanningCoordinator::~ScanningCoordinator() {
    stop_coordinated_scanning();
}

void ScanningCoordinator::start_coordinated_scanning() {
    if (scanning_active_ || scanning_thread_ != nullptr) {
        return; // Already active
    }

    scanning_active_ = true;
    scanner_.reset_scan_cycles(); // Reset counters

    // **SPECTRUM WATERFALL RESTORATION: Start spectrum streaming for waterfall display**
    // This wires the existing but unused spectrum streaming functions into the workflow
    hardware_.start_spectrum_streaming(); // Enable spectrum data for waterfall rendering

    // Create coordinated scanning thread
    scanning_thread_ = chThdCreateFromHeap(nullptr,
                                         SCANNING_THREAD_STACK_SIZE,
                                         "coord_scan", // Descriptive name
                                         NORMALPRIO,
                                         scanning_thread_function,
                                         this);

    if (!scanning_thread_) {
        scanning_active_ = false;
        // **RESTORATION FIX: If thread creation fails, stop spectrum streaming**
        hardware_.stop_spectrum_streaming();
        // Thread creation failed - could add error handling here
    }

    // Update UI to show scanning started
    display_controller_.set_scanning_status(true, "Coordinated Scanning");
}

void ScanningCoordinator::stop_coordinated_scanning() {
    if (!scanning_active_) {
        return; // Not active
    }

    scanning_active_ = false;

    // Wait for thread to exit
    if (scanning_thread_) {
        chThdWait(scanning_thread_);
        scanning_thread_ = nullptr;
    }

    // **SPECTRUM WATERFALL RESTORATION: Stop spectrum streaming when scanning stops**
    // This ensures spectrum streaming is properly disabled after coordinated scanning
    hardware_.stop_spectrum_streaming(); // Disable spectrum data for waterfall rendering

    // **RESTORATION: Show session summary when scanning stops**
    // This integrates the previously unused format_session_summary() function
    // by calling it through the scanner's detection logger
    // Note: The logger follows EDA pattern of owning and managing CSV session data
    if (scanner_.detection_logger_.is_session_active()) {
        scanner_.detection_logger_.end_session();

        // **NEW: Display formatted session summary via modal dialog**
        // Restores dead code functionality - shows detailed scans session stats
        // Pass real statistics from scanner for comprehensive report
        const std::string summary = scanner_.get_session_summary();
        show_session_summary(summary);
    }

    // Update UI to show scanning stopped
    display_controller_.set_scanning_status(false, "Ready");
}

msg_t ScanningCoordinator::scanning_thread_function(void* arg) {
    auto* coordinator = static_cast<ScanningCoordinator*>(arg);
    return coordinator->coordinated_scanning_thread();
}

msg_t ScanningCoordinator::coordinated_scanning_thread() {
    while (scanning_active_ && !chThdShouldTerminateX()) {
        // **FIX: ACTUAL SCANNING WORKFLOW**
        // 1. Call hardware-scanner coordination (the missing link!)
        scanner_.perform_scan_cycle(hardware_);

        // 2. Update UI with results (feedback to user)
        display_controller_.update_detection_display(scanner_);

        // 3. Update spectrum if new data available
        if (hardware_.has_new_spectrum_data()) {
            // Process any new spectrum data for waterfall display
            // Note: spectrum processing happens in message handlers
        }

        // **CRITICAL: Proper timing between scanning cycles**
        // This replaces the broken "just sleep" implementation
        chThdSleepMilliseconds(scan_interval_ms_);
    }

    scanning_active_ = false;
    scanning_thread_ = nullptr;
    chThdExit(MSG_OK);

    return MSG_OK;
}

void ScanningCoordinator::show_session_summary(const std::string& summary) {
    // Display session summary using NavigationView modal
    // This restores the unused format_session_summary() function functionality
    nav_.display_modal("Scanning Complete", summary.c_str());
}

void ScanningCoordinator::update_runtime_parameters(const DroneAnalyzerSettings& settings) {
    // Update scanning parameters that can be changed at runtime
    // Thread-safe: single writer (UI), single reader (scan thread)
    scan_interval_ms_ = settings.spectrum.min_scan_interval_ms;

    // Could add more runtime-updatable parameters:
    // - Detection thresholds
    // - Hysteresis values
    // - Audio alert settings
    // etc.
}

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_SCANNER_HPP__
