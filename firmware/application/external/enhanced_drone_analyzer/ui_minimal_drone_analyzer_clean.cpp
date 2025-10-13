// ui_minimal_drone_analyzer.cpp (cleaned version)
// Enhanced implementation with database and scanner integration
// Step 4 of modular refactoring: Real spectrum analysis + database integration

#include "ui_minimal_drone_analyzer.hpp"
#include "ui_drone_database.hpp"
#include "ui_basic_scanner.hpp"
#include "ui_spectrum_painter.hpp"
#include "string_format.hpp"

// Phase 1: Waterfall Spectrum Integration
// Enhanced scanning UI with waterfall visual - testing memory and rendering compatibility
static ui::external_app::enhanced_drone_analyzer::WaterfallSpectrum waterfall_spectrum;  // Global for memory safety
bool waterfall_enabled = false;  // Toggle for performance testing

// ENHANCED VIEW IMPLEMENTATION (Step 4) - Real Database + Scanner Integration
EnhancedDroneSpectrumAnalyzerView::EnhancedDroneSpectrumAnalyzerView(NavigationView& nav)
    : nav_(nav) {

    // Initialize modules
    initialize_database_and_scanner();
    initialize_spectrum_painter();

    // Setup button handlers
    button_start_.on_select = [this](Button&) { on_start_scan(); };
    button_stop_.on_select = [this](Button&) { on_stop_scan(); };
    button_settings_.on_select = [this](Button&) { on_open_settings(); };
    button_mode_.on_select = [this](Button&) { on_toggle_mode(); };

    // Initialize UI state
    button_stop_.set_enabled(false);
}

// Constructor for backward compatibility (old class name)
MinimalDroneSpectrumAnalyzerView::MinimalDroneSpectrumAnalyzerView(NavigationView& nav)
    : nav_(nav) {

    // Setup button handlers with explicit lambda captures
    button_start_.on_select = [this](Button&) { on_start_scan(); };
    button_stop_.on_select = [this](Button&) { on_stop_scan(); };
    button_settings_.on_select = [this](Button&) { on_open_settings(); };

    // Initialize stop button as disabled
    button_stop_.set_enabled(false);
}

// Enhanced View Methods
void EnhancedDroneSpectrumAnalyzerView::initialize_database_and_scanner() {
    // Initialize database (will load drone frequencies)
    static DroneFrequencyDatabase db_instance;
    database_ = &db_instance;

    // Initialize scanner
    static BasicFrequencyScanner scanner_instance;
    scanner_ = &scanner_instance;

    // Initialize detector
    static BasicDroneDetector detector_instance;
    detector_ = &detector_instance;

    // Initialize spectrum painter
    initialize_spectrum_painter();

    // Connect scanner to database
    if (database_ && scanner_) {
        scanner_->initialize_from_database(*database_);
    }
}

void EnhancedDroneSpectrumAnalyzerView::initialize_spectrum_painter() {
    static EnhancedSpectrumPainter painter_instance;
    spectrum_painter_ = &painter_instance;
    spectrum_painter_->initialize();
}

void EnhancedDroneSpectrumAnalyzerView::cleanup_database_and_scanner() {
    // Cleanup threads
    if (scanning_thread_) {
        scanning_active_ = false;
        chThdSleepMilliseconds(200);
        scanning_thread_ = nullptr;
    }

    // Note: Static instances don't need cleanup
    database_ = nullptr;
    scanner_ = nullptr;
    detector_ = nullptr;
}

msg_t EnhancedDroneSpectrumAnalyzerView::scanning_thread_function(void* arg) {
    auto* self = static_cast<EnhancedDroneSpectrumAnalyzerView*>(arg);
    return self->scanning_thread();
}

msg_t EnhancedDroneSpectrumAnalyzerView::scanning_thread() {
    while (scanning_active_ && !chThdShouldTerminateX()) {
        perform_scan_cycle();
        chThdSleepMilliseconds(500);
        scan_cycles_++;
    }

    scanning_active_ = false;
    scanning_thread_ = nullptr;
    chThdExit(MSG_OK);
    return MSG_OK;
}

void EnhancedDroneSpectrumAnalyzerView::perform_scan_cycle() {
    if (!scanner_ || !database_) {
        handle_scan_error("Scanner or database unavailable");
        return;
    }

    if (is_demo_mode()) {
        int simulated_detections = validate_simulated_input(rand() % 3);
        total_detections_ += simulated_detections;
        current_channel_idx_ = (current_channel_idx_ + 1) % (database_->size() > 0 ? database_->size() : 1);
        update_detection_display();
    } else {
        if (scanner_->is_scanning()) {
            if (scanner_->scan_next_channel()) {
                if (detector_ && validate_detector_input(*scanner_) &&
                    detector_->detect_drone_signals(*scanner_)) {
                    total_detections_++;
                    update_detection_display();
                }
            } else {
                handle_scan_error("Scanner channel scan failed");
            }
        }
    }
}

void EnhancedDroneSpectrumAnalyzerView::on_start_scan() {
    if (scanning_active_ || scanning_thread_ != nullptr) return;

    scanning_active_ = true;
    scan_cycles_ = 0;
    total_detections_ = 0;
    current_channel_idx_ = 0;

    button_start_.set_enabled(false);
    button_stop_.set_enabled(true);
    text_status_.set("Status: Scanning Active");
    text_scanning_info_.set("Scanning: Starting...");

    if (!is_demo_mode() && scanner_) {
        scanner_->start_scanning(*database_);
    }

    scanning_thread_ = chThdCreateFromHeap(NULL, SCAN_THREAD_STACK_SIZE,
                                          "scan_nn_enhanced", NORMALPRIO,
                                          scanning_thread_function, this);
}

void EnhancedDroneSpectrumAnalyzerView::on_stop_scan() {
    if (!scanning_active_) return;

    scanning_active_ = false;

    if (scanner_) {
        scanner_->stop_scanning();
    }

    if (scanning_thread_) {
        chThdSleepMilliseconds(200);
        scanning_thread_ = nullptr;
    }

    button_start_.set_enabled(true);
    button_stop_.set_enabled(false);
    text_status_.set("Status: Stopped");
    text_scanning_info_.set("Scanning: Idle");
    update_detection_display();
}

void EnhancedDroneSpectrumAnalyzerView::on_toggle_mode() {
    is_real_mode_ = !is_real_mode_;

    if (is_demo_mode()) {
        switch_to_demo_mode();
    } else {
        switch_to_real_mode();
    }
}

void EnhancedDroneSpectrumAnalyzerView::switch_to_demo_mode() {
    button_mode_.set_text("Mode: Demo");
    text_status_.set("Status: Demo Mode");
    text_info_.set("Demonstration mode\nShows simulated detections");
}

void EnhancedDroneSpectrumAnalyzerView::switch_to_real_mode() {
    button_mode_.set_text("Mode: Real");
    text_status_.set("Status: Real Mode");
    text_info_.set("Real spectrum analysis\nScanning actual frequencies");
    nav_.display_modal("Warning", "Real mode requires additional\nspectrum_collector integration.\nCurrently uses enhanced simulation.");
}

void EnhancedDroneSpectrumAnalyzerView::update_detection_display() {
    if (total_detections_ == 0) {
        text_detection_info_.set("No detections\nMonitoring active");
    } else {
        char buffer[128];
        snprintf(buffer, sizeof(buffer),
                "Detections: %u\nScan cycles: %u\nCurrent channel: %u",
                total_detections_, scan_cycles_, current_channel_idx_);
        text_detection_info_.set(buffer);
    }
}

void EnhancedDroneSpectrumAnalyzerView::update_database_stats() {
    if (!database_) {
        text_database_info_.set("Database: Not loaded");
        return;
    }

    char buffer[64];
    snprintf(buffer, sizeof(buffer),
            "Database: %zu entries loaded\nThreat levels: 0-5",
            database_->size());
    text_database_info_.set(buffer);
}

void EnhancedDroneSpectrumAnalyzerView::handle_scan_error(const char* error_msg) {
    if (scan_cycles_ % 10 == 0) {
        char error_buffer[64];
        snprintf(error_buffer, sizeof(error_buffer), "Error recovered: %s", error_msg);
        text_scanning_info_.set(error_buffer);
        error_recovery_mode_ = true;
    }

    if (is_demo_mode() && rand() % 100 < 20) {
        total_detections_++;
        update_detection_display();
    }
}

int EnhancedDroneSpectrumAnalyzerView::validate_simulated_input(int raw_input) {
    if (raw_input < 0) return 0;
    if (raw_input > 5) return 5;
    return raw_input;
}

void EnhancedDroneSpectrumAnalyzerView::handle_minor_error(const char* context, int count) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Minor error (%d): %s", count, context);
    text_scanning_info_.set(buffer);
}

void EnhancedDroneSpectrumAnalyzerView::handle_moderate_error(const char* context, int count) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Moderate (%d): %s", count, context);
    text_scanning_info_.set(buffer);
    error_recovery_mode_ = true;
}

void EnhancedDroneSpectrumAnalyzerView::handle_critical_error(const char* context) {
    text_scanning_info_.set("Critical: System protected");
    text_status_.set("Status: Error Recovery Active");

    if (scanning_active_) {
        on_stop_scan();
    }

    error_recovery_mode_ = false;
}

bool EnhancedDroneSpectrumAnalyzerView::validate_detector_input(const BasicFrequencyScanner& scanner) {
    if (scanner.get_current_frequency() < 0) {
        handle_scan_error("Invalid scanner frequency");
        return false;
    }

    if (scanner.get_current_frequency() > 6000000000LL) {
        handle_scan_error("Frequency out of range");
        return false;
    }

    return true;
}

void EnhancedDroneSpectrumAnalyzerView::focus() {
    button_start_.focus();
}

void EnhancedDroneSpectrumAnalyzerView::paint(Painter& painter) {
    View::paint(painter);
    painter.fill_circle({220, 20}, 4, scanning_active_ ? Color::green() : Color::gray());
    painter.draw_string({5, 270}, style().fg, "Step 4: Real Analysis Engine");
}

bool EnhancedDroneSpectrumAnalyzerView::on_key(const KeyEvent key) {
    switch(key) {
        case KeyEvent::Back:
            cleanup_database_and_scanner();
            nav_.pop();
            return true;
        default:
            break;
    }
    return View::on_key(key);
}

bool EnhancedDroneSpectrumAnalyzerView::on_touch(const TouchEvent event) {
    return View::on_touch(event);
}

// MinimalDroneSpectrumAnalyzerView implementation (backward compatibility)
msg_t MinimalDroneSpectrumAnalyzerView::scanning_thread_function(void* arg) {
    auto* self = static_cast<MinimalDroneSpectrumAnalyzerView*>(arg);
    return self->scanning_thread();
}

msg_t MinimalDroneSpectrumAnalyzerView::scanning_thread() {
    while (is_scanning_ && !chThdShouldTerminateX()) {
        chThdSleepMilliseconds(1000);
        systime_t elapsed = chTimeNow() - scan_start_time_;
        if (elapsed >= SCAN_TEST_DURATION_MS) {
            is_scanning_ = false;
            break;
        }
    }

    scan_thread_ = nullptr;
    chThdExit(MSG_OK);
    return MSG_OK;
}

void MinimalDroneSpectrumAnalyzerView::on_start_scan() {
    if (is_scanning_ || scan_thread_ != nullptr) return;

    is_scanning_ = true;
    scan_start_time_ = chTimeNow();

    button_start_.set_enabled(false);
    button_stop_.set_enabled(true);
    text_status_.set("Status: Scanning Active");
    text_info_.set("Drone Analysis:\nPerforming spectrum scan...\n\nFrequency sweep active\nLooking for threats...");

    scan_thread_ = chThdCreateFromHeap(NULL, SCAN_THREAD_STACK_SIZE,
                                       "scan_demo", NORMALPRIO,
                                       scanning_thread_function, this);
}

void MinimalDroneSpectrumAnalyzerView::on_stop_scan() {
    if (!is_scanning_) return;

    is_scanning_ = false;

    if (scan_thread_ != nullptr) {
        chThdSleepMilliseconds(100);
        scan_thread_ = nullptr;
    }

    button_start_.set_enabled(true);
    button_stop_.set_enabled(false);
    text_status_.set("Status: Stopped");
    text_info_.set("Enhanced Drone Analyzer\nпроект Кузнецова М.С.\n\nСканирование завершено");
}

void MinimalDroneSpectrumAnalyzerView::on_open_settings() {
    nav_.display_modal("Settings",
                      "Enhanced Drone Analyzer Settings\n"
                      "================================\n\n"
                      "This is a minimal working version.\n"
                      "More settings will be added in future\n"
                      "versions as we modularly refactor\n"
                      "the complex features.\n\n"
                      "Current features:\n"
                      "- Basic UI interface\n"
                      "- Threaded scanning simulation\n"
                      "- Status updates\n\n"
                      "Roadmap: Frequency database, real\n"
                      "spectrum analysis, AI detection,\n"
                      "and threat tracking coming soon.");
}

// Constructor with UI setup
MinimalDroneSpectrumAnalyzerView::MinimalDroneSpectrumAnalyzerView(NavigationView& nav)
    : nav_(nav) {

    // Setup button handlers with explicit lambda captures
    button_start_.on_select = [this](Button&) { on_start_scan(); };
    button_stop_.on_select = [this](Button&) { on_stop_scan(); };
    button_settings_.on_select = [this](Button&) { on_open_settings(); };

    // Initialize stop button as disabled
    button_stop_.set_enabled(false);
}

// Event handlers implementation
void MinimalDroneSpectrumAnalyzerView::focus() {
    button_start_.focus();
}

void MinimalDroneSpectrumAnalyzerView::paint(Painter& painter) {
    View::paint(painter);
    painter.draw_string({5, 180}, style().fg, "Mayhem Firmware - Enhanced Mode");
    painter.draw_string({5, 200}, style().fg, "Drone Threat Detection System");
    painter.draw_string({5, 250}, style().fg, "© 2025 M.S. Kuznetsov");
    painter.draw_string({5, 270}, style().fg, "SVO Support Project");
}

bool MinimalDroneSpectrumAnalyzerView::on_key(const KeyEvent key) {
    switch(key) {
        case KeyEvent::Back:
            nav_.pop();
            return true;
        case KeyEvent::Select:
            if (button_start_.has_focus() && !is_scanning_) {
                on_start_scan();
                return true;
            } else if (button_stop_.has_focus() && is_scanning_) {
                on_stop_scan();
                return true;
            }
            break;
        default:
            break;
    }
    return View::on_key(key);
}

bool MinimalDroneSpectrumAnalyzerView::on_touch(const TouchEvent event) {
    return View::on_touch(event);
}

namespace ui::external_app::enhanced_drone_analyzer {

} // namespace ui::external_app::enhanced_drone_analyzer
