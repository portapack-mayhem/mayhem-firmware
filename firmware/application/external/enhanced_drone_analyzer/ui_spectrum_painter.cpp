// ui_spectrum_painter.cpp
// Spectrum painter implementation for Enhanced Drone Analyzer
// Embedded-compatible spectrum visualization

#include "ui_spectrum_painter.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

EnhancedSpectrumPainter::EnhancedSpectrumPainter()
    : active_channel_count_(0) {

    // Initialize arrays with default values
    for (auto& rssi : channel_rssi_data_) {
        rssi = -120; // No signal
    }

    for (auto& threat : channel_threat_data_) {
        threat = ThreatLevel::NONE;
    }
}

void EnhancedSpectrumPainter::initialize() {
    // Reset to clean state
    active_channel_count_ = 0;

    for (auto& rssi : channel_rssi_data_) {
        rssi = -120;
    }

    for (auto& threat : channel_threat_data_) {
        threat = ThreatLevel::NONE;
    }
}

void EnhancedSpectrumPainter::paint(Painter& painter, const Rect& rect) {
    if (active_channel_count_ == 0 || rect.width() <= 0 || rect.height() <= 0) {
        return;
    }

    // Draw spectrum bars for each channel
    for (size_t i = 0; i < active_channel_count_; ++i) {
        draw_channel_bar(painter, rect, i, active_channel_count_);
    }

    // Draw threat indicators on top
    draw_threat_indicators(painter, rect);
}

void EnhancedSpectrumPainter::update_channel_data(size_t channel_index, int32_t rssi, ThreatLevel threat) {
    if (channel_index < MAX_CHANNELS) {
        channel_rssi_data_[channel_index] = rssi;
        channel_threat_data_[channel_index] = threat;

        // Update active count if needed
        if (channel_index >= active_channel_count_) {
            active_channel_count_ = channel_index + 1;
        }
    }
}

Color EnhancedSpectrumPainter::get_rssi_color(int32_t rssi_db) const {
    // Convert RSSI to color (stronger signal = brighter)
    // Range: -120 dBm (black) to -20 dBm (bright green)

    if (rssi_db <= -120) {
        return Color::black();
    }

    if (rssi_db >= -20) {
        return Color(0, 255, 0); // Bright green for strong signals
    }

    // Linear interpolation in dB range (-120 to -20)
    int32_t db_range = 100; // -20 - (-120) = 100
    int32_t db_above_min = rssi_db + 120; // Shift to positive range

    if (db_above_min <= 0) {
        return Color::black();
    }

    // Green component: 0-255 based on signal strength
    uint8_t green_intensity = static_cast<uint8_t>((db_above_min * 255) / db_range);

    return Color(0, green_intensity, 0);
}

Color EnhancedSpectrumPainter::get_threat_color(ThreatLevel threat) const {
    switch (threat) {
        case ThreatLevel::CRITICAL: return Color::red();
        case ThreatLevel::HIGH: return Color(255, 140, 0); // Dark orange
        case ThreatLevel::MEDIUM: return Color::yellow();
        case ThreatLevel::LOW: return Color::green();
        default: return Color::black();
    }
}

void EnhancedSpectrumPainter::draw_channel_bar(Painter& painter, const Rect& rect,
                                             size_t channel_index, size_t total_channels) {
    if (channel_index >= active_channel_count_) return;

    int32_t rssi = channel_rssi_data_[channel_index];
    ThreatLevel threat = channel_threat_data_[channel_index];

    // Calculate bar position and width
    int bar_width = rect.width() / total_channels;
    if (bar_width < 1) bar_width = 1;

    int bar_x = rect.left() + (channel_index * rect.width()) / total_channels;
    int bar_height = rect.height();

    // Create bar rectangle
    Rect bar_rect(bar_x, rect.top(), bar_x + bar_width, rect.bottom());

    // Base color from RSSI
    Color base_color = get_rssi_color(rssi);

    // Override with threat color if threat present
    Color final_color = (threat != ThreatLevel::NONE) ?
                       get_threat_color(threat) : base_color;

    // Draw the bar
    painter.fill_rectangle(bar_rect, final_color);

    // Add border for better separation (subtle)
    if (bar_width >= 3) {
        Color border_color(100, 100, 100); // Dark gray
        painter.draw_line(bar_x, rect.top(), bar_x, rect.bottom(), border_color);
    }
}

void EnhancedSpectrumPainter::draw_threat_indicators(Painter& painter, const Rect& rect) {
    // Draw threat level legend at bottom
    int legend_y = rect.bottom() + 5;
    int legend_height = 10; // Height of indicator boxes

    // Define threat indicators (smaller version)
    struct ThreatIndicator {
        ThreatLevel level;
        const char* label;
        Color color;
    };

    const ThreatIndicator indicators[] = {
        {ThreatLevel::HIGH, "H", Color::red()},
        {ThreatLevel::MEDIUM, "M", Color(255, 255, 0)},
        {ThreatLevel::LOW, "L", Color::green()}
    };

    int indicator_width = 20;
    int spacing = 25;
    int start_x = rect.left();

    for (const auto& indicator : indicators) {
        // Check if any channel has this threat level
        bool has_threat = false;
        for (size_t i = 0; i < active_channel_count_; ++i) {
            if (channel_threat_data_[i] == indicator.level) {
                has_threat = true;
                break;
            }
        }

        if (has_threat || indicator.level == ThreatLevel::HIGH) { // Always show HIGH as reference
            // Draw colored box
            Rect box_rect(start_x, legend_y, start_x + indicator_width, legend_y + legend_height);
            painter.fill_rectangle(box_rect, indicator.color);

            // Draw label
            char label[4];
            sprintf(label, "%s", indicator.label);
            painter.draw_string(start_x + 2, legend_y + 1, label);

            start_x += spacing;
        }
    }
}

} // namespace ui::external_app::enhanced_drone_analyzer
