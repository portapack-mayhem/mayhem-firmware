// ui_waterfall_spectrum.hpp - Waterfall spectrum visualization
// Adapted for PortaPack Mayhem firmware embedded constraints

#ifndef __UI_WATERFALL_SPECTRUM_HPP__
#define __UI_WATERFALL_SPECTRUM_HPP__

#include "ui.hpp"
#include "color.hpp"
#include <array>
#include <cstdint>

namespace ui::external_app::enhanced_drone_analyzer {

// Early access declaration for cross-header references
enum class ThreatLevel;

// Waterfall spectrum for temporal signal visualization
// OPTIMIZED for embedded systems: Ring buffer, fixed sizes, no heap allocation
class WaterfallSpectrum {
private:
    static constexpr size_t BUFFER_SIZE = 16;  // Reduced from 32 for RAM savings
    static constexpr int SLICE_PIXEL_HEIGHT = 2;

    struct WaterfallSlice {
        uint32_t timestamp;
        std::array<int16_t, 64> rssi_levels;  // Max 64 channels for embedded
        uint8_t threat_mask;  // Bitmask for threat levels (first 8 channels)
    };

    std::array<WaterfallSlice, BUFFER_SIZE> buffer_;
    size_t head_ = 0;
    size_t valid_slices_ = 0;
    bool enabled_ = false;

public:
    WaterfallSpectrum() = default;

    void enable(bool state) { enabled_ = state; }
    bool enabled() const { return enabled_; }
    bool has_data() const { return valid_slices_ > 0; }

    // Add new spectrum slice to waterfall history
    void add_slice(const std::array<int32_t, 64>& channels, size_t active_count) {
        if (!enabled_) return;

        auto& slice = buffer_[head_];
        slice.timestamp = chTimeNow();
        slice.threat_mask = 0;

        // Compress int32_t RSSI to int16_t for memory efficiency
        for (size_t i = 0; i < std::min(active_count, size_t(64)); ++i) {
            slice.rssi_levels[i] = static_cast<int16_t>(channels[i]);
        }

        // Create threat bitmask for first 8 channels (embedded efficient)
        for (size_t i = 0; i < std::min(active_count, size_t(8)); ++i) {
            if (channels[i] > -80) {  // Simple threat threshold for now
                slice.threat_mask |= (1 << i);
            }
        }

        head_ = (head_ + 1) % BUFFER_SIZE;
        valid_slices_ = std::min(valid_slices_ + 1, BUFFER_SIZE);
    }

    // Paint waterfall in specified rectangle
    void paint(Painter& painter, Rect rect, size_t active_channels) {
        if (!enabled_ || valid_slices_ == 0 || active_channels == 0) return;

        const int total_height = valid_slices_ * SLICE_PIXEL_HEIGHT;

        for (size_t time_idx = 0; time_idx < valid_slices_; ++time_idx) {
            size_t slice_idx = (head_ + BUFFER_SIZE - time_idx - 1) % BUFFER_SIZE;
            const auto& slice = buffer_[slice_idx];

            int y_base = rect.top() + total_height - (time_idx + 1) * SLICE_PIXEL_HEIGHT;

            for (size_t freq_idx = 0; freq_idx < std::min(active_channels, size_t(64)); ++freq_idx) {
                int x = rect.left() + (freq_idx * rect.width()) / active_channels;
                int16_t rssi = slice.rssi_levels[freq_idx];

                Color color = get_waterfall_color(rssi, time_idx);

                // Draw vertical line for this frequency in time
                for (int dy = 0; dy < SLICE_PIXEL_HEIGHT; ++dy) {
                    int y = y_base + dy;
                    if (y >= rect.top() && y < rect.bottom()) {
                        painter.set_pixel(x, y, color);
                    }
                }
            }

            // Draw threat indicators if present in recent slices
            if (slice.threat_mask && time_idx < 3) {
                draw_threat_indicators(painter, rect, slice, time_idx);
            }
        }
    }

    // Clear all historical data
    void clear() {
        head_ = 0;
        valid_slices_ = 0;
        // No need to clear array content for efficiency
    }

    size_t get_valid_slices() const { return valid_slices_; }

private:
    // Convert RSSI level to waterfall color
    Color get_waterfall_color(int16_t rssi, size_t time_idx) const {
        // Ignore very weak signals
        if (rssi < -100) return Color::black();

        // Color scheme: Black->Blue->Green->Yellow->Red based on signal strength
        Color base_color;
        if (rssi < -80) {
            // Very weak: Dark blue
            base_color = Color(0, 0, 50);
        } else if (rssi < -60) {
            // Weak: Blue
            base_color = Color(0, 0, std::min(255, (-rssi - 80) * 5));
        } else if (rssi < -40) {
            // Medium: Blue-Green
            base_color = Color(0, std::min(255, (-rssi - 60) * 4), 100);
        } else if (rssi < -20) {
            // Strong: Green-Yellow
            base_color = Color(std::min(255, (-rssi - 40) * 6), 255, 0);
        } else {
            // Very strong: Red
            base_color = Color(255, 0, 0);
        }

        // Apply fade effect for older data (brighter = newer, dimmer = older)
        if (time_idx > 0) {
            float fade_factor = 1.0f - (time_idx * 0.08f);
            fade_factor = std::max(0.3f, fade_factor);  // Don't fade too much

            base_color = Color(
                static_cast<uint8_t>(base_color.r() * fade_factor),
                static_cast<uint8_t>(base_color.g() * fade_factor),
                static_cast<uint8_t>(base_color.b() * fade_factor)
            );
        }

        return base_color;
    }

    // Draw small threat indicators on waterfall
    void draw_threat_indicators(Painter& painter, Rect rect, const WaterfallSlice& slice, size_t time_idx) {
        int y_text = rect.top() + (valid_slices_ - time_idx) * SLICE_PIXEL_HEIGHT - 8;

        // Only show indicators for first 8 channels to save space
        for (size_t i = 0; i < 8; ++i) {
            if (slice.threat_mask & (1 << i)) {
                int x_threat = rect.left() + (i * rect.width()) / 8;
                // Draw small red threat indicator
                painter.set_pixel(x_threat, y_text, Color::red());
                // Add small cross for emphasis
                if (x_threat + 1 < rect.right()) painter.set_pixel(x_threat + 1, y_text, Color::red());
            }
        }
    }
};

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_WATERFALL_SPECTRUM_HPP__
