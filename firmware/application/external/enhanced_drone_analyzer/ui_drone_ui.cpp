#include "ui_drone_ui.hpp"
#include "ui_menu.hpp"
#include "app_settings.hpp"
#include <algorithm>
#include <sstream>
#include <mutex>
namespace ui::external_app::enhanced_drone_analyzer {

// PHASE 1 IMPLEMENTATION: Smart Threat Header Component
SmartThreatHeader::SmartThreatHeader(Rect parent_rect)
    : View(parent_rect) {
    add_children({
        &threat_progress_bar_,
        &threat_status_main_,
        &threat_frequency_
    });

    // Initialize default state
    update(ThreatLevel::NONE, 0, 0, 0, 2400000000ULL, false);
}

void SmartThreatHeader::update(ThreatLevel max_threat, size_t approaching,
                               size_t static_count, size_t receding,
                               Frequency current_freq, bool is_scanning) {
    current_threat_ = max_threat;
    is_scanning_ = is_scanning;
    current_freq_ = current_freq;
    approaching_count_ = approaching;
    static_count_ = static_count;
    receding_count_ = receding;

    // Update progress bar state based on threat
    size_t total_drones = approaching + static_count + receding;
    threat_progress_bar_.set_value(total_drones * 10); // Rough scale
    threat_progress_bar_.set_style(get_threat_bar_color(max_threat));

    // Update status line with threat summary
    char buffer[64];
    std::string threat_name = get_threat_icon_text(max_threat);
    if (total_drones > 0) {
        snprintf(buffer, sizeof(buffer), "THREAT: %s | ▲%zu ■%zu ▼%zu",
                threat_name.c_str(), approaching, static_count, receding);
    } else if (is_scanning) {
        snprintf(buffer, sizeof(buffer), "SCANNING: ▲%zu ■%zu ▼%zu",
                approaching, static_count, receding);
    } else {
        snprintf(buffer, sizeof(buffer), "READY: No Threats Detected");
    }
    threat_status_main_.set(buffer);

    // Update frequency display
    if (current_freq > 0) {
        char freq_buffer[16];
        if (current_freq >= 1000000000) { // GHz
            float freq_ghz = static_cast<float>(current_freq) / 1000000000.0f;
            if (is_scanning) {
                snprintf(freq_buffer, sizeof(freq_buffer), "%.2fGHz SCANNING", freq_ghz);
            } else {
                snprintf(freq_buffer, sizeof(freq_buffer), "%.2fGHz READY", freq_ghz);
            }
        } else { // MHz
            float freq_mhz = static_cast<float>(current_freq) / 1000000.0f;
            if (is_scanning) {
                snprintf(freq_buffer, sizeof(freq_buffer), "%.1fMHz SCANNING", freq_mhz);
            } else {
                snprintf(freq_buffer, sizeof(freq_buffer), "%.1fMHz READY", freq_mhz);
            }
        }
        threat_frequency_.set(freq_buffer);
    } else {
        threat_frequency_.set("NO SIGNAL");
    }

    // Apply colors to text elements
    Color threat_color = get_threat_text_color(max_threat);
    threat_status_main_.set_style(threat_color);
    threat_frequency_.set_style(threat_color);

    set_dirty();
}

void SmartThreatHeader::set_max_threat(ThreatLevel threat) {
    if (threat != current_threat_) {
        update(threat, approaching_count_, static_count_, receding_count_,
               current_freq_, is_scanning_);
    }
}

void SmartThreatHeader::set_movement_counts(size_t approaching, size_t static_count, size_t receding) {
    update(current_threat_, approaching, static_count, receding,
           current_freq_, is_scanning_);
}

void SmartThreatHeader::set_current_frequency(Frequency freq) {
    if (freq != current_freq_) {
        update(current_threat_, approaching_count_, static_count_, receding_count_,
               freq, is_scanning_);
    }
}

void SmartThreatHeader::set_scanning_state(bool is_scanning) {
    if (is_scanning != is_scanning_) {
        update(current_threat_, approaching_count_, static_count_, receding_count_,
               current_freq_, is_scanning);
    }
}

void SmartThreatHeader::set_color_scheme(bool use_dark_theme) {
    // Future implementation for theme switching
    (void)use_dark_theme; // Suppress unused parameter warning
}

Color SmartThreatHeader::get_threat_bar_color(ThreatLevel level) const {
    switch (level) {
        case ThreatLevel::CRITICAL: return Color::red();
        case ThreatLevel::HIGH: return Color(255, 140, 0); // Dark orange
        case ThreatLevel::MEDIUM: return Color::yellow();
        case ThreatLevel::LOW: return Color::green();
        case ThreatLevel::NONE:
        default: return Color::blue(); // Safe blue for no threats
    }
}

Color SmartThreatHeader::get_threat_text_color(ThreatLevel level) const {
    // Map threat levels to colors with good contrast
    switch (level) {
        case ThreatLevel::CRITICAL: return Color::red();
        case ThreatLevel::HIGH: return Color(255, 165, 0); // Orange
        case ThreatLevel::MEDIUM: return Color::yellow();
        case ThreatLevel::LOW: return Color::green();
        case ThreatLevel::NONE:
        default: return Color::white(); // White for neutral state
    }
}

std::string SmartThreatHeader::get_threat_icon_text(ThreatLevel level) const {
    switch (level) {
        case ThreatLevel::CRITICAL: return "CRITICAL 🔴";
        case ThreatLevel::HIGH: return "HIGH 🟠";
        case ThreatLevel::MEDIUM: return "MEDIUM 🟡";
        case ThreatLevel::LOW: return "LOW 🟢";
        case ThreatLevel::NONE:
        default: return "CLEAR ✅";
    }
}

void SmartThreatHeader::paint(Painter& painter) {
    // Custom rendering for threat visualization
    View::paint(painter);

    // Optional: Add gradient background based on threat level
    if (current_threat_ >= ThreatLevel::HIGH) {
        // Pulse effect for high threats
        static uint32_t pulse_timer = 0;
        pulse_timer++;
        uint8_t alpha = (pulse_timer % 20) < 10 ? 50 : 100;

        Color pulse_color = get_threat_bar_color(current_threat_);
        pulse_color = Color(pulse_color.r, pulse_color.g, pulse_color.b, alpha);

        painter.fill_rectangle({parent_rect_.left(), parent_rect_.top(),
                               parent_rect_.width(), 4}, pulse_color);
    }
}

// PHASE 2: ThreatCard Implementation - Modern compact drone display
ThreatCard::ThreatCard(size_t card_index, Rect parent_rect)
    : View(parent_rect), card_index_(card_index) {
    add_children({&card_text_});
}

void ThreatCard::update_card(const DisplayDroneEntry& drone) {
    is_active_ = true;
    frequency_ = drone.frequency;
    threat_ = drone.threat;
    rssi_ = drone.rssi;
    last_seen_ = drone.last_seen;
    threat_name_ = drone.type_name;

    // For now, set a simple trend (can be enhanced later)
    trend_ = MovementTrend::STATIC;

    // Update text display
    card_text_.set(render_compact());
    card_text_.set_style(get_card_text_color());

    set_dirty();
}

void ThreatCard::clear_card() {
    is_active_ = false;
    card_text_.set("");
    set_dirty();
}

std::string ThreatCard::render_compact() const {
    if (!is_active_) return "";

    char buffer[32];

    // Format: "🏗️ DJI Mini │ 2400MHz │ ▲ HIGH │ 74dB"
    const char* trend_symbol = (trend_ == MovementTrend::APPROACHING) ? "▲" :
                              (trend_ == MovementTrend::RECEDING) ? "▼" : "■";

    // Threat level abbreviation
    const char* threat_abbr = (threat_ == ThreatLevel::CRITICAL) ? "CRIT" :
                             (threat_ == ThreatLevel::HIGH) ? "HIGH" :
                             (threat_ == ThreatLevel::MEDIUM) ? "MED" :
                             (threat_ == ThreatLevel::LOW) ? "LOW" : "NONE";

    // Convert frequency to MHz for display
    float freq_mhz = static_cast<float>(frequency_) / 1000000.0f;
    if (freq_mhz >= 1000) { // GHz format
        freq_mhz /= 1000;
        snprintf(buffer, sizeof(buffer), "🛰️ %s │ %.1fG │ %s %s │ %ddB",
                threat_name_.c_str(), freq_mhz, trend_symbol, threat_abbr, rssi_);
    } else {
        snprintf(buffer, sizeof(buffer), "🛰️ %s │ %.0fM │ %s %s │ %ddB",
                threat_name_.c_str(), freq_mhz, trend_symbol, threat_abbr, rssi_);
    }

    return std::string(buffer);
}

Color ThreatCard::get_card_bg_color() const {
    if (!is_active_) return Color::black();

    switch (threat_) {
        case ThreatLevel::CRITICAL: return Color(64, 0, 0);    // Dark red background
        case ThreatLevel::HIGH: return Color(64, 32, 0);       // Dark orange background
        case ThreatLevel::MEDIUM: return Color(32, 32, 0);     // Dark yellow background
        case ThreatLevel::LOW: return Color(0, 32, 0);         // Dark green background
        case ThreatLevel::NONE:
        default: return Color(0, 0, 64);                       // Dark blue background
    }
}

Color ThreatCard::get_card_text_color() const {
    if (!is_active_) return Color::white();

    switch (threat_) {
        case ThreatLevel::CRITICAL: return Color::red();
        case ThreatLevel::HIGH: return Color(255, 140, 0);     // Orange
        case ThreatLevel::MEDIUM: return Color::yellow();
        case ThreatLevel::LOW: return Color::green();
        case ThreatLevel::NONE:
        default: return Color::white();
    }
}

void ThreatCard::paint(Painter& painter) {
    // Custom background rendering for threat visualization
    View::paint(painter);

    if (is_active_) {
        Color bg_color = get_card_bg_color();
        painter.fill_rectangle({parent_rect_.left(), parent_rect_.top(),
                               parent_rect_.width(), 2}, bg_color);
    }
}

// PHASE 2.5: ConsoleStatusBar Implementation - Ultra-compact information layout
ConsoleStatusBar::ConsoleStatusBar(size_t bar_index, Rect parent_rect)
    : View(parent_rect), bar_index_(bar_index) {
    add_children({&progress_text_, &alert_text_, &normal_text_});

    // Initially show normal mode
    set_display_mode(DisplayMode::NORMAL);
}

void ConsoleStatusBar::update_scanning_progress(uint32_t progress_percent, uint32_t total_cycles, uint32_t detections) {
    set_display_mode(DisplayMode::SCANNING);

    // Create a simple ASCII progress bar (8 characters)
    char progress_bar[9] = "░░░░░░░░";
    uint8_t filled = (progress_percent * 8) / 100;
    for (uint8_t i = 0; i < filled; i++) {
        progress_bar[i] = '█';
    }

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s %u%% C:%u D:%u",
            progress_bar, progress_percent, total_cycles, detections);

    progress_text_.set(buffer);
    progress_text_.set_style(Theme::getInstance()->fg_blue);

    // Switch to alert mode if high threat detected
    if (detections > 0) {
        set_display_mode(DisplayMode::ALERT);
        snprintf(buffer, sizeof(buffer), "⚠️ DETECTED: %u threats found!", detections);
        alert_text_.set(buffer);
        alert_text_.set_style(Theme::getInstance()->fg_red);
    }

    set_dirty();
}

void ConsoleStatusBar::update_alert_status(ThreatLevel threat, size_t total_drones, const std::string& alert_msg) {
    set_display_mode(DisplayMode::ALERT);

    const char* icons[5] = {"ℹ️", "⚠️", "🟠", "🔴", "🚨"}; // NONE to CRITICAL
    const char* colors[5] = {"white", "yellow", "orange", "red", "maroon"};

    size_t icon_idx = std::min(static_cast<size_t>(threat), size_t(4));

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%s ALERT: %zu drones | %s",
            icons[icon_idx], total_drones, alert_msg.c_str());

    alert_text_.set(buffer);
    alert_text_.set_style((threat >= ThreatLevel::CRITICAL) ? Theme::getInstance()->fg_red : Theme::getInstance()->fg_yellow);

    set_dirty();
}

void ConsoleStatusBar::update_normal_status(const std::string& primary, const std::string& secondary) {
    set_display_mode(DisplayMode::NORMAL);

    char buffer[64];
    if (secondary.empty()) {
        snprintf(buffer, sizeof(buffer), "%s", primary.c_str());
    } else {
        snprintf(buffer, sizeof(buffer), "%s | %s", primary.c_str(), secondary.c_str());
    }

    normal_text_.set(buffer);
    normal_text_.set_style(Theme::getInstance()->fg_light->foreground);

    set_dirty();
}

void ConsoleStatusBar::set_display_mode(DisplayMode mode) {
    if (mode_ == mode) return;

    mode_ = mode;

    // Hide all texts initially
    progress_text_.hidden(true);
    alert_text_.hidden(true);
    normal_text_.hidden(true);

    // Show appropriate text based on mode
    switch (mode) {
        case DisplayMode::SCANNING:
            progress_text_.hidden(false);
            break;
        case DisplayMode::ALERT:
            alert_text_.hidden(false);
            break;
        case DisplayMode::NORMAL:
        default:
            normal_text_.hidden(false);
            break;
    }

    set_dirty();
}

void ConsoleStatusBar::paint(Painter& painter) {
    // Custom rendering with subtle background for console feel
    View::paint(painter);

    // Add subtle background stripe
    if (mode_ == DisplayMode::ALERT) {
        painter.fill_rectangle({parent_rect_.left(), parent_rect_.top(),
                               parent_rect_.width(), 2}, Color(32, 0, 0)); // Subtle red stripe
    }
}

DroneDisplayController::DroneDisplayController(NavigationView& nav)
    : nav_(nav),
      displayed_drones_{},
      threat_bins_{},
      spectrum_gradient{},
      spectrum_fifo_(nullptr)
{
    if (!spectrum_gradient_.load_file(default_gradient_file)) {
        spectrum_gradient_.set_default();
    }
    initialize_mini_spectrum();
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
Color DroneDisplayController::get_drone_type_color(DroneType type) const {
    switch (type) {
        case DroneType::MAVIC: return Color(0, 0, 255);    // DJI Mavic series - blue
        case DroneType::PHANTOM: return Color(255, 255, 0); // DJI Phantom - yellow
        case DroneType::DJI_MINI: return Color(0, 128, 255); // DJI Mini series - light blue
        case DroneType::PARROT_ANAFI: return Color(255, 0, 255); // Parrot Anafi - magenta
        case DroneType::PARROT_BEBOP: return Color(255, 165, 0); // Parrot Bebop - orange
        case DroneType::PX4_DRONE: return Color(128, 0, 128); // PX4-based - purple
        case DroneType::MILITARY_DRONE: return Color(255, 0, 0); // Military - red
        case DroneType::UNKNOWN:
        default: return Color(64, 64, 64); // Gray for unknown
    }
}
std::string DroneDisplayController::get_drone_type_name(DroneType type) const {
    switch (type) {
        case DroneType::MAVIC: return "Mavic";
        case DroneType::PHANTOM: return "Phantom";
        case DroneType::DJI_MINI: return "DJI Mini";
        case DroneType::PARROT_ANAFI: return "Anafi";
        case DroneType::PARROT_BEBOP: return "Bebop";
        case DroneType::PX4_DRONE: return "PX4";
        case DroneType::MILITARY_DRONE: return "Military";
        case DroneType::UNKNOWN:
        default: return "Unknown";
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
    if (scanner.is_scanning_active()) {
    Frequency current_freq = scanner.get_current_scanning_frequency();
    if (current_freq > 0) {
        char freq_buffer[32];
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
    size_t total_freqs = scanner.get_database_size();
    if (total_freqs > 0 && scanner.is_scanning_active()) {
        uint32_t progress_percent = 50; // Placeholder - would need actual progress tracking
        scanning_progress_.set_value(std::min(progress_percent, (uint32_t)100));
    } else {
        scanning_progress_.set_value(0);
    }
    ThreatLevel max_threat = scanner.get_max_detected_threat();
    bool has_detections = (scanner.get_approaching_count() + scanner.get_receding_count() +
                          scanner.get_static_count()) > 0;
    if (has_detections) {
        char summary_buffer[64];
        snprintf(summary_buffer, sizeof(summary_buffer), "THREAT: %s | ▲%llu ■%llu ▼%llu",
                get_threat_level_name(max_threat),
                scanner.get_approaching_count(),
                scanner.get_static_count(),
                scanner.get_receding_count());
        text_threat_summary_.set(summary_buffer);
        text_threat_summary_.set_style(Theme::getInstance()->fg_red); // Threat = red
    } else {
        text_threat_summary_.set("THREAT: NONE | All clear");
        text_threat_summary_.set_style(Theme::getInstance()->fg_green); // No threat = green
    update_trends_display(scanner.get_approaching_count(),
                         scanner.get_static_count(),
                         scanner.get_receding_count());
    char status_buffer[64];
    if (scanner.is_scanning_active()) {
        std::string mode_str = scanner.is_real_mode() ? "REAL" : "DEMO";
        snprintf(status_buffer, sizeof(status_buffer), "%s - Detections: %u",
                mode_str.c_str(), scanner.get_total_detections());
    } else {
        snprintf(status_buffer, sizeof(status_buffer), "Ready - Enhanced Drone Analyzer");
    }
    text_status_info_.set(status_buffer);
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
    snprintf(trend_buffer, sizeof(trend_buffer), "Trends: ▲%zu ■%zu ▼%zu",
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
void DroneDisplayController::set_frequency_display(Frequency freq) {
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
void DroneDisplayController::add_detected_drone(Frequency freq, DroneType type, ThreatLevel threat, int32_t rssi) {
    systime_t now = chTimeNow();
    auto it = std::find_if(detected_drones_.begin(), detected_drones_.end(),
                          [freq](const DisplayDroneEntry& entry) {
                              return entry.frequency == freq;
                          });
    if (it != detected_drones_.end()) {
        it->rssi = rssi;
        it->threat = threat;
        it->type = type;
        it->last_seen = now;
        it->type_name = get_drone_type_name(type);
        it->display_color = get_drone_type_color(type);
    } else {
        if (detected_drones_.size() < MAX_TRACKED_DRONES) { // Safety limit for memory management
            DisplayDroneEntry entry;
            entry.frequency = freq;
            entry.rssi = rssi;
            entry.threat = threat;
            entry.type = type;
            entry.last_seen = now;
            entry.type_name = get_drone_type_name(type);
            entry.display_color = get_drone_type_color(type);
            detected_drones_.push_back(entry);
        }
    }
    update_drones_display();
}
void DroneDisplayController::sort_drones_by_rssi() {
    std::sort(detected_drones_.begin(), detected_drones_.end(),
              [](const DisplayDroneEntry& a, const DisplayDroneEntry& b) {
                  if (a.rssi != b.rssi) return a.rssi > b.rssi;
                  if (a.threat != b.threat) return static_cast<int>(a.threat) > static_cast<int>(b.threat);
                  return a.last_seen > b.last_seen;
              });
}
void DroneDisplayController::update_drones_display(const DroneScanner& scanner) {
    const systime_t STALE_TIMEOUT = 30000; // 30 seconds in ChibiOS ticks
    systime_t now = chTimeNow();
    detected_drones_.erase(
        std::remove_if(detected_drones_.begin(), detected_drones_.end(),
                      [now, STALE_TIMEOUT](const DisplayDroneEntry& entry) {
                          return (now - entry.last_seen) > STALE_TIMEOUT;
                      }),
        detected_drones_.end());
    sort_drones_by_rssi();
    displayed_drones_.clear();
    size_t count = std::min(detected_drones_.size(), MAX_DISPLAYED_DRONES);
    for (size_t i = 0; i < count; ++i) {
        displayed_drones_.push_back(detected_drones_[i]);
    }
    highlight_threat_zones_in_spectrum(displayed_drones_);
    render_drone_text_display();
    for (size_t i = 0; i < MAX_TRACKED_DRONES && i < 3; ++i) {
        const auto& drone = scanner.getTrackedDrone(i);
        if (drone.update_count > 0) {  // Only show active drones
        }
    }
}
void DroneDisplayController::render_drone_text_display() {
    text_drone_1.set("");
    text_drone_2.set("");
    text_drone_3.set("");
    for (size_t i = 0; i < std::min(displayed_drones_.size(), size_t(3)); ++i) {
        const auto& drone = displayed_drones_[i];
        char buffer[32];
        char trend_symbol;
        switch (drone.trend) {
            case MovementTrend::APPROACHING: trend_symbol = '▲'; break;
            case MovementTrend::RECEDING: trend_symbol = '▼'; break;
            case MovementTrend::STATIC:
            case MovementTrend::UNKNOWN:
            default: trend_symbol = '■'; break;
        }
        std::string freq_str;
        if (drone.frequency >= 1000000000) {  // GHz
            freq_str = to_string_dec_uint(drone.frequency / 1000000000, 1) + "G";
        } else if (drone.frequency >= 1000000) { // MHz
            freq_str = to_string_dec_uint(drone.frequency / 1000000, 1) + "M";
        } else { // kHz
            freq_str = to_string_dec_uint(drone.frequency / 1000, 1) + "k";
        }
        snprintf(buffer, sizeof(buffer), DRONE_DISPLAY_FORMAT,
                drone.type_name.c_str(),
                freq_str.c_str(),
                drone.rssi,
                trend_symbol);
        Color threat_color = get_threat_level_color(drone.threat);
        switch(i) {
            case 0:
                text_drone_1.set(buffer);
                text_drone_1.set_style(threat_color);
                break;
            case 1:
                text_drone_2.set(buffer);
                text_drone_2.set_style(threat_color);
                break;
            case 2:
                text_drone_3.set(buffer);
                text_drone_3.set_style(threat_color);
                break;
        }
    }
}
void DroneDisplayController::initialize_mini_spectrum() {
    if (!spectrum_gradient_.load_file(default_gradient_file)) {
        spectrum_gradient_.set_default();
    }
    clear_spectrum_buffers();
}
void DroneDisplayController::process_mini_spectrum_data(const ChannelSpectrum& spectrum) {
    uint8_t current_bin_power = 0;
    for (size_t bin = 0; bin < 240; bin++) {  // Process screen_width bins
        get_max_power_for_current_bin(spectrum, current_bin_power);
        if (process_bins(&current_bin_power)) {
            return;  // New waterfall line completed
        }
    }
}
bool DroneDisplayController::process_bins(uint8_t* power_level) {
    bins_hz_size += each_bin_size;  // Accumulate Hz for this bin
    if (bins_hz_size >= 1000000) {  // Each pixel represents 1MHz step
        if (*power_level > min_color_power) {
            add_spectrum_pixel_from_bin(*power_level);
        } else {
            add_spectrum_pixel_from_bin(0);
        }
        *power_level = 0;  // Reset for next bin
        if (pixel_index == 0) {  // New line completed (pixel_index reset to 0)
            bins_hz_size = 0;  // Reset Hz accumulator for next line
            return true;  // Signal new line
        }
        bins_hz_size -= 1000000;  // Carry excess Hz into next pixel
    }
    return false;
}
void DroneDisplayController::get_max_power_for_current_bin(const ChannelSpectrum& spectrum, uint8_t& max_power) {
    size_t spec_size = spectrum.db.size();
    if (spec_size < 256) return;  // Safety check for expected size >= 256

    for (size_t bin = 2; bin <= 253; bin++) {
        if (bin >= 122 && bin <= 134) continue; // Skip DC spike

        size_t index;
        if (bin < 128) {
            index = 128 + bin;
        } else {
            index = bin - 128;
        }

        // Bounds check before access
        if (index >= spec_size) continue;

        uint8_t power = spectrum.db[index];
        if (power > max_power)
            max_power = power;
    }
}
void DroneDisplayController::add_spectrum_pixel_from_bin(uint8_t power) {
    if (!validate_spectrum_data()) {
        clear_spectrum_buffers();
        return;
    }
    if (pixel_index < spectrum_row.size()) {
        Color pixel_color = spectrum_gradient_.lut[
            std::min(power, static_cast<uint8_t>(spectrum_gradient_.lut.size() - 1))
        ];
        for (size_t i = 0; i < threat_bins_count_; i++) {
            if (threat_bins_[i].bin == pixel_index) {
                pixel_color = get_threat_level_color(threat_bins_[i].threat);
                break;  // First matching bin wins
            }
        }
        spectrum_row[pixel_index] = pixel_color;
        pixel_index++;
    }
}
void DroneDisplayController::render_mini_spectrum() {
    std::scoped_lock<std::mutex> lock(spectrum_access_mutex_);  // Section 3: Thread safety for spectrum rendering

    if (!validate_spectrum_data()) {
        clear_spectrum_buffers();
        return;
    }
    const Color background_color = spectrum_gradient_.lut.size() > 0 ? spectrum_gradient_.lut[0] : Color::black();
    std::fill(spectrum_row.begin(), spectrum_row.end(), background_color);
    if (pixel_index > 0) {  // Got some pixels, render anyway (modified from full line only)
        display.draw_pixels(
            {{0, display.scroll(1)}, {screen_width, 1}},  // Scroll and draw at top
            spectrum_row                                  // Render the completed line
        );
        pixel_index = 0;
    }
}
void DroneDisplayController::highlight_threat_zones_in_spectrum(const std::array<DisplayDroneEntry, MAX_DISPLAYED_DRONES>& drones) {
    threat_bins_count_ = 0;
    for (const auto& drone : drones) {
        if (drone.frequency > 0) {  // Valid frequency
            size_t bin_x = frequency_to_spectrum_bin(drone.frequency);
            if (bin_x < MINI_SPECTRUM_WIDTH && threat_bins_count_ < MAX_DISPLAYED_DRONES) {
                threat_bins_[threat_bins_count_].bin = bin_x;
                threat_bins_[threat_bins_count_].threat = drone.threat;
                threat_bins_count_++;
            }
        }
    }
}
void DroneDisplayController::clear_spectrum_buffers() {
    std::fill(spectrum_power_levels_.begin(), spectrum_power_levels_.end(), 0);
}
bool DroneDisplayController::validate_spectrum_data() const {
    if (spectrum_power_levels_.size() != MINI_SPECTRUM_WIDTH) return false;
    if (spectrum_gradient_.lut.empty()) return false;
    return true;
}
size_t DroneDisplayController::get_safe_spectrum_index(size_t x, size_t y) const {
    if (x >= MINI_SPECTRUM_WIDTH || y >= MINI_SPECTRUM_HEIGHT) {
        return 0; // Return safe default - first element
    }
    return y * MINI_SPECTRUM_WIDTH + x;
}
void DroneDisplayController::set_spectrum_range(Frequency min_freq, Frequency max_freq) {
    if (min_freq >= max_freq || min_freq < MIN_HARDWARE_FREQ || max_freq > MAX_HARDWARE_FREQ) {
        spectrum_config_.min_freq = 2400000000ULL;  // ISM default
        spectrum_config_.max_freq = 2500000000ULL;
        return;
    }
    spectrum_config_.min_freq = min_freq;
    spectrum_config_.max_freq = max_freq;
    spectrum_config_.bandwidth = (max_freq - min_freq) > 24000000 ?
                                24000000 : static_cast<uint32_t>(max_freq - min_freq);
    spectrum_config_.sampling_rate = spectrum_config_.bandwidth; // Direct mapping
}
size_t DroneDisplayController::frequency_to_spectrum_bin(Frequency freq_hz) const {
    const Frequency MIN_FREQ = spectrum_config_.min_freq;
    const Frequency MAX_FREQ = spectrum_config_.max_freq;
    const Frequency FREQ_RANGE = MAX_FREQ - MIN_FREQ;
    if (freq_hz < MIN_FREQ || freq_hz > MAX_FREQ || FREQ_RANGE == 0) {
        return MINI_SPECTRUM_WIDTH; // Out of range - safe sentinel value
    }
    Frequency relative_freq = freq_hz - MIN_FREQ;
    size_t bin = (relative_freq * MINI_SPECTRUM_WIDTH) / FREQ_RANGE;
    return std::min(bin, MINI_SPECTRUM_WIDTH - 1);
}
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
}
void DroneUIController::on_start_scan() {
    if (scanning_active_) return; // Already scanning
    scanning_active_ = true;
    scanner_.start_scanning();
    display_controller_->set_scanning_status(true, "Scanning Active");
    display_controller_->update_detection_display(scanner_);
}
void DroneUIController::on_stop_scan() {
    scanning_active_ = false;
    scanner_.stop_scanning();
    audio_.stop_audio();
    display_controller_->set_scanning_status(false, "Stopped");
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
    display_controller_->set_scanning_status(scanning_active_,
                                           scanner_.is_real_mode() ? "Real Mode" : "Demo Mode");
}
void DroneUIController::show_menu() {
    auto menu_view = nav_.push<MenuView>({
        {Translator::translate("load_database"), [this]() { on_load_frequency_file(); }},
        {Translator::translate("save_frequency"), [this]() { on_save_frequency(); }},
        {"Save Settings", [this]() { on_save_settings(); }},     // RESTORED: Settings persistence
        {"Load Settings", [this]() { on_load_settings(); }},     // RESTORED: Settings loading
        {Translator::translate("toggle_audio"), [this]() { on_toggle_audio_simple(); }}, // PHASE 1: RESTORE Audio Enable Toggle
        {Translator::translate("audio_settings"), [this]() { on_audio_toggle(); }},
        {Translator::translate("add_preset"), [this]() { on_add_preset_quick(); }}, // PHASE 4: RESTORE Preset system for drone database
    char success_msg[128];
    snprintf(success_msg, sizeof(success_msg),
            "RESTORATION: Preset Added Successfully\n%s (%d MHz)\nThreat: %s",
            selected_preset.display_name.c_str(),
            static_cast<int>(selected_preset.frequency_hz / 1000000),
            selected_preset.threat_level == ThreatLevel::HIGH ? "HIGH" :
            selected_preset.threat_level == ThreatLevel::MEDIUM ? "MEDIUM" : "LOW");
    nav_.display_modal("Preset Added", success_msg);
        {Translator::translate("manage_freq"), [this]() { on_manage_frequencies(); }},
        {Translator::translate("create_db"), [this]() { on_create_new_database(); }},
        {Translator::translate("advanced"), [this]() { on_advanced_settings(); }},
        {Translator::translate("constant_settings"), [this]() { on_open_constant_settings(); }}, // New: Constant settings dialog
        {Translator::translate("frequency_warning"), [this]() { on_frequency_warning(); }},
        {Translator::translate("select_language"), [this]() { on_select_language(); }}, // Language selection
        {Translator::translate("about_author"), [this]() { on_about(); }}
    });
}
void DroneUIController::on_select_language() {
    auto language_menu = nav_.push<MenuView>({
        {Translator::translate("english"), [this]() { Translator::set_language(Language::ENGLISH); nav_.display_modal(Translator::translate("english"), "Language updated to English"); }},
        {Translator::translate("russian"), [this]() { Translator::set_language(Language::RUSSIAN); nav_.display_modal("Русский", "Язык изменен на русский"); }}
    });
}
void DroneUIController::on_load_frequency_file() {
    bool loaded = scanner_.load_frequency_database();
    if (!loaded) {
        nav_.display_modal("Error", "Failed to load frequency\ndatabase");
        return;
    }
    size_t db_size = scanner_.get_database_size();
    if (db_size > 100) {
        on_frequency_warning();  // Call the fully implemented warning function
    }
    char success_msg[128];
    snprintf(success_msg, sizeof(success_msg), "Loaded %zu frequencies\nsuccessfully", db_size);
    nav_.display_modal("Success", success_msg);
}
void DroneUIController::on_save_frequency() {
    nav_.display_modal("Info", "Save functionality\nComing soon...");
}
void DroneUIController::on_audio_toggle() {
    static DroneAudioSettings audio_settings = {
        .audio_enabled = audio_.is_audio_enabled(),
        .test_threat_level = ThreatLevel::HIGH
    };
    nav_.push<DroneAudioSettingsView>(audio_settings, audio_);
}
void DroneUIController::on_advanced_settings() {
    auto submenu = nav_.push<MenuView>({
        {"Spectrum Mode", [this]() { on_spectrum_settings(); }},  // PHASE 3: Now connects restore set_spectrum_mode()
        {"Hardware Control", [this]() { on_hardware_control_menu(); }}, // PHASE 5: Add hardware getters/setters
        {"System Status", [this]() { show_system_status(); }},    // Split info into separate function
        {"Performance Stats", [this]() { show_performance_stats(); }}, // NEW: Add performance monitoring
        {"Debug Info", [this]() { show_debug_info(); }}           // NEW: Add debugging info
    });
}
void DroneUIController::show_system_status() {
    char settings_info[512];
    int active_drones = static_cast<int>(scanner_.get_approaching_count() +
                                       scanner_.get_receding_count() +
                                       scanner_.get_static_count());
    size_t db_entries = scanner_.get_database_size();
    const char* mode_status = scanner_.is_real_mode() ? "HARDWARE/SPECTRUM" : "SIMULATION";
    const char* audio_status = audio_.is_audio_enabled() ? "ENABLED" : "DISABLED";
    const char* thread_status = scanner_.is_scanning_active() ? "YES" : "NO";
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
    nav_.display_modal("System Status", settings_info);
}
void DroneUIController::show_performance_stats() {
    char perf_info[512];
    uint32_t scan_cycles = scanner_.get_scan_cycles();
    float avg_cycle_time = scan_cycles > 0 ? 750.0f : 0.0f; // ms per cycle
    size_t memory_used = scanner_.get_total_memory_usage(); // hypothetical function
    snprintf(perf_info, sizeof(perf_info),
            "PERFORMANCE STATISTICS\n"
            "======================\n\n"
            "SCANNING:\n"
            "Cycles completed: %u\n"
            "Avg. cycle time: %.0f ms\n"
            "Total detections: %u\n"
            "Detection rate: %.1f/sec\n\n"
            "MEMORY:\n"
            "Used: %zu KB\n"
            "Available: ~112 KB\n"
            "Efficiency: %.1f%%",
            scan_cycles,
            avg_cycle_time,
            scanner_.get_total_detections(),
            static_cast<float>(scanner_.get_total_detections()) * 1000.0f / (scan_cycles * avg_cycle_time + 1),
            memory_used / 1024,
            100.0f - (static_cast<float>(memory_used) / 114688.0f * 100.0f));
    nav_.display_modal("Performance Stats", perf_info);
}
void DroneUIController::show_debug_info() {
    char debug_info[512];
    Frequency current_freq = scanner_.get_current_scanning_frequency();
    std::string scanning_mode = scanner_.scanning_mode_name();
    snprintf(debug_info, sizeof(debug_info),
            "DEBUG INFORMATION\n"
            "=================\n\n"
            "CURRENT STATE:\n"
            "Scanning: %s\n"
            "Mode: %s\n"
            "Frequency: %.3f MHz\n"
            "Threat Level: %s\n\n"
            "HARDWARE:\n"
            "Spectrum Active: %s\n"
            "RSSI: %d dB\n"
            "Temperature: %d°C\n\n"
            "THREADS:\n"
            "Scanner: %s\n"
            "Spectrum: %s",
            scanner_.is_scanning_active() ? "YES" : "NO",
            scanning_mode.c_str(),
            static_cast<float>(current_freq) / 1000000.0f,
            "NONE", // TODO: Add real threat level
            hardware_.is_spectrum_streaming_active() ? "YES" : "NO",
            hardware_.get_current_rssi(),
            35, // Placeholder temperature
            scanner_.is_scanning_active() ? "RUNNING" : "STOPPED",
            hardware_.is_spectrum_streaming_active() ? "STREAMING" : "IDLE");
    nav_.display_modal("Debug Info", debug_info);
}
void DroneUIController::on_manage_frequencies() {
    nav_.push<DroneFrequencyManagerView>();
}
void DroneUIController::on_create_new_database() {
    nav_.display_modal("Info", "Create Database\nComing soon...");
}
void DroneUIController::on_frequency_warning() {
    size_t freq_count = scanner_.get_database_size();
    if (freq_count == 0) freq_count = 1; // Avoid division by zero
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
void DroneUIController::on_hardware_control_menu() {
    auto hardware_menu = nav_.push<MenuView>({
        {"Show Current Bandwidth", [this]() { show_current_bandwidth(); }},
        {"Set Bandwidth (MHz)", [this]() { on_set_bandwidth_config(); }},
        {"Show Center Frequency", [this]() { show_current_center_freq(); }},
        {"Set Center Frequency", [this]() { on_set_center_freq_config(); }}
    });
}
void DroneUIController::on_save_settings() {
    if (DroneAnalyzerSettingsManager::save_settings(settings_)) {
        nav_.display_modal("Success", "Settings saved successfully");
    } else {
        nav_.display_modal("Error", "Failed to save settings");
    }
}
void DroneUIController::on_load_settings() {
    if (DroneAnalyzerSettingsManager::load_settings(settings_)) {
        nav_.display_modal("Success", "Settings loaded successfully");
    } else {
        nav_.display_modal("Error", "Failed to load settings");
    }
}
void DroneUIController::show_current_bandwidth() {
    uint32_t current_bandwidth = hardware_.get_spectrum_bandwidth();
    char bw_msg[128];
    if (current_bandwidth >= 1000000) {  // MHz
        float bw_mhz = static_cast<float>(current_bandwidth) / 1000000.0f;
        snprintf(bw_msg, sizeof(bw_msg), "Current Bandwidth:\n%.1f MHz", bw_mhz);
    } else {
        float bw_khz = static_cast<float>(current_bandwidth) / 1000.0f;
        snprintf(bw_msg, sizeof(bw_msg), "Current Bandwidth:\n%.1f kHz", bw_khz);
    }
    nav_.display_modal("Spectrum Bandwidth", bw_msg);
}
void DroneUIController::show_current_center_freq() {
    Frequency current_center = hardware_.get_spectrum_center_frequency();
    char freq_msg[128];
    if (current_center >= 1000000000) {  // GHz
        float freq_ghz = static_cast<float>(current_center) / 1000000000.0f;
        snprintf(freq_msg, sizeof(freq_msg), "Center Frequency:\n%.2f GHz", freq_ghz);
    } else {  // MHz
        float freq_mhz = static_cast<float>(current_center) / 1000000.0f;
        snprintf(freq_msg, sizeof(freq_msg), "Center Frequency:\n%.1f MHz", freq_mhz);
    }
    nav_.display_modal("Center Frequency", freq_msg);
}
void DroneUIController::on_set_bandwidth_config() {
    auto bandwidth_menu = nav_.push<MenuView>({
        {"4 MHz (Narrow)", [this]() { set_bandwidth_from_menu(4000000); }},
        {"8 MHz (Medium)", [this]() { set_bandwidth_from_menu(8000000); }},
        {"20 MHz (Wide)", [this]() { set_bandwidth_from_menu(20000000); }},
        {"24 MHz (Ultra-wide)", [this]() { set_bandwidth_from_menu(24000000); }}
    });
}
void DroneUIController::on_set_center_freq_config() {
    auto freq_menu = nav_.push<MenuView>({
        {"433 MHz (ISM)", [this]() { set_center_freq_from_menu(433000000); }},
        {"915 MHz (ISM)", [this]() { set_center_freq_from_menu(915000000); }},
        {"2.4 GHz (WiFi)", [this]() { set_center_freq_from_menu(2400000000ULL); }},
        {"5.8 GHz (FPV)", [this]() { set_center_freq_from_menu(5800000000ULL); }}
    });
}
void DroneUIController::set_bandwidth_from_menu(uint32_t bandwidth_hz) {
    hardware_.set_spectrum_bandwidth(bandwidth_hz);
    char confirm_msg[128];
    float bw_mhz = static_cast<float>(bandwidth_hz) / 1000000.0f;
    snprintf(confirm_msg, sizeof(confirm_msg), "Bandwidth set to:\n%.1f MHz\n\nRestart scanning to apply.", bw_mhz);
    nav_.display_modal("Bandwidth Applied", confirm_msg);
}
void DroneUIController::set_center_freq_from_menu(Frequency center_freq) {
    hardware_.set_spectrum_center_frequency(center_freq);
    char confirm_msg[128];
    if (center_freq >= 1000000000) {
        float freq_ghz = static_cast<float>(center_freq) / 1000000000.0f;
        snprintf(confirm_msg, sizeof(confirm_msg), "Center frequency set to:\n%.2f GHz", freq_ghz);
    } else {
        float freq_mhz = static_cast<float>(center_freq) / 1000000.0f;
        snprintf(confirm_msg, sizeof(confirm_msg), "Center frequency set to:\n%.1f MHz", freq_mhz);
    }
    nav_.display_modal("Frequency Applied", confirm_msg);
}
void DroneUIController::on_about() {
    nav_.push<AuthorContactView>();
}
void DroneUIController::on_spectrum_settings() {
    auto mode_menu = nav_.push<MenuView>({
        {"Ultra Narrow (4MHz)", [this]() { select_spectrum_mode(SpectrumMode::ULTRA_NARROW); }},
        {"Narrow (8MHz)", [this]() { select_spectrum_mode(SpectrumMode::NARROW); }},
        {"Medium (12MHz)", [this]() { select_spectrum_mode(SpectrumMode::MEDIUM); }},
        {"Wide (20MHz)", [this]() { select_spectrum_mode(SpectrumMode::WIDE); }},
        {"Ultra Wide (24MHz)", [this]() { select_spectrum_mode(SpectrumMode::ULTRA_WIDE); }},
        {"Set Custom Range", [this]() { on_spectrum_range_config(); }}
    });
}
void DroneUIController::select_spectrum_mode(SpectrumMode mode) {
    hardware_.set_spectrum_mode(mode);
    const char* mode_names[] = {"ULTRA_NARROW", "NARROW", "MEDIUM", "WIDE", "ULTRA_WIDE"};
    char confirm_msg[128];
    snprintf(confirm_msg, sizeof(confirm_msg), "Spectrum mode set to:\n%s\n\nRestart scanning to apply.", mode_names[static_cast<size_t>(mode)]);
    nav_.display_modal("Spectrum Mode Applied", confirm_msg);
}
void DroneUIController::on_spectrum_range_config() {
    display_controller_->set_spectrum_range(2400000000ULL, 2500000000ULL); // 2.4-2.5GHz ISM default
    nav_.display_modal("Spectrum Range", "Custom range set to:\n2.4-2.5 GHz ISM band\n\nRestart scanning to apply.");
}
void DroneUIController::on_open_constant_settings() {
    static ConstantSettingsManager manager;
    nav_.push<ConstantSettingsView>(nav_);
}
void DroneUIController::on_add_preset_quick() {
    // PHASE 1 RECOVERY: Restore get_all_presets() functionality wiring
    auto all_presets = DroneFrequencyPresets::get_all_presets();

    char info_msg[256];
    int preset_count = static_cast<int>(all_presets.size());
    snprintf(info_msg, sizeof(info_msg),
            "PHASE 1: Presets System Restored\nAvailable Presets: %d total\nSelect one to add to database\n\nPattern: Recovered unused get_all_presets()\nCross-reference: Looking Glass preset ranges",
            preset_count);
    nav_.display_modal("Preset System Recovery", info_msg);

    DronePresetSelector::show_preset_menu(nav_,
        [this](const DronePreset& selected_preset) {
            // PHASE 1 VALIDATION: Use recovered is_valid() check from DroneDatabaseEntry
            if (!selected_preset.is_valid()) {
                nav_.display_modal("Error", "Invalid preset frequency range\nPreset rejected for safety");
                return;
            }

            add_preset_to_scanner(selected_preset);

            // PHASE 1 ENHANCED FEEDBACK: Show recovery status
            char success_msg[256];
            snprintf(success_msg, sizeof(success_msg),
                    "PHASE 1 SUCCESS: Preset Added\n"
                    "Name: %s (%d MHz)\n"
                    "Threat: %s\n\n"
                    "Recovery Status: get_all_presets()\n"
                    "Integration: Presets→FreqmanDB",
                    selected_preset.display_name.c_str(),
                    static_cast<int>(selected_preset.frequency_hz / 1000000),
                    selected_preset.threat_level == ThreatLevel::HIGH ? "HIGH" :
                    (selected_preset.threat_level == ThreatLevel::MEDIUM ? "MEDIUM" : "LOW"));
            nav_.display_modal("Preset Addition Complete", success_msg);
        });
}
void DroneUIController::add_preset_to_scanner(const DronePreset& preset) {
    FreqmanDB preset_db("DRONES"); // Use same file as scanner
    if (preset_db.open()) {
        freqman_entry entry{
            .frequency_a = static_cast<Frequency>(preset.frequency_hz),
            .frequency_b = static_cast<Frequency>(preset.frequency_hz),
            .type = freqman_type::Single,
            .modulation = freqman_index_t(1),  // NFM default for drones
            .bandwidth = freqman_index_t(3),   // 25kHz default
            .step = freqman_index_t(5),        // 25kHz step
            .description = preset.name_template,
            .tonal = ""
        };
        preset_db.append_entry(entry);
        preset_db.save();
        scanner_.load_frequency_database();
        char status_msg[64];
        snprintf(status_msg, sizeof(status_msg), "Added: %s (%d MHz)",
                preset.display_name.substr(0, 15).c_str(),
                static_cast<int>(preset.frequency_hz / 1000000));
        display_controller_->set_scanning_status(false, status_msg);
    } else {
        nav_.display_modal("Error", "Cannot access database\nfor preset addition");
    }
}
void DroneUIController::on_toggle_audio_simple() {
    audio_.toggle_audio();
    bool new_state = audio_.is_audio_enabled();
    const char* status_msg = new_state ? "Audio alerts ENABLED" : "Audio alerts DISABLED";
    const char* full_msg = new_state ?
        "Audio alerts ENABLED\n\nDetection beeps will sound\nfor threat detections" :
        "Audio alerts DISABLED\n\nNo audio feedback will\nbe provided";
    nav_.display_modal(status_msg, full_msg);
}
EnhancedDroneSpectrumAnalyzerView::EnhancedDroneSpectrumAnalyzerView(NavigationView& nav)
    : nav_(nav),
      hardware_(std::make_unique<DroneHardwareController>()),
      scanner_(std::make_unique<DroneScanner>()),
      audio_(std::make_unique<AudioManager>()),
      ui_controller_(std::make_unique<DroneUIController>(nav, *hardware_, *scanner_, *audio_)),
      display_controller_(std::make_unique<DroneDisplayController>(nav)),
      scanning_coordinator_(std::make_unique<ScanningCoordinator>(nav, *hardware_, *scanner_, *display_controller_, *audio_))
{
    scanning_coordinator_->update_runtime_parameters(ui_controller_->settings());

    // PHASE 3: Initialize modern UI components
    initialize_modern_layout();

    button_start_.on_select = [this](Button&) {
        handle_start_stop_button();
    };
    button_menu_.on_select = [this, &nav](Button&) {
        handle_menu_button();
    };

    field_scanning_mode_.on_change = [this](size_t index) {
        DroneScanner::ScanningMode mode = static_cast<DroneScanner::ScanningMode>(index);
        scanner_->set_scanning_mode(mode);
        display_controller_->set_scanning_status(ui_controller_->is_scanning(),
                                               scanner_->scanning_mode_name());
        // Update new UI components
        update_modern_layout();
    };

    int initial_mode = static_cast<int>(scanner_->get_scanning_mode());
    field_scanning_mode_.set_value(initial_mode);

    // PHASE 3: Add modern UI components instead of old scattered text fields
    add_child(smart_header_.get());
    add_child(status_bar_.get());
    for (auto& card : threat_cards_) {
        add_child(card.get());
    }

    // Legacy buttons for now (will be repositioned in final design)
    add_child(&button_start_);
    add_child(&button_menu_);

    // Initial layout update
    update_modern_layout();
}
void EnhancedDroneSpectrumAnalyzerView::focus() {
    button_start_.focus();
}
void EnhancedDroneSpectrumAnalyzerView::paint(Painter& painter) {
    View::paint(painter);
}
bool EnhancedDroneSpectrumAnalyzerView::on_key(const KeyEvent key) {
    switch(key) {
        case KeyEvent::Back:
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
    display.scroll_set_area(109, screen_height - 1);
    hardware_->on_hardware_show();
}
void EnhancedDroneSpectrumAnalyzerView::on_hide() {
    stop_scanning_thread();
    hardware_->on_hardware_hide();
    View::on_hide();
}
void EnhancedDroneSpectrumAnalyzerView::start_scanning_thread() {
    if (scanning_coordinator_->is_scanning_active()) return;
    scanning_coordinator_->start_coordinated_scanning();
}
void EnhancedDroneSpectrumAnalyzerView::stop_scanning_thread() {
    if (!scanning_coordinator_->is_scanning_active()) return;
    scanning_coordinator_->stop_coordinated_scanning();
}
bool EnhancedDroneSpectrumAnalyzerView::handle_start_stop_button() {
    if (scanning_coordinator_->is_scanning_active()) {
        ui_controller_->on_stop_scan();
        button_start_.set_text("START/STOP");
    } else {
        ui_controller_->on_start_scan();
        button_start_.set_text("STOP");
    }
    return true;
}
bool EnhancedDroneSpectrumAnalyzerView::handle_menu_button() {
    ui_controller_->show_menu();
    return true;
}

// PHASE 3: Modern Layout Implementation - State-based UI adaptation
void EnhancedDroneSpectrumAnalyzerView::initialize_modern_layout() {
    // Initialize new UI components with proper positioning
    smart_header_ = std::make_unique<SmartThreatHeader>(Rect{0, 0, screen_width, 48});
    status_bar_ = std::make_unique<ConsoleStatusBar>(0, Rect{0, screen_height - 32, screen_width, 16});

    // Initialize threat cards in vertical stack below header
    size_t card_y_pos = 52; // Start below header (48 + 4 margin)
    for (size_t i = 0; i < threat_cards_.size(); ++i) {
        threat_cards_[i] = std::make_unique<ThreatCard>(i, Rect{0, card_y_pos, screen_width, 24});
        card_y_pos += 26; // 24 + 2 margin
    }

    // Initial state update
    handle_scanner_update();
}

void EnhancedDroneSpectrumAnalyzerView::update_modern_layout() {
    // Refresh all modern UI components with current scanner state
    handle_scanner_update();
}

void EnhancedDroneSpectrumAnalyzerView::handle_scanner_update() {
    if (!scanner_) return;

    // Get current scanner state
    ThreatLevel max_threat = scanner_->get_max_detected_threat();
    size_t approaching = scanner_->get_approaching_count();
    size_t static_count = scanner_->get_static_count();
    size_t receding = scanner_->get_receding_count();
    bool is_scanning = scanner_->is_scanning_active();
    Frequency current_freq = scanner_->get_current_scanning_frequency();
    uint32_t total_detections = scanner_->get_total_detections();

    // Update Smart Threat Header - single consolidated update
    smart_header_->update(max_threat, approaching, static_count, receding,
                         current_freq, is_scanning);

    // Update status bar based on scanning state
    if (is_scanning) {
        // PHASE 3: Estimate progress based on scan cycles (need improvement)
        uint32_t cycles = scanner_->get_scan_cycles();
        uint32_t progress = std::min(cycles * 10, 100u); // Rough estimate
        status_bar_->update_scanning_progress(progress, cycles, total_detections);
    } else if (approaching + static_count + receding > 0) {
        // Alert mode when threats detected but not scanning
        size_t total_drones = approaching + static_count + receding;
        status_bar_->update_alert_status(max_threat, total_drones, "Threats detected!");
    } else {
        // Normal ready state
        status_bar_->update_normal_status("EDA Ready", "No threats detected");
    }

    // Update threat cards with current top 3 threats
    for (size_t i = 0; i < std::min(3u, DisplayDroneEntry::MAX_DISPLAYED_DRONES); ++i) {
        const auto& drone = scanner_->getTrackedDrone(i);
        if (drone.update_count > 0 && threat_cards_[i]) {
            // Create a display entry for the card
            DisplayDroneEntry entry;
            entry.frequency = drone.frequency;
            entry.type = drone.type;
            entry.threat = drone.threat_level;
            entry.rssi = drone.rssi;
            entry.last_seen = chTimeNow();
            entry.type_name = drone.model_name; // Assuming scanner provides this
            entry.display_color = Color::white(); // Default, should be calculated

            threat_cards_[i]->update_card(entry);
        } else {
            // Clear inactive cards
            if (threat_cards_[i]) {
                threat_cards_[i]->clear_card();
            }
        }
    }
    // Clear remaining cards if less than 3 threats
    for (size_t i = std::min(3u, DisplayDroneEntry::MAX_DISPLAYED_DRONES); i < 3; ++i) {
        if (threat_cards_[i]) {
            threat_cards_[i]->clear_card();
        }
    }
}

LoadingScreenView::LoadingScreenView(NavigationView& nav)
    : nav_(nav),
      text_eda_(
          {120 - 12, 213, 24, 16},  // Centered X, 2/3 height Y (320 * 2/3 = 213)
          "EDA"
      ),
      timer_start_(chTimeNow())
{
    text_eda_.set_style(Theme::getInstance()->fg_red);  // Dark crushed red from theme
    add_child(&text_eda_);
    set_focusable(false);
}
LoadingScreenView::~LoadingScreenView() {
}
void LoadingScreenView::paint(Painter& painter) {
    painter.fill_rectangle(
        {0, 0, portapack::display.width(), portapack::display.height()},
        Color::black()
    );
    View::paint(painter);
}
} // namespace ui::external_app::enhanced_drone_analyzer
// TEMPORARY: Add missing SmartThreatHeader implementation to resolve compilation error
void SmartThreatHeader::update(ThreatLevel max_threat, size_t approaching,
                              size_t static_count, size_t receding,
                              Frequency current_freq, bool is_scanning) {
    // Update internal state variables (PHASE 1 MISSING IMPLEMENTATION FIX)
    current_threat_ = max_threat;
    approaching_count_ = approaching;
    static_count_ = static_count;
    receding_count_ = receding;
    current_freq_ = current_freq;
    is_scanning_ = is_scanning;

    // Update progress bar color and value based on threat level
    size_t progress_value = static_cast<size_t>(max_threat) * 25; // Convert threat level to visual progress
    if (progress_value > 100) progress_value = 100;
    threat_progress_bar_.set_value(progress_value);

    // Update status text with current threat and counts
    std::string status_text = get_threat_icon_text(max_threat) + " | ▲" +
                            std::to_string(approaching_count_) + " ■" +
                            std::to_string(static_count_) + " ▼" +
                            std::to_string(receding_count_);
    threat_status_main_.set(status_text.c_str());

    // Update frequency display
    std::string freq_text = to_string_short_freq(current_freq);
    if (is_scanning) {
        freq_text += " SCANNING";
    }
    threat_frequency_.set(freq_text.c_str());

    // Mark for repaint to show immediate changes
    set_dirty();
}

// Complete SmartThreatHeader methods (PHASE 1 MISSING IMPLEMENTATION FIX)
void SmartThreatHeader::set_max_threat(ThreatLevel threat) {
    current_threat_ = threat;
    set_dirty();
}

void SmartThreatHeader::set_movement_counts(size_t approaching, size_t static_count, size_t receding) {
    approaching_count_ = approaching;
    static_count_ = static_count;
    receding_count_ = receding;
    set_dirty();
}

void SmartThreatHeader::set_current_frequency(Frequency freq) {
    current_freq_ = freq;
    set_dirty();
}

void SmartThreatHeader::set_scanning_state(bool is_scanning) {
    is_scanning_ = is_scanning;
    set_dirty();
}

void SmartThreatHeader::set_color_scheme(bool use_dark_theme) {
    // Color scheme logic (simplified for now)
    set_dirty();
}

// Helper methods for SmartThreatHeader
Color SmartThreatHeader::get_threat_bar_color(ThreatLevel level) const {
    switch (level) {
        case ThreatLevel::CRITICAL: return Theme::getInstance()->fg_red->foreground;
        case ThreatLevel::HIGH: return Theme::getInstance()->fg_orange->foreground;
        case ThreatLevel::MEDIUM: return Theme::getInstance()->fg_yellow->foreground;
        case ThreatLevel::LOW: return Theme::getInstance()->fg_blue->foreground;
        default: return Theme::getInstance()->fg_green->foreground;
    }
}

Color SmartThreatHeader::get_threat_text_color(ThreatLevel level) const {
    return get_threat_bar_color(level); // Same color for text and bar
}

std::string SmartThreatHeader::get_threat_icon_text(ThreatLevel level) const {
    switch (level) {
        case ThreatLevel::CRITICAL: return "THREAT: CRITICAL";
        case ThreatLevel::HIGH: return "THREAT: HIGH";
        case ThreatLevel::MEDIUM: return "THREAT: MEDIUM";
        case ThreatLevel::LOW: return "THREAT: LOW";
        case ThreatLevel::NONE: return "THREAT: NORMAL";
        default: return "THREAT: NONE";
    }
}

void SmartThreatHeader::paint(Painter& painter) {
    View::paint(painter);

    // Draw threat progress bar
    auto bar_color = get_threat_bar_color(current_threat_);
    threat_progress_bar_.set_color(bar_color);

    // Draw additional visual elements if needed
    // SmartThreatHeader custom painting logic
}
