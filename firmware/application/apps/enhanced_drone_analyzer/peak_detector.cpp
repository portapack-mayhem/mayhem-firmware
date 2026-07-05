#include <cstdint>
#include <cstddef>

#include "peak_detector.hpp"
#include "constants.hpp"

namespace drone_analyzer {

uint8_t PeakDetector::quickselect_percentile(
    uint8_t* buf,
    size_t count,
    uint8_t percentile
) noexcept {
    if (buf == nullptr || count == 0) return 0;

    const size_t k = (count * static_cast<size_t>(percentile)) / 100;
    const size_t k_safe = (k < count) ? k : count - 1;
    size_t qs_left = 0;
    size_t qs_right = count - 1;

    while (qs_left < qs_right) {
        const size_t pivot_idx = qs_left + (qs_right - qs_left) / 2;
        const uint8_t pivot = buf[pivot_idx];
        buf[pivot_idx] = buf[qs_right];
        buf[qs_right] = pivot;

        size_t store = qs_left;
        for (size_t i = qs_left; i < qs_right; ++i) {
            if (buf[i] < pivot) {
                const uint8_t t = buf[store];
                buf[store] = buf[i];
                buf[i] = t;
                ++store;
            }
        }
        {
            const uint8_t t = buf[store];
            buf[store] = buf[qs_right];
            buf[qs_right] = t;
        }

        if (store == k_safe) break;
        if (store < k_safe) qs_left = store + 1;
        else qs_right = store - 1;
    }
    return buf[k_safe];
}

PeakDetector::PeakInfo PeakDetector::find(
    const uint8_t* spectrum,
    uint8_t* sort_buf,
    Range range,
    EdgePolicy edge,
    uint8_t noise_percentile
) noexcept {
    PeakInfo out{};
    if (spectrum == nullptr || sort_buf == nullptr) return out;

    const size_t edge_skip = (edge == EdgePolicy::Wide)
        ? static_cast<size_t>(FFT_EDGE_SKIP)
        : static_cast<size_t>(FFT_EDGE_SKIP_NARROW);

    size_t lower_lo = edge_skip;
    size_t lower_hi = FFT_DC_SPIKE_START;
    size_t upper_lo = FFT_DC_SPIKE_END;
    size_t upper_hi = FFT_BIN_COUNT - edge_skip;

    if (range == Range::LowerOnly) {
        upper_lo = lower_hi;
        upper_hi = lower_hi;
    } else if (range == Range::UpperOnly) {
        lower_lo = lower_hi;
        lower_hi = lower_hi;
    }

    // --- 1. Noise floor (median of usable bins) ---
    size_t idx = 0;
    uint8_t peak_value = 0;
    size_t peak_index = lower_lo;

    for (size_t i = lower_lo; i < lower_hi; ++i) {
        sort_buf[idx] = spectrum[i];
        if (spectrum[i] > peak_value) {
            peak_value = spectrum[i];
            peak_index = i;
        }
        ++idx;
    }
    for (size_t i = upper_lo; i < upper_hi; ++i) {
        sort_buf[idx] = spectrum[i];
        if (spectrum[i] > peak_value) {
            peak_value = spectrum[i];
            peak_index = i;
        }
        ++idx;
    }

    out.usable_count = idx;
    if (idx == 0) return out;

    out.value = peak_value;
    out.index = peak_index;
    out.noise_floor = quickselect_percentile(sort_buf, idx, noise_percentile);
    out.margin = (peak_value > out.noise_floor) ? (peak_value - out.noise_floor) : 0;

    // --- 2. Width measurement (extend left/right while above half-margin) ---
    if (out.margin == 0) {
        out.width = 1;
        out.left = peak_index;
        out.right = peak_index;
        return out;
    }

    const uint8_t elevated = out.noise_floor + (out.margin / 3);

    size_t left = peak_index;
    while (left > edge_skip) {
        size_t prev = left - 1;
        const bool in_dc = (prev >= FFT_DC_SPIKE_START) && (prev < FFT_DC_SPIKE_END);
        if (in_dc) { --left; continue; }
        if (spectrum[prev] < elevated) break;
        --left;
    }

    size_t right = peak_index;
    while (right < FFT_BIN_COUNT - edge_skip - 1) {
        size_t next = right + 1;
        const bool in_dc = (next >= FFT_DC_SPIKE_START) && (next < FFT_DC_SPIKE_END);
        if (in_dc) { ++right; continue; }
        if (spectrum[next] < elevated) break;
        ++right;
    }

    out.left = left;
    out.right = right;
    out.width = right - left + 1;
    return out;
}

} // namespace drone_analyzer
