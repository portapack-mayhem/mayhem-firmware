// ui_drone_scanning.hpp
// Scanning logic and management for drone detection

#ifndef __UI_DRONE_SCANNING_HPP__
#define __UI_DRONE_SCANNING_HPP__

#include "ui.hpp"
#include "freqman_db.hpp"
#include <array>

namespace ui::external_app::enhanced_drone_analyzer {

class DroneScanningManager {
private:
    // Scanning state
    FreqmanDB& freq_db_;
    static constexpr uint32_t SCAN_THREAD_STACK_SIZE = 2048;
    Thread* scanning_thread_ = nullptr;
    bool scanning_active_ = false;
    uint32_t scan_cycles_ = 0;
    uint32_t total_detections_ = 0;
    uint32_t current_channel_idx_ = 0;
    int32_t last_valid_rssi_ = -120;

    // Scanning logic
    static msg_t scanning_thread_function(void* arg);
    msg_t scanning_thread();

public:
    explicit DroneScanningManager(FreqmanDB& freq_db);
    ~DroneScanningManager();

    // Scanning operations
    void start_scanning();
    void stop_scanning();
    bool is_scanning() const { return scanning_active_; }

    // Cycle processing
    void perform_scan_cycle();

    // Statistics
    uint32_t get_scan_cycles() const { return scan_cycles_; }
    uint32_t get_total_detections() const { return total_detections_; }

    // RSSI access
    int32_t get_last_valid_rssi() const { return last_valid_rssi_; }
    void update_rssi(int32_t rssi) { last_valid_rssi_ = rssi; }
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_SCANNING_HPP__
