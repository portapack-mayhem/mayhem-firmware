// ui_enhanced_spectrum_painter.hpp
// Enhanced Spectrum Painter adapted for PortaPack Mayhem firmware

#ifndef __UI_ENHANCED_SPECTRUM_PAINTER_HPP__
#define __UI_ENHANCED_SPECTRUM_PAINTER_HPP__

#include "ui_painter.hpp"
#include "color.hpp"
#include "ui_enhanced_frequency_database.hpp"
#include <array>
#include <cstdint>

namespace ui::external_app::enhanced_drone_analyzer {

// Simplified spectrum painter for embedded constraints
class EnhancedSpectrumPainter {
public:
    EnhancedSpectrumPainter();

    // Basic functionality for embedded system
    void initialize();
    void paint(Painter& painter, const Rect& rect);
    void update_channel_data(size_t channel_index, int32_t rssi, ThreatLevel threat);

private:
    // Limited data for embedded system
    std::array<int32_t, 64> channel_rssi_data_;
    std::array<ThreatLevel, 64> channel_threat_data_;
    size_t channel_count_;

    // Simplified color functions
    Color get_color_for_threat(ThreatLevel threat) const;
    Color get_color_for_rssi(int32_t rssi) const;
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_ENHANCED_SPECTRUM_PAINTER_HPP__
