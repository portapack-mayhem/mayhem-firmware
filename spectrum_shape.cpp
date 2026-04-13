#include <cstdint>
#include <cstddef>
#include "spectrum_shape.hpp"
#include "constants.hpp"

namespace drone_analyzer {

uint8_t SpectrumShape::compute_noise_floor(
    const uint8_t* spectrum,
    uint8_t* sort_buf,
    size_t& usable_count
) noexcept {
    usable_count = 0;
    for (size_t i = FFT_EDGE_SKIP_NARROW; i < FFT_BIN_COUNT - FFT_EDGE_SKIP_NARROW; ++i) {
        if (i >= FFT_DC_SPIKE_START && i < FFT_DC_SPIKE_END) continue;
        sort_buf[usable_count++] = spectrum[i];
    }
    if (usable_count == 0) return 0;

    const size_t k = usable_count / 2;
    uint8_t qs_left = 0;
    uint8_t qs_right = static_cast<uint8_t>(usable_count) - 1;

    while (qs_left < qs_right) {
        const uint8_t pivot_idx = qs_left + (qs_right - qs_left) / 2;
        uint8_t pivot = sort_buf[pivot_idx];
        sort_buf[pivot_idx] = sort_buf[qs_right];
        sort_buf[qs_right] = pivot;
        uint8_t store = qs_left;
        for (uint8_t i = qs_left; i < qs_right; ++i) {
            if (sort_buf[i] < pivot) {
                uint8_t t = sort_buf[store];
                sort_buf[store] = sort_buf[i];
                sort_buf[i] = t;
                store++;
            }
        }
        {
            uint8_t t = sort_buf[store];
            sort_buf[store] = sort_buf[qs_right];
            sort_buf[qs_right] = t;
        }
        if (store == k) break;
        if (store < k) qs_left = store + 1;
        else qs_right = store - 1;
    }

    return sort_buf[k];
}

size_t SpectrumShape::find_peak(
    const uint8_t* spectrum,
    uint8_t noise_floor,
    uint8_t& out_peak_value
) noexcept {
    uint8_t peak_value = noise_floor;
    size_t peak_index = FFT_EDGE_SKIP_NARROW;

    for (size_t i = FFT_EDGE_SKIP_NARROW; i < FFT_BIN_COUNT - FFT_EDGE_SKIP_NARROW; ++i) {
        if (i >= FFT_DC_SPIKE_START && i < FFT_DC_SPIKE_END) continue;
        if (spectrum[i] > peak_value) {
            peak_value = spectrum[i];
            peak_index = i;
        }
    }

    out_peak_value = peak_value;
    return peak_index;
}

size_t SpectrumShape::measure_width(
    const uint8_t* spectrum,
    size_t peak_index,
    uint8_t elevated_threshold,
    size_t& out_left,
    size_t& out_right
) noexcept {
    size_t left = peak_index;
    while (left > FFT_EDGE_SKIP_NARROW) {
        size_t prev = left - 1;
        if (prev >= FFT_DC_SPIKE_START && prev < FFT_DC_SPIKE_END) { --left; continue; }
        if (spectrum[prev] < elevated_threshold) break;
        --left;
    }

    size_t right = peak_index;
    while (right < FFT_BIN_COUNT - FFT_EDGE_SKIP_NARROW - 1) {
        size_t next = right + 1;
        if (next >= FFT_DC_SPIKE_START && next < FFT_DC_SPIKE_END) { ++right; continue; }
        if (spectrum[next] < elevated_threshold) break;
        ++right;
    }

    out_left = left;
    out_right = right;
    return right - left + 1;
}

int32_t SpectrumShape::compute_avg_margin(
    const uint8_t* spectrum,
    size_t left,
    size_t right,
    uint8_t noise_floor
) noexcept {
    int32_t margin_sum = 0;
    size_t bin_count = 0;
    for (size_t i = left; i <= right; ++i) {
        if (i >= FFT_DC_SPIKE_START && i < FFT_DC_SPIKE_END) continue;
        if (spectrum[i] > noise_floor) {
            margin_sum += (spectrum[i] - noise_floor);
            ++bin_count;
        }
    }
    return (bin_count > 0) ? margin_sum / static_cast<int32_t>(bin_count) : 0;
}

bool SpectrumShape::check_sharpness(
    uint8_t peak_margin,
    int32_t avg_margin,
    uint8_t threshold
) noexcept {
    if (threshold <= 50 || avg_margin <= 0) return true;
    const int32_t sharpness = (static_cast<int32_t>(peak_margin) * 100) / avg_margin;
    return sharpness >= threshold;
}

bool SpectrumShape::check_peak_ratio(
    uint8_t peak_margin,
    size_t signal_width,
    uint8_t threshold
) noexcept {
    if (threshold == 0 || signal_width == 0) return true;
    const int32_t ratio = (static_cast<int32_t>(peak_margin) * 10) / static_cast<int32_t>(signal_width);
    return ratio >= threshold;
}

bool SpectrumShape::check_valley_depth(
    const uint8_t* spectrum,
    size_t left,
    size_t right,
    uint8_t noise_floor,
    uint8_t threshold
) noexcept {
    if (threshold == 0) return true;

    uint8_t left_valley = 0;
    uint8_t right_valley = 0;

    if (left > FFT_EDGE_SKIP_NARROW) {
        size_t lv = left - 1;
        if (!(lv >= FFT_DC_SPIKE_START && lv < FFT_DC_SPIKE_END) && spectrum[lv] > noise_floor) {
            left_valley = spectrum[lv] - noise_floor;
        }
    }

    if (right < FFT_BIN_COUNT - FFT_EDGE_SKIP_NARROW - 1) {
        size_t rv = right + 1;
        if (!(rv >= FFT_DC_SPIKE_START && rv < FFT_DC_SPIKE_END) && spectrum[rv] > noise_floor) {
            right_valley = spectrum[rv] - noise_floor;
        }
    }

    const uint8_t max_valley = (left_valley > right_valley) ? left_valley : right_valley;
    return max_valley < threshold;
}

bool SpectrumShape::check_flatness(
    uint8_t peak_margin,
    int32_t avg_margin,
    uint8_t threshold
) noexcept {
    if (threshold <= 50 || avg_margin <= 0) return true;
    const int32_t flatness = (static_cast<int32_t>(peak_margin) * 100) / avg_margin;
    return flatness >= threshold;
}

bool SpectrumShape::check_symmetry(
    size_t peak_index,
    size_t left,
    size_t right,
    size_t signal_width,
    uint8_t threshold
) noexcept {
    if (threshold == 0 || signal_width <= 1) return true;

    const size_t left_width = peak_index - left;
    const size_t right_width = right - peak_index;
    const size_t max_side = (left_width > right_width) ? left_width : right_width;
    const size_t min_side = (left_width < right_width) ? left_width : right_width;

    if (max_side == 0) return true;
    const uint8_t sym_pct = static_cast<uint8_t>((min_side * 100) / max_side);
    return sym_pct >= threshold;
}

SpectrumShape::AnalysisResult SpectrumShape::analyze(
    const uint8_t* spectrum,
    uint8_t* sort_buf,
    const Config& config
) noexcept {
    AnalysisResult result;

    if (spectrum == nullptr || sort_buf == nullptr) return result;

    size_t usable_count = 0;
    result.noise_floor = compute_noise_floor(spectrum, sort_buf, usable_count);
    result.peak_index = find_peak(spectrum, result.noise_floor, result.peak_value);
    result.peak_margin = result.peak_value - result.noise_floor;

    if (result.peak_margin < config.margin) return result;

    const uint8_t elevated_threshold = result.noise_floor + (result.peak_margin / 4);
    size_t sig_left = 0, sig_right = 0;
    result.signal_width = measure_width(spectrum, result.peak_index, elevated_threshold, sig_left, sig_right);

    if (result.signal_width < config.min_width) return result;
    if (result.signal_width > config.max_width) return result;

    int32_t avg_margin = 0;
    if (config.peak_sharpness > 50 || config.flatness > 50) {
        avg_margin = compute_avg_margin(spectrum, sig_left, sig_right, result.noise_floor);
    }

    if (!check_sharpness(result.peak_margin, avg_margin, config.peak_sharpness)) return result;
    if (!check_peak_ratio(result.peak_margin, result.signal_width, config.peak_ratio)) return result;
    if (!check_valley_depth(spectrum, sig_left, sig_right, result.noise_floor, config.valley_depth)) return result;
    if (!check_flatness(result.peak_margin, avg_margin, config.flatness)) return result;
    if (!check_symmetry(result.peak_index, sig_left, sig_right, result.signal_width, config.symmetry)) return result;

    result.signal_detected = true;
    return result;
}

} // namespace drone_analyzer
