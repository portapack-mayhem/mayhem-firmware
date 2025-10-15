// ui_drone_ui.cpp
// UI components implementation for Enhanced Drone Analyzer

#include "ui_drone_ui.hpp"
#include "ui_menu.hpp"
#include "app_settings.hpp"

#include <algorithm>
#include <sstream>

namespace ui::external_app::enhanced_drone_analyzer {

// DroneDisplayController implementation
DroneDisplayController::DroneDisplayController(NavigationView& nav)
    : nav_(nav)
{
    // UI components initialized in constructor initializer list
}

Color DroneDisplayController::get_threat_level_color(ThreatLevel level) const {
    switch (level) {
        case ThreatLevel::CRITICAL: return Color::red();
        case ThreatLevel::HIGH: return Color(255, 140, 0); // Dark orange - military threat
        case ThreatLevel::MEDIUM: return Color::yellow();
        case ThreatLevel::LOW: return Color::green();
        case ThreatLevel::NONE:
        default: return Color::white(); // No threat
    }
}

const char* DroneDisplayController::get_threat_level_name(ThreatLevel level) const {
    switch (level) {
        case ThreatLevel::CRITICAL: return "CRITICAL";
        case ThreatLevel::HIGH: return "HIGH";
        case ThreatLevel::MEDIUM: return "MEDIUM";
        case ThreatLevel::LOW: return "LOW";
        case ThreatLevel::NONE:
        default: return "NONE";
    }
}

void DroneDisplayController::update_detection_display(const DroneScanner& scanner) {
    // UPDATE BIG DISPLAY WITH CURRENT STATUS
    if (scanner.is_scanning_active()) {
        rf::Frequency current_freq = scanner.get_current_scanning_frequency();
        char freq_buffer[32];

        if (current_freq > 0) {
            float freq_mhz = static_cast<float>(current_freq) / 1000000.0f;
            if (freq_mhz < 1000.0f) {
                snprintf(freq_buffer, sizeof(freq_buffer), "%.1f MHz", freq_mhz);
            } else {
                freq_mhz /= 1000.0f; // Convert to GHz
                snprintf(freq_buffer, sizeof(freq_buffer), "%.2f GHz", freq_mhz);
            }
            big_display_.set(freq_buffer);
        } else {
            big_display_.set("SCANNING...");
        }
    } else {
        big_display_.set("READY");
    }

    // FIXED: PROGRESS BAR - Use database size for progress tracking
    size_t total_freqs = scanner.get_database_size();
    if (total_freqs > 0 && scanner.is_scanning_active()) {
        // Simple progress indication (scanner doesn't track exact progress yet)
        uint32_t progress_percent = 50; // Placeholder - would need actual progress tracking
        scanning_progress_.set_value(std::min(progress_percent, (uint32_t)100));
    } else {
        scanning_progress_.set_value(0);
    }

    // UPDATE COMPACT STATUS LINES - essential info only
    ThreatLevel max_threat = scanner.get_max_detected_threat();
    bool has_detections = (scanner.get_approaching_count() + scanner.get_receding_count() +
                          scanner.get_static_count()) > 0;

    // COMPACT THREAT SUMMARY LINE
    if (has_detections) {
        char summary_buffer[64];
        snprintf(summary_buffer, sizeof(summary_buffer), "THREAT: %s | ▲%lu ■%lu ▼%lu",
                get_threat_level_name(max_threat),
                scanner.get_approaching_count(),
                scanner.get_static_count(),
                scanner.get_receding_count());
        text_threat_summary_.set(summary_buffer);
        text_threat_summary_.set_style(Theme::getInstance()->fg_red); // Threat = red
    } else {
        text_threat_summary_.set("THREAT: NONE | All clear");
        text_threat_summary_.set_style(Theme::getInstance()->fg_green); // No threat = green
    }

    // STATUS INFO LINE
    char status_buffer[64];
    if (scanner.is_scanning_active()) {
        std::string mode_str = scanner.is_real_mode() ? "REAL" : "DEMO";
        snprintf(status_buffer, sizeof(status_buffer), "%s - Detections: %u",
                mode_str.c_str(), scanner.get_total_detections());
    } else {
        snprintf(status_buffer, sizeof(status_buffer), "Ready - Enhanced Drone Analyzer");
    }
    text_status_info_.set(status_buffer);

    // SCANNER STATS LINE
    size_t loaded_freqs = scanner.get_database_size();
    char stats_buffer[64];
    if (scanner.is_scanning_active() && loaded_freqs > 0) {
        size_t current_idx = 0; // Placeholder - need to add current index tracking
        snprintf(stats_buffer, sizeof(stats_buffer), "Freq: %zu/%zu | Cycle: %u",
                current_idx + 1, loaded_freqs, scanner.get_scan_cycles());
    } else if (loaded_freqs > 0) {
        snprintf(stats_buffer, sizeof(stats_buffer), "Loaded: %zu frequencies", loaded_freqs);
    } else {
        snprintf(stats_buffer, sizeof(stats_buffer), "No database loaded");
    }
    text_scanner_stats_.set(stats_buffer);

    // UPDATE BIG DISPLAY COLOR BASED ON THREAT
    if (max_threat >= ThreatLevel::HIGH) {
        big_display_.set_style(Theme::getInstance()->fg_red);      // Critical threat
    } else if (max_threat >= ThreatLevel::MEDIUM) {
        big_display_.set_style(Theme::getInstance()->fg_yellow);   // Medium threat
    } else if (has_detections) {
        big_display_.set_style(Theme::getInstance()->fg_orange);   // Low threat
    } else if (scanner.is_scanning_active()) {
        big_display_.set_style(Theme::getInstance()->fg_green);    // Scanning but no threat
    } else {
        big_display_.set_style(Theme::getInstance()->bg_darkest);  // Not scanning
    }
}

void DroneDisplayController::update_trends_display(size_t approaching,
                                                   size_t static_count,
                                                   size_t receding) {
    char trend_buffer[64];
    snprintf(trend_buffer, sizeof(trend_buffer), "Trends: ▲%lu ■%lu ▼%lu",
             approaching, static_count, receding);
    text_trends_compact_.set(trend_buffer);
}

void DroneDisplayController::set_scanning_status(bool active, const std::string& message) {
    if (active) {
        text_status_.set(std::string("Status: ") + message);
        text_status_.set_style(Theme::getInstance()->fg_green);
    } else {
        text_status_.set("Status: Ready");
        text_status_.set_style(Theme::getInstance()->fg_light->foreground);
    }
}

void DroneDisplayController::set_frequency_display(rf::Frequency freq) {
    if (freq > 0) {
        char freq_buffer[32];
        float freq_mhz = static_cast<float>(freq) / 1000000.0f;
        if (freq_mhz < 1000.0f) {
            snprintf(freq_buffer, sizeof(freq_buffer), "%.3f MHz", freq_mhz);
        } else {
            freq_mhz /= 1000.0f; // Convert to GHz
            snprintf(freq_buffer, sizeof(freq_buffer), "%.2f GHz", freq_mhz);
        }
        big_display_.set(freq_buffer);
    } else {
        big_display_.set("READY");
    }
}

// DroneUIController implementation
DroneUIController::DroneUIController(NavigationView& nav,
                                   DroneHardwareController& hardware,
                                   DroneScanner& scanner,
                                   DroneAudioController& audio)
    : nav_(nav),
      hardware_(hardware),
      scanner_(scanner),
      audio_(audio),
      scanning_active_(false),
      display_controller_(std::make_unique<DroneDisplayController>(nav))
{
    // UI controller initialized
}

void DroneUIController::on_start_scan() {
    if (scanning_active_) return; // Already scanning

    scanning_active_ = true;
    scanner_.start_scanning();

    display_controller_->set_scanning_status(true, "Scanning Active");

    // Update UI components
    display_controller_->update_detection_display(scanner_);
}

void DroneUIController::on_stop_scan() {
    if (!scanning_active_) return; // Not scanning

    scanning_active_ = false;
    scanner_.stop_scanning();

    display_controller_->set_scanning_status(false, "Stopped");

    // Update UI components
    display_controller_->update_detection_display(scanner_);
}

void DroneUIController::on_toggle_mode() {
    if (scanner_.is_real_mode()) {
        scanner_.switch_to_demo_mode();
        if (hardware_.is_spectrum_streaming_active()) {
            hardware_.stop_spectrum_streaming();
        }
    } else {
        scanner_.switch_to_real_mode();
        if (!hardware_.is_spectrum_streaming_active()) {
            hardware_.start_spectrum_streaming();
        }
    }

    // Update status display
    display_controller_->set_scanning_status(scanning_active_,
                                           scanner_.is_real_mode() ? "Real Mode" : "Demo Mode");
}

void DroneUIController::show_menu() {
    auto menu_view = nav_.push<MenuView>({
        {"Load Database", [this]() { on_load_frequency_file(); }},
        {"Save Frequency", [this]() { on_save_frequency(); }},
        {"Audio Settings", [this]() { on_audio_toggle(); }},
        {"Manage Frequencies", [this]() { on_manage_frequencies(); }},
        {"Create Database", [this]() { on_create_new_database(); }},
        {"Advanced", [this]() { on_advanced_settings(); }},
        {"Frequency Warning", [this]() { on_frequency_warning(); }}
    });
}

void DroneUIController::on_load_frequency_file() {
    // Placeholder - implement file loading UI
    nav_.display_modal("Info", "Load functionality\nComing soon...");
}

void DroneUIController::on_save_frequency() {
    // Placeholder - implement save frequency UI
    nav_.display_modal("Info", "Save functionality\nComing soon...");
}

void DroneUIController::on_audio_toggle() {
    // Toggle audio state
    audio_.toggle_audio();

    std::string status = audio_.is_audio_enabled() ? "Audio: ENABLED" : "Audio: DISABLED";
    nav_.display_modal("Audio Settings", status.c_str());
}

void DroneUIController::on_advanced_settings() {
    char settings_info[512];

    int active_drones = static_cast<int>(scanner_.get_approaching_count() +
                                       scanner_.get_receding_count() +
                                       scanner_.get_static_count());

    // Database status
    size_t db_entries = scanner_.get_database_size();

    // Spectrum/Sim mode
    const char* mode_status = scanner_.is_real_mode() ? "HARDWARE/SPECTRUM" : "SIMULATION";

    // Audio alert status
    const char* audio_status = audio_.is_audio_enabled() ? "ENABLED" : "DISABLED";

    // Thread status (placeholder)
    const char* thread_status = scanner_.is_scanning_active() ? "YES" : "NO";

    // Spectrum streaming status
    const char* spectrum_status = hardware_.is_spectrum_streaming_active() ? "YES" : "NO";

    snprintf(settings_info, sizeof(settings_info),
            "ENHANCED DRONE ANALYZER v0.3 MODULAR\n"
            "=====================================\n\n"
            "SYSTEM STATUS:\n"
            "Mode: %s\n"
            "Thread Active: %s\n"
            "Spectrum Streaming: %s\n\n"
            "DATABASE:\n"
            "Entries loaded: %zu\n"
            "Active tracking: %d/8\n"
            "Current threat: %s\n\n"
            "AUDIO ALERTS:\n"
            "Status: %s\n"
            "Detection beeps: ENABLED\n"
            "SOS signals: READY",
            mode_status,
            thread_status,
            spectrum_status,
            db_entries,
            active_drones,
            "NONE",  // Placeholder for max threat
            audio_status);

    nav_.display_modal("Advanced Settings", settings_info);
}

void DroneUIController::on_manage_frequencies() {
    // Placeholder - frequency management UI
    nav_.display_modal("Info", "Frequency Manager\nComing soon...");
}

void DroneUIController::on_create_new_database() {
    // Placeholder - create database UI
    nav_.display_modal("Info", "Create Database\nComing soon...");
}

void DroneUIController::on_frequency_warning() {
    size_t freq_count = scanner_.get_database_size();
    if (freq_count == 0) freq_count = 1; // Avoid division by zero

    // Scan cycle time: 750ms per frequency
    float total_seconds = (freq_count * 750.0f) / 1000.0f;
    float total_minutes = total_seconds / 60.0f;

    char warning_text[512];
    if (total_minutes >= 1.0f) {
        snprintf(warning_text, sizeof(warning_text),
                "SCANNING WARNING\n\n"
                "Loaded frequencies: %zu\n"
                "Est. cycle time: %.1f min\n\n"
                "Large frequency lists slow\ndown scanning significantly.\n\n"
                "Recommendations:\n50-100 frequencies max",
                freq_count, total_minutes);
    } else {
        snprintf(warning_text, sizeof(warning_text),
                "SCANNING WARNING\n\n"
                "Loaded frequencies: %zu\n"
                "Est. cycle time: %.1f sec\n\n"
                "Large frequency lists slow\ndown scanning significantly.\n\n"
                "Recommendations:\n50-100 frequencies max",
                freq_count, total_seconds);
    }

    nav_.display_modal("Frequency Scan Warning", warning_text);
}

// EnhancedDroneSpectrumAnalyzerView implementation
EnhancedDroneSpectrumAnalyzerView::EnhancedDroneSpectrumAnalyzerView(NavigationView& nav)
    : nav_(nav)
{
    // Initialize controllers
    hardware_ = std::make_unique<DroneHardwareController>();
    scanner_ = std::make_unique<DroneScanner>();
    audio_ = std::make_unique<DroneAudioController>();
    ui_controller_ = std::make_unique<DroneUIController>(nav, *hardware_, *scanner_, *audio_);

    // Setup button callbacks
    button_start_.on_select = [this](Button&) {
        handle_start_stop_button();
    };

    button_menu_.on_select = [this, &nav](Button&) {
        handle_menu_button();
    };

    // Add child views
    add_child(&button_start_);
    add_child(&button_menu_);

    // Focus initial button
    focus();
}

void EnhancedDroneSpectrumAnalyzerView::focus() {
    button_start_.focus();
}

void EnhancedDroneSpectrumAnalyzerView::paint(Painter& painter) {
    View::paint(painter);

    // V0 STYLE - MINIMAL TEXT INTERFACE ONLY
    // NO graphics, NO spectrum display, NO waterfall - clean text status
    // painter.fill_circle({220, 20}, 4, scanning_active_ ? Color::green() : Color::gray());
}

bool EnhancedDroneSpectrumAnalyzerView::on_key(const KeyEvent key) {
    switch(key) {
        case KeyEvent::Back:
            // Cleanup resources before navigation
            stop_scanning_thread();
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

void EnhancedDroneSpectrumAnalyzerView::on_show() {
    View::on_show();

    // FOLLOWING LOOKING GLASS PATTERN: Hardware setup in on_show()
    hardware_->on_hardware_show();

    // 4. Setup display like other spectrum apps
    display.scroll_set_area(109, screen_height - 1);
}

void EnhancedDroneSpectrumAnalyzerView::on_hide() {
    // Stop any active scanning
    stop_scanning_thread();

    // FOLLOWING LOOKING GLASS PATTERN: Cleanup in on_hide()
    hardware_->on_hardware_hide();

    View::on_hide();
}

void EnhancedDroneSpectrumAnalyzerView::start_scanning_thread() {
    if (scanner_->is_scanning_active()) return;

    // Create scanning thread
    scanner_->start_scanning();

    // TODO: Implement actual scanning coordination between hardware and scanner
    // For now, this is a simplified version
}

void EnhancedDroneSpectrumAnalyzerView::stop_scanning_thread() {
    if (!scanner_->is_scanning_active()) return;

    scanner_->stop_scanning();
}

bool EnhancedDroneSpectrumAnalyzerView::handle_start_stop_button() {
    if (ui_controller_->is_scanning()) {
        // Stop scanning
        ui_controller_->on_stop_scan();
        button_start_.set_text("START/STOP");
    } else {
        // Start scanning
        ui_controller_->on_start_scan();
        button_start_.set_text("STOP");
    }
    return true;
}

bool EnhancedDroneSpectrumAnalyzerView::handle_menu_button() {
    ui_controller_->show_menu();
    return true;
}

} // namespace ui::external_app::enhanced_drone_analyzer
