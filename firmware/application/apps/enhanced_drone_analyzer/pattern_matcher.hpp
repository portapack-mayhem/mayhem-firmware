#ifndef PATTERN_MATCHER_HPP
#define PATTERN_MATCHER_HPP

#include <cstdint>
#include <cstddef>

#include "pattern_types.hpp"
#include "constants.hpp"

namespace drone_analyzer {

/**
 * @brief SAD-based pattern matching with frequency-proximity pre-filter.
 * @note Pure 16-bin waveform comparison. Each pattern's match_threshold is
 *       auto-tuned at save time from the captured peak's SNR margin
 *       (see PatternManagerView::capture_and_save).
 * @note Pure integer math, no heap, no exceptions, no virtual functions.
 * @note Reentrant — single instance can be shared between threads if the
 *       caller serializes access (matches PatternManager's mutex).
 *
 * Stack: ~16 bytes per call (normalized[16] on caller frame).
 * Flash: 0 (header only).
 * SRAM: 0 (no static state, no members).
 */
class PatternMatcher {
public:
    PatternMatcher() noexcept = default;

    void set_patterns(const SignalPattern* patterns, size_t count) noexcept;
    void clear_patterns() noexcept;

    /**
     * @brief Match spectrum against all enabled patterns.
     * @param spectrum_256 Raw FFT spectrum (256 bins)
     * @param current_freq Current tuned frequency (Hz). If non-zero, patterns
     *                     with center_freq set are filtered by frequency proximity.
     *                     Pass 0 to disable frequency filtering (shape-only match).
     * @return Best match result (may be no_match if score below per-pattern threshold).
     */
    [[nodiscard]] PatternMatchResult match(
        const uint8_t* spectrum_256,
        FreqHz current_freq = 0
    ) noexcept;

    /**
     * @brief Normalize 256-bin FFT to 16-bin waveform for pattern matching.
     * @param fft_256 Raw FFT spectrum data (256 bins, 0-255)
     * @param wave_16 Output 16-bin normalized waveform
     * @note Skips DC spike (bins FFT_DC_SPIKE_START..FFT_DC_SPIKE_END)
     *       and filter rolloff edges (0..PATTERN_NORM_EDGE_SKIP-1, tail end).
     * @note Shared between PatternMatcher and DroneScannerUI so saved
     *       patterns match against live spectra identically.
     */
    static void normalize(
        const uint8_t* fft_256,
        uint8_t* wave_16
    ) noexcept;

    /**
     * @brief Normalize 240-pixel Looking Glass buffer to 16-bin waveform.
     * @param lg_buffer_240 Looking Glass reordered spectrum (240 pixels, continuous,
     *                       no DC gap — produced by SweepProcessor::reorder_frame())
     * @param wave_16       Output 16-bin normalized waveform (same format as normalize())
     * @note Skips first/last 4 pixels (filter rolloff). No DC gap to skip — the
     *       Looking Glass reordering already eliminated it.
     * @note The 16-bin output format is IDENTICAL to normalize(), so stored patterns
     *       remain compatible. The SAD comparison works the same way.
     */
    static void normalize_from_lg(
        const uint8_t* lg_buffer_240,
        uint8_t* wave_16
    ) noexcept;

    /**
     * @brief Match a Looking Glass reordered buffer against all enabled patterns.
     * @param lg_buffer_240 Looking Glass reordered spectrum (240 pixels)
     * @param current_freq  Current tuned frequency for proximity filter. Pass 0 to disable.
     * @return Best match result (same format as match()).
     * @note Same as match() but normalizes from LG buffer instead of raw 256-bin FFT.
     */
    [[nodiscard]] PatternMatchResult match_from_lg(
        const uint8_t* lg_buffer_240,
        FreqHz current_freq = 0
    ) noexcept;

private:
    const SignalPattern* patterns_{nullptr};
    size_t pattern_count_{0};

    /**
     * @brief Compute SAD-based similarity (0-1000) after amplitude-normalizing
     *        both waveforms to their own peak.
     * @note Pure waveform comparison, no feature awareness.
     */
    [[nodiscard]] static uint16_t compute_similarity(
        const uint8_t* wave_16,
        const uint8_t* pattern_wave
    ) noexcept;
};

} // namespace drone_analyzer

#endif // PATTERN_MATCHER_HPP
