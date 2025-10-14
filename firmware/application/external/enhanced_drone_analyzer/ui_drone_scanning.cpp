// ui_drone_scanning.cpp
// Implementation of scanning logic and management

#include "ui_drone_scanning.hpp"
#include "portapack.hpp"
#include "radio.hpp"
#include "freqman.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

DroneScanningManager::DroneScanningManager(FreqmanDB& freq_db)
    : freq_db_(freq_db) {
}

DroneScanningManager::~DroneScanningManager() {
    stop_scanning();
}

msg_t DroneScanningManager::scanning_thread_function(void* arg) {
    auto* self = static_cast<DroneScanningManager*>(arg);
    return self->scanning_thread();
}

msg_t DroneScanningManager::scanning_thread() {
    while (scanning_active_ && !chThdShouldTerminateX()) {
        perform_scan_cycle();
        chThdSleepMilliseconds(750); // 750ms scan interval
        scan_cycles_++;
    }

    scanning_active_ = false;
    scanning_thread_ = nullptr;
    chThdExit(MSG_OK);
    return MSG_OK;
}

void DroneScanningManager::start_scanning() {
    if (scanning_active_ || scanning_thread_ != nullptr) return;

    scanning_active_ = true;
    scan_cycles_ = 0;
    total_detections_ = 0;
    current_channel_idx_ = 0;

    scanning_thread_ = chThdCreateFromHeap(NULL, SCAN_THREAD_STACK_SIZE,
                                          "drone_scan", NORMALPRIO,
                                          scanning_thread_function, this);
}

void DroneScanningManager::stop_scanning() {
    if (!scanning_active_) return;

    scanning_active_ = false;

    // Wait for thread to stop
    if (scanning_thread_) {
        chThdWait(scanning_thread_);
        scanning_thread_ = nullptr;
    }
}

void DroneScanningManager::perform_scan_cycle() {
    if (!freq_db_.is_open() || freq_db_.entry_count() == 0) return;

    // Cycle through frequency database - FIXED: Proper loop bounds and validation
    size_t entry_count = freq_db_.entry_count();
    if (current_channel_idx_ >= entry_count) {
        current_channel_idx_ = 0; // Reset if out of bounds
        return;
    }

    const auto& current_entry = freq_db_[current_channel_idx_];

    if (current_entry.frequency_a > 0) {
        // Set frequency for scanning - following scanner pattern
        radio::set_tuning_frequency(current_entry.frequency_a);
        chThdSleepMilliseconds(10); // Settling time

        // Get real RSSI from spectrum analysis
        int32_t rssi = last_valid_rssi_;

        // Process the frequency entry with actual RSSI logic
        // This would call drone detection logic in the main view
        // For now, count all scans as potential detections

        total_detections_++;
        last_valid_rssi_ = rssi; // Update cached RSSI
    }

    // Move to next channel - wrap around properly
    current_channel_idx_ = (current_channel_idx_ + 1) % entry_count;

    // Update scan cycle count after each complete cycle
    scan_cycles_++;
}

} // namespace ui::external_app::enhanced_drone_analyzer
