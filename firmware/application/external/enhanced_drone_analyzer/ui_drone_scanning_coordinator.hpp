// ui_drone_scanning_coordinator.hpp
// ScanningCoordinator class - fixes the broken scanning architecture
// Coordinates scanner, hardware, and UI into cohesive scanning workflow

#ifndef __UI_DRONE_SCANNING_COORDINATOR_HPP__
#define __UI_DRONE_SCANNING_COORDINATOR_HPP__

#include "ui_drone_hardware.hpp"
#include "ui_drone_scanner.hpp"
#include "ui_drone_ui.hpp"
#include "ui_drone_settings.hpp"  // For DroneAnalyzerSettings

namespace ui::external_app::enhanced_drone_analyzer {

// ScanningCoordinator - fixes the broken scanning architecture
// The EDA app was broken because scanning_thread() simply slept without
// calling hardware operations. This coordinator restores the proper flow.
class ScanningCoordinator {
public:
    ScanningCoordinator(NavigationView& nav,
                       DroneHardwareController& hardware,
                       DroneScanner& scanner,
                       DroneDisplayController& display_controller);

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

    // Scanning configuration
    uint32_t scan_interval_ms_ = 750;  // Default from EDA config
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_DRONE_SCANNING_COORDINATOR_HPP__
