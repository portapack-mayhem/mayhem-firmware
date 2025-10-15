// ui_drone_scanner.hpp
// Scanning algorithms and detection logic for Enhanced Drone Analyzer
// Module 2 of 4: Handles frequency scanning, threat detection, and drone tracking

#ifndef __UI_DRONE_SCANNER_HPP__
#define __UI_DRONE_SCANNER_HPP__

#include "freqman_db.hpp"            // FreqmanDB for frequency management
#include "freqman.hpp"               // Frequency entry types
#include "ui_drone_types.hpp"        // DroneType, ThreatLevel, MovementTrend
#include "ui_drone_hardware.hpp"     // Hardware controller for scanning

#include <vector>
#include <array>

namespace ui::external_app::enhanced_drone_analyzer {

class DroneScanner {
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
    const freqman_entry* get_current_frequency_entry() const;

    // Scanning parameters and progress
    void set_scanning_parameters(uint32_t scan_cycles, uint32_t total_detections);
    void reset_scan_cycles() { scan_cycles_ = 0; }

    // Real vs Demo mode handling
    bool is_real_mode() const { return is_real_mode_; }
    void switch_to_real_mode();
    void switch_to_demo_mode();
    bool is_demo_mode() const { return !is_real_mode_; }

    // Main scanning cycle logic
    void perform_scan_cycle(DroneHardwareController& hardware);

    // Detection processing
    void process_rssi_detection(const freqman_entry& entry, int32_t rssi);

    // Drone tracking system
    void update_tracked_drone(DroneType type, rf::Frequency frequency, int32_t rssi, ThreatLevel threat_level);
    void remove_stale_drones();

    // Statistics and counters
    uint32_t get_scan_cycles() const { return scan_cycles_; }
    uint32_t get_total_detections() const { return total_detections_; }
    size_t get_approaching_count() const { return approaching_count_; }
    size_t get_receding_count() const { return receding_count_; }
    size_t get_static_count() const { return static_count_; }
    ThreatLevel get_max_detected_threat() const { return max_detected_threat_; }

    // Current scanning state
    rf::Frequency get_current_scanning_frequency() const;
    rf::Frequency get_last_scanned_frequency() const { return last_scanned_frequency_; }

    // Error handling
    void handle_scan_error(const char* error_msg);

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

    // Mode handling
    bool is_real_mode_ = true;

    // Drone tracking system (embedded-compatible fixed arrays)
    static constexpr size_t MAX_TRACKED_DRONES = 8;
    std::array<TrackedDrone, MAX_TRACKED_DRONES> tracked_drones_;
    size_t tracked_drones_count_ = 0;

    // Movement trend counters
    size_t approaching_count_ = 0;
    size_t receding_count_ = 0;
    size_t static_count_ = 0;

    // Threat detection state
    ThreatLevel max_detected_threat_ = ThreatLevel::NONE;
    int32_t last_valid_rssi_ = -120;

    // Private scanning methods
    void initialize_database_and_scanner();
    void cleanup_database_and_scanner();
    void scan_init_from_loaded_frequencies();

    // Embedded trend counting for Portapack constraints
    void update_tracking_counts();
    void update_trends_compact_display();

    // Frequency validation
    bool validate_detection_simple(int32_t rssi_db, ThreatLevel threat);

    // Private frequency access helpers
    rf::Frequency get_current_radio_frequency() const;

    // Prevent copying
    DroneScanner(const DroneScanner&) = delete;
    DroneScanner& operator=(const DroneScanner&) = delete;
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_SCANNER_HPP__
