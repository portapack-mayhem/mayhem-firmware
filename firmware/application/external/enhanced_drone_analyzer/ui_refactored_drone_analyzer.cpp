// ui_refactored_drone_analyzer.cpp
// Implementation of refactored Enhanced Drone Analyzer - composition of components

#include "ui_refactored_drone_analyzer.hpp"
#include "portapack_shared_memory.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

RefactoredDroneSpectrumAnalyzerView::RefactoredDroneSpectrumAnalyzerView(NavigationView& nav)
    : nav_(nav)
    , lifecycle_(nav)
    , spectrum_scanner_()
    , drone_tracker_()
    , is_real_mode_(false)
    , total_detections_(0)
    , max_detected_threat_(ThreatLevel::NONE) {

    // Initialize hardware through lifecycle component
    lifecycle_.initialize_hardware();

    // Setup button handlers using proper lambda signatures
    button_start_.on_select = [this](Button&) { on_start_scan(); };
    button_stop_.on_select = [this](Button&) { on_stop_scan(); };
    button_save_freq_.on_select = [this](Button&) { on_save_frequency(); };
    button_load_file_.on_select = [this](Button&) { on_load_frequency_file(); };
    button_advanced_.on_select = [this](Button&) { on_frequency_warning(); };
    button_mode_.on_select = [this](Button&) { on_toggle_mode(); };

    // Initialize button states
    button_stop_.set_enabled(false);
    switch_to_demo_mode();
}

RefactoredDroneSpectrumAnalyzerView::~RefactoredDroneSpectrumAnalyzerView() {
    // Components will be cleaned up by their own destructors
}

void RefactoredDroneSpectrumAnalyzerView::focus() {
    button_start_.focus();
}

void RefactoredDroneSpectrumAnalyzerView::setup_ui_controls() {
    // UI controls setup could be done here if needed
}

void RefactoredDroneSpectrumAnalyzerView::setup_button_handlers() {
    // Button setup done in constructor
}

void RefactoredDroneSpectrumAnalyzerView::paint(Painter& painter) {
    View::paint(painter);
    // Minimal UI - text only interface
}

bool RefactoredDroneSpectrumAnalyzerView::on_key(const KeyEvent key) {
    switch(key) {
        case KeyEvent::Back:
            nav_.pop();
            return true;
        default:
            break;
    }
    return View::on_key(key);
}

bool RefactoredDroneSpectrumAnalyzerView::on_touch(const TouchEvent event) {
    return View::on_touch(event);
}

void RefactoredDroneSpectrumAnalyzerView::on_start_scan() {
    if (spectrum_scanner_.is_scanning()) return;

    spectrum_scanner_.start_scanning();
    button_start_.set_enabled(false);
    button_stop_.set_enabled(true);

    text_status_.set("Status: Scanning Active");
    text_scanning_info_.set("Scanning: Starting...");
}

void RefactoredDroneSpectrumAnalyzerView::on_stop_scan() {
    if (!spectrum_scanner_.is_scanning()) return;

    spectrum_scanner_.stop_scanning();
    drone_tracker_.remove_stale_drones();

    button_start_.set_enabled(true);
    button_stop_.set_enabled(false);

    text_status_.set("Status: Stopped");
    text_scanning_info_.set("Scanning: Idle");

    update_detection_display();
}

void RefactoredDroneSpectrumAnalyzerView::on_toggle_mode() {
    is_real_mode_ = !is_real_mode_;

    if (is_demo_mode()) {
        switch_to_demo_mode();
    } else {
        switch_to_real_mode();
    }
}

void RefactoredDroneSpectrumAnalyzerView::on_show() {
    View::on_show();

    if (is_real_mode_) {
        spectrum_scanner_.start_spectrum_streaming();
    }
}

void RefactoredDroneSpectrumAnalyzerView::on_hide() {
    if (spectrum_scanner_.is_scanning()) {
        on_stop_scan();
    }

    View::on_hide();
}

void RefactoredDroneSpectrumAnalyzerView::update_detection_display() {
    // Update progress bar based on scan progress
    if (spectrum_scanner_.get_scan_cycles() > 0) {
        scanning_progress_bar_.set_value(spectrum_scanner_.get_scan_cycles() % 100);
    } else {
        scanning_progress_bar_.set_value(0);
    }

    // Update threat level display
    if (total_detections_ == 0) {
        text_detection_info_.set("No detections\nMonitoring active");
    } else {
        char buffer[128];
        snprintf(buffer, sizeof(buffer),
                "Detections: %u\nScan cycles: %u",
                total_detections_, spectrum_scanner_.get_scan_cycles());
        text_detection_info_.set(buffer);
    }
}

void RefactoredDroneSpectrumAnalyzerView::update_database_display() {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Tracked drones: %zu\nTrends: ▲%lu ■%lu ▼%lu",
             drone_tracker_.get_tracked_count(),
             drone_tracker_.get_approaching_count(),
             drone_tracker_.get_static_count(),
             drone_tracker_.get_receding_count());
    text_database_info_.set(buffer);
}

void RefactoredDroneSpectrumAnalyzerView::handle_scan_error(const char* error_msg) {
    char error_buffer[64];
    snprintf(error_buffer, sizeof(error_buffer), "Error: %s", error_msg);
    text_scanning_info_.set(error_buffer);
}

void RefactoredDroneSpectrumAnalyzerView::switch_to_demo_mode() {
    button_mode_.set_text("Mode: Demo");
    text_status_.set("Status: Demo Mode");
    text_info_.set("Demonstration mode\nShows simulated detections");
}

void RefactoredDroneSpectrumAnalyzerView::switch_to_real_mode() {
    button_mode_.set_text("Mode: Real");
    text_status_.set("Status: Real Mode");
    text_info_.set("Real spectrum analysis\nScanning actual frequencies");

    nav_.display_modal("Warning", "Real mode requires\nfull integration with\nspectrum streaming.\nCurrently limited.");
}

void RefactoredDroneSpectrumAnalyzerView::on_open_settings() {
    lifecycle_.on_open_settings();
}

void RefactoredDroneSpectrumAnalyzerView::on_frequency_warning() {
    nav_.display_modal("Frequency Warning",
                      "SCANNING WARNING\n\n"
                      "Large frequency lists\nslow down scanning.\n\n"
                      "Recommendations:\n50-100 frequencies max");
}

void RefactoredDroneSpectrumAnalyzerView::on_save_frequency() {
    nav_.display_modal("Save Feature", "Frequency saving\nnot yet implemented\nin refactored version.");
}

void RefactoredDroneSpectrumAnalyzerView::on_load_frequency_file() {
    nav_.display_modal("Load Feature", "File loading\nnot yet implemented\nin refactored version.");
}

} // namespace ui::external_app::enhanced_drone_analyzer
