// ui_enhanced_spectrum_painter.cpp
// Simple spectrum painter implementation for PortaPack Mayhem

#include "ui_enhanced_spectrum_painter.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

EnhancedSpectrumPainter::EnhancedSpectrumPainter() :
    channel_count_(0) {
    channel_rssi_data_.fill(-120);
    channel_threat_data_.fill(ThreatLevel::NONE);
}

void EnhancedSpectrumPainter::initialize() {
    channel_count_ = 0;
    channel_rssi_data_.fill(-120);
    channel_threat_data_.fill(ThreatLevel::NONE);
}

void EnhancedSpectrumPainter::paint(Painter& painter, const Rect& rect) {
    if (channel_count_ == 0) return;

    // Draw simple spectrum bars
    int bar_width = rect.width() / channel_count_;
    int max_height = rect.height();

    for (size_t i = 0; i < channel_count_; ++i) {
        int32_t rssi = channel_rssi_data_[i];
        ThreatLevel threat = channel_threat_data_[i];

        // Normalize RSSI to height (assuming RSSI range -120 to 0 dBm)
        int height = ((rssi + 120) * max_height) / 120;
        height = std::max(1, std::min(height, max_height));

        // Get color based on threat level
        Color color = get_color_for_threat(threat);

        // Draw bar
        int x = rect.left() + (i * bar_width);
        int y = rect.bottom() - height;
        painter.fill_rectangle({x, y, bar_width - 1, height}, color);
    }
}

void EnhancedSpectrumPainter::update_channel_data(size_t channel_index, int32_t rssi, ThreatLevel threat) {
    if (channel_index < channel_rssi_data_.size()) {
        channel_rssi_data_[channel_index] = rssi;
        channel_threat_data_[channel_index] = threat;
        channel_count_ = std::max(channel_count_, channel_index + 1);
    }
}

Color EnhancedSpectrumPainter::get_color_for_threat(ThreatLevel threat) const {
    switch (threat) {
        case ThreatLevel::CRITICAL: return Color::red();
        case ThreatLevel::HIGH: return Color::orange();
        case ThreatLevel::MEDIUM: return Color::yellow();
        case ThreatLevel::LOW: return Color::green();
        case ThreatLevel::NONE:
        default: return Color::blue();
    }
}

Color EnhancedSpectrumPainter::get_color_for_rssi(int32_t rssi) const {
    // Simple color gradient based on RSSI strength
    if (rssi > -60) return Color::red();
    if (rssi > -80) return Color::orange();
    if (rssi > -100) return Color::yellow();
    return Color::green();
}

} // namespace ui::external_app::enhanced_drone_analyzer
