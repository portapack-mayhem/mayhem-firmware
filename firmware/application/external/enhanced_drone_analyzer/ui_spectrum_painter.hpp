// ui_spectrum_painter.hpp
// Spectrum painter for Enhanced Drone Analyzer - embedded compatible version
// Derived from military spectrum analysis tools

#ifndef __UI_SPECTRUM_PAINTER_HPP__
#define __UI_SPECTRUM_PAINTER_HPP__

#include <cstdint>
#include <array>

namespace ui::external_app::enhanced_drone_analyzer {

// Color definition for embedded system
struct Color {
    uint8_t r, g, b;
    constexpr Color(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0)
        : r(red), g(green), b(blue) {}

    static constexpr Color red() { return Color(255, 0, 0); }
    static constexpr Color green() { return Color(0, 255, 0); }
    static constexpr Color blue() { return Color(0, 0, 255); }
    static constexpr Color yellow() { return Color(255, 255, 0); }
    static constexpr Color black() { return Color(0, 0, 0); }
    static constexpr Color white() { return Color(255, 255, 255); }
};

// Embedded-compatible rectangle structure
struct Rect {
    int16_t left, top, right, bottom;
    constexpr Rect(int16_t l = 0, int16_t t = 0, int16_t r = 0, int16_t b = 0)
        : left(l), top(t), right(r), bottom(b) {}

    int16_t width() const { return right - left; }
    int16_t height() const { return bottom - top; }
    constexpr Rect origin() const {
        return Rect(0, 0, right - left, bottom - top);
    }
};

// Military threat level colors
enum class ThreatLevel {
    NONE = 0,
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

// Painter interface for portable rendering
class Painter {
public:
    virtual void fill_rectangle(const Rect& rect, const Color& color) = 0;
    virtual void draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, const Color& color) = 0;
    virtual void draw_pixel(int16_t x, int16_t y, const Color& color) = 0;
    virtual void draw_string(int16_t x, int16_t y, const char* str) = 0;
    virtual void draw_string_centered(const Rect& rect, const char* str) = 0;

    virtual Color get_threat_color(ThreatLevel threat) const {
        switch (threat) {
            case ThreatLevel::CRITICAL: return Color::red();
            case ThreatLevel::HIGH: return Color(255, 165, 0); // Orange
            case ThreatLevel::MEDIUM: return Color::yellow();
            case ThreatLevel::LOW: return Color::green();
            default: return Color::black();
        }
    }
};

// Embedded-spectrum painter - optimized for PortaPack Mayhem
class EnhancedSpectrumPainter {
public:
    EnhancedSpectrumPainter();

    void initialize();
    void paint(Painter& painter, const Rect& rect);
    void update_channel_data(size_t channel_index, int32_t rssi, ThreatLevel threat);

private:
    static constexpr size_t MAX_CHANNELS = 64; // Embedded constraint

    // Channel data storage
    std::array<int32_t, MAX_CHANNELS> channel_rssi_data_;
    std::array<ThreatLevel, MAX_CHANNELS> channel_threat_data_;
    size_t active_channel_count_;

    // Get color for RSSI level (-120 to -20 dBm range)
    Color get_rssi_color(int32_t rssi_db) const;

    // Get color for threat level
    Color get_threat_color(ThreatLevel threat) const;

    // Draw single channel bar
    void draw_channel_bar(Painter& painter, const Rect& rect,
                         size_t channel_index, size_t total_channels);

    // Draw threat indicators
    void draw_threat_indicators(Painter& painter, const Rect& rect);
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_SPECTRUM_PAINTER_HPP__
