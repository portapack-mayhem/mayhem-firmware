// ui_basic_scanner.hpp
// Basic frequency scanner using database
// Step 3.2 of modular refactoring - add basic spectrum analysis functionality

#ifndef __UI_BASIC_SCANNER_HPP__
#define __UI_BASIC_SCANNER_HPP__

#include "ui_minimal_drone_analyzer.hpp"
#include "ui_drone_database.hpp"
#include <cstdint>

// FORWARD DECLARATIONS FOR COMPATIBILITY
// These will be properly included when integrating with spectrum_collector.hpp later
namespace rf {
    using Frequency = uint32_t;
}

namespace radio {
    bool set_tuning_frequency(rf::Frequency freq);
    void set_direction(int direction);
}

// SCANNING CHANNEL STRUCT
struct ScanningChannel {
    uint32_t frequency_hz;
    DroneType drone_type;
    ThreatLevel threat_level;
    int32_t rssi_threshold_db;
    uint32_t bandwidth_hz;
    int32_t current_rssi_db;
    uint32_t scan_count;
    systime_t last_scan_time;
    bool is_active;

    ScanningChannel() : frequency_hz(0), drone_type(DroneType::UNKNOWN),
                       threat_level(ThreatLevel::NONE), rssi_threshold_db(-80),
                       bandwidth_hz(1000000), current_rssi_db(-120),
                       scan_count(0), last_scan_time(0), is_active(false) {}
};

// BASIC FREQUENCY SCANNER CLASS
class BasicFrequencyScanner {
public:
    static constexpr size_t MAX_CHANNELS = 64;  // Match database size

    BasicFrequencyScanner();
    ~BasicFrequencyScanner() = default;

    // Initialize with database
    bool initialize_from_database(const DroneFrequencyDatabase& database);

    // Scanning methods
    bool start_scanning(const DroneFrequencyDatabase& database);
    bool stop_scanning();
    bool is_scanning() const { return is_scanning_; }

    // Channel access
    const ScanningChannel* get_channel(size_t index) const;
    size_t get_channel_count() const { return active_channels_; }

    // Get current scanning position
    size_t get_current_channel_index() const { return current_channel_index_; }

    // Scan single channel (main scanning function)
    bool scan_next_channel();

    // Get RSSI reading (will be replaced with real spectrum_collector later)
    int32_t get_current_rssi_reading() const;

    // Utility functions
    uint32_t get_total_scan_count() const { return total_scans_; }
    systime_t get_scan_start_time() const { return scan_start_time_; }

private:
    std::array<ScanningChannel, MAX_CHANNELS> channels_;
    size_t active_channels_;
    size_t current_channel_index_;
    bool is_scanning_;
    uint32_t total_scans_;
    systime_t scan_start_time_;
    static constexpr uint32_t SCAN_PAUSE_MS = 50;  // 50ms between channel scans

    // Initialize channel array
    void reset_channels();
    void add_channel_from_entry(const DroneFrequencyEntry& entry);

    // RSSI simulation (to be replaced with real hardware reading)
    int32_t simulate_rssi_reading(uint32_t frequency_hz, ThreatLevel expected_threat) const;
};

// BASIC DETECTOR CLASS
class BasicDroneDetector {
public:
    BasicDroneDetector();
    ~BasicDroneDetector() = default;

    // Detection methods
    bool detect_drone_signals(const BasicFrequencyScanner& scanner);

    // Get detection results
    size_t get_detection_count() const { return detection_count_; }
    const ScanningChannel* get_last_detection() const { return last_detection_; }

    // Statistics
    uint32_t get_total_detections() const { return total_detections_; }
    uint32_t get_false_positives() const { return false_positives_; }
    uint32_t get_confirmed_detections() const { return confirmed_detections_; }

private:
    size_t detection_count_;
    uint32_t total_detections_;
    uint32_t false_positives_;
    uint32_t confirmed_detections_;
    size_t current_channel_index_;
    static constexpr int32_t DETECTION_THRESHOLD_BASE_DB = -80;  // Base threshold
    static constexpr int32_t SIGNAL_VARIANCE_DB = 5;           // Noise variance

    // Detection logic
    bool check_channel_for_detection(const ScanningChannel& channel) const;
    bool validate_signal_strength(int32_t rssi, int32_t threshold) const;
    bool check_signal_consistency(const ScanningChannel& channel) const;
};

#endif // __UI_BASIC_SCANNER_HPP__
