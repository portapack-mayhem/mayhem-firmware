// ui_drone_scanning_coordinator.cpp
// Implementation of ScanningCoordinator - fixes the broken scanning workflow

#include "ui_drone_scanning_coordinator.hpp"
#include "ui_drone_settings.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

ScanningCoordinator::ScanningCoordinator(NavigationView& nav,
                                       DroneHardwareController& hardware,
                                       DroneScanner& scanner,
                                       DroneDisplayController& display_controller)
    : nav_(nav),
      hardware_(hardware),
      scanner_(scanner),
      display_controller_(display_controller),
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

    // Create coordinated scanning thread
    scanning_thread_ = chThdCreateFromHeap(nullptr,
                                         SCANNING_THREAD_STACK_SIZE,
                                         "coord_scan", // Descriptive name
                                         NORMALPRIO,
                                         scanning_thread_function,
                                         this);

    if (!scanning_thread_) {
        scanning_active_ = false;
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
