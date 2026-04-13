#include <cstdint>
#include <cstring>
#include <string_view>

#include "drone_display.hpp"

namespace drone_analyzer {

// ============================================================================
// Constructor / Destructor
// ============================================================================

DroneDisplay::DroneDisplay(const Rect parent_rect) noexcept
    : ui::View()
    , display_data_()
    , spectrum_buffer_{}
    , histogram_buffer_{}
    , status_text_{0}
    , spectrum_data_size_(0)
    , histogram_data_size_(0)
    , spectrum_visible_(true)
    , histogram_visible_(true)
    , drone_list_visible_(true)
    , status_bar_visible_(true) {
    set_parent_rect(parent_rect);
    set_status_text(STATUS_READY);
}

DroneDisplay::~DroneDisplay() noexcept {
    // Destructor - no dynamic memory to free
}

// ============================================================================
// Paint Method
// ============================================================================

void DroneDisplay::paint(Painter& painter) {
    const auto sr = screen_rect();
    const uint16_t ox = sr.location().x();
    const uint16_t oy = sr.location().y();
    const uint16_t w = sr.size().width();
    const uint16_t total_h = sr.size().height();

    // NOTE: No full-screen clear — each render_* method clears its own area.
    // This eliminates double-clear flickering.

    // Calculate section heights dynamically
    constexpr uint16_t STATUS_H = 16;
    constexpr uint16_t SPECTRUM_H = 50;
    constexpr uint16_t HISTOGRAM_H = 30;

    uint16_t remaining = total_h;

    const bool show_spec = (spectrum_visible_ && spectrum_data_size_ > 0);
    const bool show_composite = (composite_mode_ && composite_data_ != nullptr && composite_data_size_ > 0);
    const bool show_hist = (histogram_visible_ && histogram_data_size_ > 0);
    const bool show_list = (drone_list_visible_ && display_data_.drone_count > 0);

    const uint16_t spec_h = (show_spec || show_composite) ? SPECTRUM_H : 0;
    if (spec_h <= remaining) remaining -= spec_h; else remaining = 0;

    const uint16_t hist_h = show_hist ? HISTOGRAM_H : 0;
    if (hist_h <= remaining) remaining -= hist_h; else remaining = 0;

    const uint16_t status_h = (remaining >= STATUS_H) ? STATUS_H : 0;
    if (status_h <= remaining) remaining -= status_h; else remaining = 0;

    const uint16_t drone_h = remaining;

    uint16_t y_offset = oy;

    if (show_spec || show_composite) {
        if (show_composite) {
            if (dual_sweep_mode_ && sweep2_data_ != nullptr && sweep2_data_size_ > 0) {
                render_dual_composite(painter, ox, y_offset, w, spec_h);
            } else if (multi_zone_count_ > 1) {
                render_multi_zone(painter, ox, y_offset, w, spec_h);
            } else {
                render_composite(painter, composite_data_, composite_data_size_,
                                ox, y_offset, w, spec_h);
            }
        } else {
            render_spectrum(painter, spectrum_buffer_.data(), spectrum_data_size_,
                            ox, y_offset, w, spec_h);
        }
        y_offset += spec_h;
    }

    if (show_hist) {
        render_histogram(painter, histogram_buffer_.data(), histogram_data_size_,
                         ox, y_offset, w, hist_h);
        y_offset += hist_h;
    }

    if (show_list && drone_h > 0) {
        render_drone_list(painter, display_data_.drones, display_data_.drone_count,
                          ox, y_offset, w, drone_h);
        y_offset += drone_h;
    }

    if (status_h > 0) {
        render_status_bar(painter, status_text_, ox, y_offset, w, status_h);
    }
}

// ============================================================================
// Render Methods
// ============================================================================

void DroneDisplay::render_spectrum(
    Painter& painter,
    const uint8_t* spectrum_data,
    size_t spectrum_size,
    uint16_t start_x,
    uint16_t start_y,
    uint16_t width,
    uint16_t height
) noexcept {
    if (spectrum_data == nullptr || spectrum_size == 0 || height < 4) {
        return;
    }

    draw_rectangle(painter, start_x, start_y, width, height, COLOR_BACKGROUND);
    draw_rectangle(painter, start_x, start_y, width, 1, COLOR_UNKNOWN_THREAT);
    draw_text(painter, "SPECTRUM +/-1MHz", start_x + 2, start_y + 2, COLOR_TEXT);

    constexpr uint16_t MIN_BAR_WIDTH = 2;
    const uint16_t usable_width = width - 4;
    const uint16_t bar_count = static_cast<uint16_t>(spectrum_size);
    uint16_t bar_width = usable_width / bar_count;
    if (bar_width < MIN_BAR_WIDTH) bar_width = MIN_BAR_WIDTH;

    const uint16_t chart_start_x = start_x + 2;
    const uint16_t chart_start_y = start_y + 12;
    const uint16_t chart_height = height - 14;
    if (chart_height < 4) return;

    // Noise floor and margin filtering are pre-computed in set_spectrum_data()
    // Here we only draw — pure paint, no DSP
    for (size_t i = 0; i < spectrum_size; ++i) {
        if (i >= FFT_DC_SPIKE_START && i < FFT_DC_SPIKE_END) continue;

        const uint8_t value = spectrum_data[i];
        if (value < min_color_power_) continue;

        const uint16_t bar_height = (static_cast<uint16_t>(value) * chart_height) / 255;
        const uint16_t x = chart_start_x + static_cast<uint16_t>(i) * bar_width;
        const uint16_t y = chart_start_y + chart_height - bar_height;

        uint32_t color = COLOR_LOW_THREAT;
        if (value > 200) color = COLOR_CRITICAL_THREAT;
        else if (value > 150) color = COLOR_HIGH_THREAT;
        else if (value > 100) color = COLOR_MEDIUM_THREAT;

        if (bar_height > 0) {
            draw_rectangle(painter, x, y, bar_width, bar_height, color);
        }
    }
}

void DroneDisplay::render_histogram(
    Painter& painter,
    const uint16_t* histogram_data,
    size_t histogram_size,
    uint16_t start_x,
    uint16_t start_y,
    uint16_t width,
    uint16_t height
) noexcept {
    // Validate input
    if (histogram_data == nullptr || histogram_size == 0 || width < 10 || height < 4) {
        return;
    }
    
    // Draw background with border
    draw_rectangle(painter, start_x, start_y, width, height, COLOR_BACKGROUND);
    draw_rectangle(painter, start_x, start_y, width, 1, COLOR_UNKNOWN_THREAT);
    draw_text(painter, "Power", start_x + 2, start_y + 2, COLOR_TEXT);

    // Subsample to fit screen (128 bins → max 60 bars)
    constexpr size_t MAX_BARS = 60;
    const size_t step = (histogram_size > MAX_BARS) ? (histogram_size / MAX_BARS) : 1;
    const size_t visible_count = histogram_size / step;

    // Find max value for scaling (include all visible bins)
    uint16_t max_value = 0;
    for (size_t i = 0; i < histogram_size; i += step) {
        if (histogram_data[i] > max_value) {
            max_value = histogram_data[i];
        }
    }

    if (max_value == 0) {
        draw_text(painter, "No data", start_x + 2, start_y + 12, COLOR_UNKNOWN_THREAT);
        return;
    }

    constexpr uint16_t MIN_BAR_WIDTH = 2;
    const uint16_t usable_width = width - 4;
    uint16_t bar_width = usable_width / static_cast<uint16_t>(visible_count);
    if (bar_width < MIN_BAR_WIDTH) bar_width = MIN_BAR_WIDTH;

    const uint16_t chart_start_x = start_x + 2;
    const uint16_t chart_start_y = start_y + 12;
    const uint16_t chart_height = height - 14;
    if (chart_height < 4) return;

    for (size_t bar = 0; bar < visible_count; ++bar) {
        const size_t idx = bar * step;
        const uint16_t value = histogram_data[idx];
        const uint16_t bar_height = (value * chart_height) / max_value;
        const uint16_t x = chart_start_x + static_cast<uint16_t>(bar) * bar_width;
        const uint16_t y = chart_start_y + chart_height - bar_height;

        if (bar_height > 0) {
            draw_rectangle(painter, x, y, bar_width - 1, bar_height, COLOR_MEDIUM_THREAT);
        }
    }
}

void DroneDisplay::render_drone_list(
    Painter& painter,
    const DisplayDroneEntry* drones,
    size_t drone_count,
    uint16_t start_x,
    uint16_t start_y,
    uint16_t width,
    uint16_t height
) noexcept {
    // Validate input
    if (drones == nullptr || drone_count == 0) {
        draw_rectangle(painter, start_x, start_y, width, height, COLOR_BACKGROUND);
        draw_rectangle(painter, start_x, start_y, width, 1, COLOR_UNKNOWN_THREAT);
        draw_text(painter, "DETECTED SIGNALS", start_x + 2, start_y + 2, COLOR_TEXT);
        draw_text(painter, STATUS_NO_DRONES, start_x + 2, start_y + 14, COLOR_UNKNOWN_THREAT);
        return;
    }
    
    // Draw background with border
    draw_rectangle(painter, start_x, start_y, width, height, COLOR_BACKGROUND);
    draw_rectangle(painter, start_x, start_y, width, 1, COLOR_UNKNOWN_THREAT);  // Top border
    
    // Draw header
    draw_text(painter, "DETECTED SIGNALS", start_x + 2, start_y + 2, COLOR_TEXT);
    
    // Draw column headers
    constexpr uint16_t HEADER_H = 12;
    constexpr uint16_t ENTRY_MIN_H = 22;  // Minimum height per entry
    const uint16_t list_start_y = start_y + HEADER_H;
    const uint16_t available_h = height - HEADER_H;
    
    // Calculate entry height (max 4 entries visible)
    uint16_t entry_height = available_h / static_cast<uint16_t>(drone_count);
    if (entry_height > 40) entry_height = 40;
    if (entry_height < ENTRY_MIN_H) entry_height = ENTRY_MIN_H;
    
    // Draw drone entries
    for (size_t i = 0; i < drone_count; ++i) {
        const uint16_t y = list_start_y + static_cast<uint16_t>(i) * entry_height;
        if (y + entry_height > start_y + height) break;  // Don't overflow
        draw_drone_entry(painter, drones[i], start_x, y, width, entry_height);
    }
}

void DroneDisplay::render_status_bar(
    Painter& painter,
    const char* status_text,
    uint16_t start_x,
    uint16_t start_y,
    uint16_t width,
    uint16_t height
) noexcept {
    // Validate input
    if (status_text == nullptr || height < 4) {
        return;
    }
    
    // Draw background with top border
    draw_rectangle(painter, start_x, start_y, width, height, COLOR_BACKGROUND);
    draw_rectangle(painter, start_x, start_y, width, 1, COLOR_UNKNOWN_THREAT);
    
    // Draw status text centered vertically
    const uint16_t text_y = start_y + (height > 10 ? 4 : 1);
    draw_text(painter, status_text, start_x + 4, text_y, COLOR_TEXT);
}

// ============================================================================
// Data Management
// ============================================================================

ErrorCode DroneDisplay::update_display_data(const DisplayData& display_data) noexcept {
    // Validate input
    const ErrorCode error = validate_drone_buffer(
        display_data.drones,
        display_data.drone_count,
        MAX_DISPLAYED_DRONES
    );
    if (error != ErrorCode::SUCCESS) {
        return error;
    }
    
    // Copy display data
    display_data_ = display_data;
    
    return ErrorCode::SUCCESS;
}

const DisplayData& DroneDisplay::get_display_data() const noexcept {
    return display_data_;
}

void DroneDisplay::clear_display(Painter& painter) noexcept {
    draw_rectangle(painter, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, COLOR_BACKGROUND);
}

ErrorCode DroneDisplay::set_spectrum_data(
    const uint8_t* spectrum_data,
    size_t spectrum_size
) noexcept {
    const ErrorCode error = validate_spectrum_data(spectrum_data, spectrum_size);
    if (error != ErrorCode::SUCCESS) {
        return error;
    }

    const size_t count = (spectrum_size < spectrum_buffer_.size()) ? spectrum_size : spectrum_buffer_.size();
    spectrum_data_size_ = count;

    // Pre-filter: apply shape margin filter during copy (not during paint)
    // This moves noise floor computation out of the hot paint() path
    if (spectrum_shape_margin_ > 0) {
        // Quickselect median — O(n) vs O(n²) insertion sort
        uint8_t sorted[240];
        size_t sort_count = 0;
        for (size_t i = 0; i < count && sort_count < 240; ++i) {
            if (i >= FFT_DC_SPIKE_START && i < FFT_DC_SPIKE_END) continue;
            sorted[sort_count++] = spectrum_data[i];
        }
        if (sort_count > 0) {
            const size_t k = sort_count / 2;
            size_t qs_left = 0;
            size_t qs_right = sort_count - 1;
            while (qs_left < qs_right) {
                const size_t pivot_idx = qs_left + (qs_right - qs_left) / 2;
                const uint8_t pivot = sorted[pivot_idx];
                sorted[pivot_idx] = sorted[qs_right];
                sorted[qs_right] = pivot;
                size_t store = qs_left;
                for (size_t i = qs_left; i < qs_right; ++i) {
                    if (sorted[i] < pivot) {
                        const uint8_t t = sorted[store];
                        sorted[store] = sorted[i];
                        sorted[i] = t;
                        ++store;
                    }
                }
                {
                    const uint8_t t = sorted[store];
                    sorted[store] = sorted[qs_right];
                    sorted[qs_right] = t;
                }
                if (store == k) break;
                if (store < k) qs_left = store + 1;
                else qs_right = store - 1;
            }
            const uint8_t noise_floor = sorted[k];
            const uint8_t display_threshold = noise_floor + spectrum_shape_margin_;
            for (size_t i = 0; i < count; ++i) {
                const uint8_t val = spectrum_data[i];
                spectrum_buffer_[i] = (val >= display_threshold) ? val : 0;
            }
        } else {
            for (size_t i = 0; i < count; ++i) {
                spectrum_buffer_[i] = spectrum_data[i];
            }
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            spectrum_buffer_[i] = spectrum_data[i];
        }
    }

    return ErrorCode::SUCCESS;
}

ErrorCode DroneDisplay::set_histogram_data(
    const uint16_t* histogram_data,
    size_t histogram_size
) noexcept {
    // Validate input
    const ErrorCode error = validate_histogram_data(histogram_data, histogram_size);
    if (error != ErrorCode::SUCCESS) {
        return error;
    }
    
    // Copy histogram data
    histogram_data_size_ = histogram_size;
    for (size_t i = 0; i < histogram_size && i < histogram_buffer_.size(); ++i) {
        histogram_buffer_[i] = histogram_data[i];
    }
    
    return ErrorCode::SUCCESS;
}

void DroneDisplay::set_status_text(const char* status_text) noexcept {
    if (status_text == nullptr) {
        return;
    }
    
    // Copy status text
    size_t i = 0;
    while (i < MAX_TEXT_LENGTH - 1 && status_text[i] != '\0') {
        status_text_[i] = status_text[i];
        ++i;
    }
    status_text_[i] = '\0';
}

const char* DroneDisplay::get_status_text() const noexcept {
    return status_text_;
}

// ============================================================================
// Drawing Helpers
// ============================================================================

void DroneDisplay::draw_spectrum_line(
    Painter& painter,
    uint16_t x,
    uint16_t y,
    uint32_t color
) noexcept {
    draw_rectangle(painter, x, y, 1, 1, color);
}

void DroneDisplay::draw_histogram_bar(
    Painter& painter,
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    uint32_t color
) noexcept {
    if (width == 0 || height == 0) {
        return;
    }
    draw_rectangle(painter, x, y, width, height, color);
}

void DroneDisplay::draw_drone_entry(
    Painter& painter,
    const DisplayDroneEntry& drone,
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height
) noexcept {
    // Draw entry separator line
    draw_rectangle(painter, x, y + height - 1, width, 1, COLOR_UNKNOWN_THREAT);
    
    // Format frequency
    char freq_buffer[16];
    format_frequency(drone.frequency, freq_buffer, sizeof(freq_buffer));
    
    // Format RSSI
    char rssi_buffer[16];
    format_rssi(drone.rssi, rssi_buffer, sizeof(rssi_buffer));
    
    // Format threat level string
    const char* threat_str = "";
    switch (drone.threat) {
        case ThreatLevel::CRITICAL: threat_str = "CRIT"; break;
        case ThreatLevel::HIGH:     threat_str = "HIGH"; break;
        case ThreatLevel::MEDIUM:   threat_str = "MED";  break;
        case ThreatLevel::LOW:      threat_str = "LOW";  break;
        default:                    threat_str = "---";  break;
    }
    
    // Get movement trend symbol
    char trend_symbol = MOVEMENT_TREND_SYMBOL_UNKNOWN;
    switch (drone.trend) {
        case MovementTrend::APPROACHING: trend_symbol = '<'; break;  // Approaching
        case MovementTrend::RECEDING:    trend_symbol = '>'; break;  // Receding
        case MovementTrend::STATIC:      trend_symbol = '~'; break;  // Static
        default:                         trend_symbol = '-'; break;
    }
    
    // Layout: Use proportional columns
    // Col 1 (0-40%): Type name (colored)
    // Col 2 (40-60%): Threat level
    // Col 3 (60-75%): Frequency
    // Col 4 (75-90%): RSSI
    // Col 5 (90-100%): Trend
    
    constexpr uint16_t PAD = 3;
    const uint16_t col1_end = (width * 40) / 100;
    const uint16_t col3_end = (width * 75) / 100;
    const uint16_t col4_end = (width * 90) / 100;
    
    // Line 1: Type | Threat
    draw_text(painter, drone.get_type_name(), x + PAD, y + 2, drone.display_color);
    draw_text(painter, threat_str, x + col1_end + PAD, y + 2, drone.display_color);
    
    // Line 2: Frequency | RSSI | Trend
    draw_text(painter, freq_buffer, x + PAD, y + 12, COLOR_TEXT);
    draw_text(painter, rssi_buffer, x + col3_end + PAD, y + 12, COLOR_TEXT);
    
    // Trend symbol at far right
    char trend_buffer[2] = {trend_symbol, '\0'};
    draw_text(painter, trend_buffer, x + col4_end + PAD, y + 12, COLOR_TEXT);
}

void DroneDisplay::draw_text(
    Painter& painter,
    const char* text,
    uint16_t x,
    uint16_t y,
    uint32_t color
) noexcept {
    if (text == nullptr) {
        return;
    }

    const Color fg_color = Color::RGB(color);
    const Color bg_color = Color::black();

    painter.draw_string(
        Point{x, y},
        Theme::getInstance()->fg_light->font,
        fg_color,
        bg_color,
        std::string_view(text)
    );
}

void DroneDisplay::draw_text(
    Painter& painter,
    std::string_view text,
    uint16_t x,
    uint16_t y,
    uint32_t color
) noexcept {
    if (text.empty()) {
        return;
    }

    const Color fg_color = Color::RGB(color);
    const Color bg_color = Color::black();

    painter.draw_string(
        Point{x, y},
        Theme::getInstance()->fg_light->font,
        fg_color,
        bg_color,
        text
    );
}

void DroneDisplay::draw_rectangle(
    Painter& painter,
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    uint32_t color,
    bool fill
) noexcept {
    const Color rect_color = Color::RGB(color);
    const Rect rect{x, y, width, height};

    if (fill) {
        painter.fill_rectangle(rect, rect_color);
    } else {
        painter.draw_rectangle(rect, rect_color);
    }
}

// ============================================================================
// Utility Methods
// ============================================================================

uint16_t DroneDisplay::map_rssi_to_height(
    RssiValue rssi,
    uint16_t max_height
) const noexcept {
    const int32_t rssi_range = RSSI_MAX_DBM - RSSI_MIN_DBM;
    const int32_t rssi_clamped = clamp(rssi, RSSI_MIN_DBM, RSSI_MAX_DBM);
    const int32_t rssi_normalized = rssi_clamped - RSSI_MIN_DBM;
    return static_cast<uint16_t>((rssi_normalized * max_height) / rssi_range);
}

uint32_t DroneDisplay::get_threat_color(ThreatLevel threat) const noexcept {
    switch (threat) {
        case ThreatLevel::LOW:
            return COLOR_LOW_THREAT;
        case ThreatLevel::MEDIUM:
            return COLOR_MEDIUM_THREAT;
        case ThreatLevel::HIGH:
            return COLOR_HIGH_THREAT;
        case ThreatLevel::CRITICAL:
            return COLOR_CRITICAL_THREAT;
        default:
            return COLOR_UNKNOWN_THREAT;
    }
}

static void write_uint(char*& buf, size_t& remaining, uint32_t value) noexcept {
    char tmp[10];
    int len = 0;
    if (value == 0) {
        tmp[len++] = '0';
    } else {
        while (value > 0 && len < 10) {
            tmp[len++] = '0' + static_cast<char>(value % 10);
            value /= 10;
        }
    }
    for (int i = len - 1; i >= 0 && remaining > 1; --i) {
        *buf++ = tmp[i];
        --remaining;
    }
}

static void write_uint_pad(char*& buf, size_t& remaining, uint32_t value, int pad) noexcept {
    char tmp[10];
    int len = 0;
    while (len < pad) {
        tmp[len++] = '0' + static_cast<char>(value % 10);
        value /= 10;
    }
    for (int i = len - 1; i >= 0 && remaining > 1; --i) {
        *buf++ = tmp[i];
        --remaining;
    }
}

static void write_str(char*& buf, size_t& remaining, const char* s) noexcept {
    while (*s != '\0' && remaining > 1) {
        *buf++ = *s++;
        --remaining;
    }
}

void DroneDisplay::format_frequency(
    FreqHz frequency,
    char* buffer,
    size_t buffer_size
) const noexcept {
    if (buffer == nullptr || buffer_size < 16) {
        return;
    }

    const uint32_t mhz = static_cast<uint32_t>(frequency / 1'000'000ULL);
    const uint32_t khz = static_cast<uint32_t>((frequency % 1'000'000ULL) / 1'000ULL);

    char* buf = buffer;
    size_t remaining = buffer_size;
    write_uint(buf, remaining, mhz);
    if (remaining > 1) { *buf++ = '.'; --remaining; }
    write_uint_pad(buf, remaining, khz, 3);
    write_str(buf, remaining, " MHz");
    *buf = '\0';
}

void DroneDisplay::format_rssi(
    RssiValue rssi,
    char* buffer,
    size_t buffer_size
) const noexcept {
    if (buffer == nullptr || buffer_size < 8) {
        return;
    }

    char* buf = buffer;
    size_t remaining = buffer_size;

    if (rssi < 0) {
        if (remaining > 1) { *buf++ = '-'; --remaining; }
        rssi = -rssi;
    }

    write_uint(buf, remaining, static_cast<uint32_t>(rssi));
    write_str(buf, remaining, " db");
    *buf = '\0';
}

ErrorCode DroneDisplay::validate_spectrum_data(
    const uint8_t* spectrum_data,
    size_t spectrum_size
) const noexcept {
    return validate_spectrum_buffer(spectrum_data, spectrum_size);
}

ErrorCode DroneDisplay::validate_histogram_data(
    const uint16_t* histogram_data,
    size_t histogram_size
) const noexcept {
    return validate_histogram_buffer(histogram_data, histogram_size);
}

uint16_t DroneDisplay::clamp(
    int32_t value,
    int32_t min,
    int32_t max
) const noexcept {
    if (value < min) {
        return static_cast<uint16_t>(min);
    }
    if (value > max) {
        return static_cast<uint16_t>(max);
    }
    return static_cast<uint16_t>(value);
}

void DroneDisplay::set_composite_data(const uint8_t* data, size_t size) noexcept {
    composite_data_ = data;
    composite_data_size_ = size;
}

void DroneDisplay::render_composite(
    Painter& painter,
    const uint8_t* composite_data,
    size_t composite_size,
    uint16_t start_x,
    uint16_t start_y,
    uint16_t width,
    uint16_t height
) noexcept {
    if (composite_data == nullptr || composite_size == 0 || height < 4) {
        return;
    }

    draw_rectangle(painter, start_x, start_y, width, height, COLOR_BACKGROUND);
    draw_rectangle(painter, start_x, start_y, width, 1, COLOR_UNKNOWN_THREAT);

    // Title: compact frequency range (e.g. "5700M-5900M")
    if (sweep_freq_start_ < sweep_freq_end_) {
        char title_buf[20];
        const uint32_t mhz_lo = static_cast<uint32_t>(sweep_freq_start_ / 1000000ULL);
        const uint32_t mhz_hi = static_cast<uint32_t>(sweep_freq_end_ / 1000000ULL);
        char* dst = title_buf;
        size_t rem = sizeof(title_buf);
        write_uint(dst, rem, mhz_lo);
        write_str(dst, rem, "M-");
        write_uint(dst, rem, mhz_hi);
        write_str(dst, rem, "M");
        *dst = '\0';
        draw_text(painter, title_buf, start_x + 2, start_y + 2, COLOR_TEXT);
    } else {
        draw_text(painter, "SWEEP", start_x + 2, start_y + 2, COLOR_TEXT);
    }

    const uint16_t bar_count = static_cast<uint16_t>(composite_size);
    // Each composite entry = 1 pixel (240 entries fit in 236px, painter clips)
    constexpr uint16_t bar_width = 1;

    const uint16_t chart_start_x = start_x + 2;
    const uint16_t chart_start_y = start_y + 12;
    const uint16_t chart_height = height - 14;
    if (chart_height < 4) return;

    for (uint16_t i = 0; i < bar_count; ++i) {
        const uint8_t power = composite_data[i];
        
        // Always draw something - even zero values show as baseline
        // This prevents gaps after reset() when composite[] is all zeros
        const uint16_t bar_height = (static_cast<uint16_t>(power + 1) * chart_height) / 256;
        // Minimum 1 pixel height to show bar exists
        const uint16_t final_height = (bar_height > 0) ? bar_height : 1;

        const uint16_t x = chart_start_x + i * bar_width;
        const uint16_t y = chart_start_y + chart_height - final_height;

        // Only color bars above threshold - zero values show as dark baseline
        uint32_t color = COLOR_BACKGROUND;
        if (power >= min_color_power_) {
            if (power > 200) color = COLOR_CRITICAL_THREAT;
            else if (power > 150) color = COLOR_HIGH_THREAT;
            else if (power > 100) color = COLOR_MEDIUM_THREAT;
            else color = COLOR_LOW_THREAT;
        }

        draw_rectangle(painter, x, y, bar_width, final_height, color);
    }
}

void DroneDisplay::set_multi_zone_data(const uint8_t buffers[][240], uint8_t zone_count, size_t /*buffer_size*/,
                                       const FreqHz* freq_starts, const FreqHz* freq_ends) noexcept {
    if (zone_count > MAX_ZONES) zone_count = MAX_ZONES;
    multi_zone_count_ = zone_count;
    for (uint8_t z = 0; z < zone_count; ++z) {
        multi_zone_data_[z] = buffers[z];
        zone_freq_start_[z] = freq_starts[z];
        zone_freq_end_[z] = freq_ends[z];
    }
}

void DroneDisplay::render_multi_zone(
    Painter& painter,
    uint16_t start_x,
    uint16_t start_y,
    uint16_t width,
    uint16_t height
) noexcept {
    if (multi_zone_count_ == 0 || height < 20) return;

    // Divide height into equal zones with 1px separator
    const uint16_t zone_h = height / multi_zone_count_;
    const uint16_t sep_h = 1;

    for (uint8_t z = 0; z < multi_zone_count_; ++z) {
        const uint16_t zone_y = start_y + z * (zone_h + sep_h);
        const uint8_t* data = multi_zone_data_[z];

        // Background
        draw_rectangle(painter, start_x, zone_y, width, zone_h, COLOR_BACKGROUND);

        // Separator line at top (except first zone)
        if (z > 0) {
            draw_rectangle(painter, start_x, zone_y - sep_h, width, sep_h, COLOR_UNKNOWN_THREAT);
        }

        // Title: zone frequency range
        if (data != nullptr) {
            char title[16];
            const uint32_t mhz_lo = static_cast<uint32_t>(zone_freq_start_[z] / 1000000ULL);
            const uint32_t mhz_hi = static_cast<uint32_t>(zone_freq_end_[z] / 1000000ULL);
            char* dst = title;
            size_t rem = sizeof(title);
            write_uint(dst, rem, mhz_lo);
            write_str(dst, rem, "-");
            write_uint(dst, rem, mhz_hi);
            write_str(dst, rem, "M");
            *dst = '\0';
            draw_text(painter, title, start_x + 2, zone_y + 2, COLOR_TEXT);

            // Bar chart
            constexpr uint16_t bar_width = 1;
            const uint16_t chart_y = zone_y + 12;
            const uint16_t chart_h = zone_h - 14;
            if (chart_h < 4) continue;

            for (uint16_t i = 0; i < width - 4; ++i) {
                if (i >= 240) break;
                const uint8_t power = data[i];
                if (power == 0 || power < min_color_power_) continue;

                const uint16_t bar_h = (static_cast<uint16_t>(power) * chart_h) / 255;
                if (bar_h == 0) continue;

                const uint16_t x = start_x + 2 + i * bar_width;
                const uint16_t y = chart_y + chart_h - bar_h;

                uint32_t color = COLOR_LOW_THREAT;
                if (power > 200) color = COLOR_CRITICAL_THREAT;
                else if (power > 150) color = COLOR_HIGH_THREAT;
                else if (power > 100) color = COLOR_MEDIUM_THREAT;

                draw_rectangle(painter, x, y, bar_width, bar_h, color);
            }
        }
    }
}

void DroneDisplay::set_sweep2_data(const uint8_t* data, size_t size) noexcept {
    sweep2_data_ = data;
    sweep2_data_size_ = size;
}

void DroneDisplay::render_dual_composite(
    Painter& painter,
    uint16_t start_x,
    uint16_t start_y,
    uint16_t width,
    uint16_t height
) noexcept {
    if (composite_data_ == nullptr || sweep2_data_ == nullptr || height < 8) {
        return;
    }

    // Split height between two sweep bands
    const uint16_t band_h = height / 2;
    if (band_h < 4) return;

    // Render sweep 1 (top half)
    render_composite(painter, composite_data_, composite_data_size_,
                     start_x, start_y, width, band_h);

    // Render sweep 2 (bottom half) — temporarily swap freq range
    const FreqHz saved_start = sweep_freq_start_;
    const FreqHz saved_end = sweep_freq_end_;
    sweep_freq_start_ = sweep2_freq_start_;
    sweep_freq_end_ = sweep2_freq_end_;
    render_composite(painter, sweep2_data_, sweep2_data_size_,
                     start_x, start_y + band_h, width, band_h);
    sweep_freq_start_ = saved_start;
    sweep_freq_end_ = saved_end;
}

} // namespace drone_analyzer
