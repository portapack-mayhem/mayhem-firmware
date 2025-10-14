// ui_drone_spectrum_scanner.hpp
// Spectrum scanning and RSSI processing for drone detection

#ifndef __UI_DRONE_SPECTRUM_SCANNER_HPP__
#define __UI_DRONE_SPECTRUM_SCANNER_HPP__

#include "ui.hpp"
#include "ui_drone_types.hpp"  // Unified types
#include "baseband_api.hpp"
#include "freqman_db.hpp"
#include <vector>
#include <string>

struct DroneFrequencyEntry {
    rf::Frequency frequency;
    DroneType drone_type;
    ThreatLevel threat_level;
    std::string description;
};

struct SimpleDroneValidation {
    static bool validate_rssi_signal(int32_t rssi_db, ThreatLevel threat);
};

class DroneSpectrumScanner {
private:
    FreqmanDB freq_db_;
    std::vector<DroneFrequencyEntry> drone_database_;
    bool spectrum_streaming_active_;
    int32_t last_valid_rssi_;
    size_t current_db_index_;

    // Thread management
    static msg_t scanning_thread_function(void* arg);
    msg_t scanning_thread();
    Thread* scanning_thread_;
    bool scanning_active_;
    uint32_t scan_cycles_;

public:
    DroneSpectrumScanner();
    ~DroneSpectrumScanner();

    // Spectrum management
    void initialize_spectrum_collector();
    void cleanup_spectrum_collector();
    void start_spectrum_streaming();

    // Scanning operations
    void start_scanning();
    void stop_scanning();

    // RSSI processing
    int32_t get_real_rssi_from_baseband_spectrum(rf::Frequency target_frequency);
    void process_real_rssi_data_for_freq_entry(const freqman_entry& current_entry, int32_t rssi);

    // Callbacks
    void on_channel_spectrum(const ChannelSpectrum& spectrum);

    // Status
    bool is_scanning() const { return scanning_active_; }
    uint32_t get_scan_cycles() const { return scan_cycles_; }

    // Database operations
    void load_drone_database();
    const DroneFrequencyEntry* lookup_frequency(rf::Frequency freq_mhz) const;
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_SPECTRUM_SCANNER_HPP__
