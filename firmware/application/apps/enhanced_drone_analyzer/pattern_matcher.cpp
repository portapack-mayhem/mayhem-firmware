#include "pattern_matcher.hpp"
#include "constants.hpp"

namespace drone_analyzer {

void PatternMatcher::set_patterns(const SignalPattern* patterns, size_t count) noexcept {
    if (patterns == nullptr) {
        pattern_count_ = 0;
        patterns_ = nullptr;
        return;
    }
    patterns_ = patterns;
    const size_t safe_count = (count > MAX_PATTERNS) ? MAX_PATTERNS : count;
    pattern_count_ = safe_count;
}

void PatternMatcher::clear_patterns() noexcept {
    patterns_ = nullptr;
    pattern_count_ = 0;
}

void PatternMatcher::normalize(
    const uint8_t* fft_256,
    uint8_t* wave_16
) noexcept {
    if (fft_256 == nullptr || wave_16 == nullptr) return;

    constexpr size_t valid_start = PATTERN_NORM_EDGE_SKIP;
    constexpr size_t valid_end = FFT_BIN_COUNT - PATTERN_NORM_EDGE_SKIP;
    constexpr size_t usable_bins = valid_end - valid_start;

    for (size_t i = 0; i < PATTERN_WAVEFORM_SIZE; ++i) {
        const size_t start = valid_start + (i * usable_bins / PATTERN_WAVEFORM_SIZE);
        const size_t end   = valid_start + ((i + 1) * usable_bins / PATTERN_WAVEFORM_SIZE);

        uint32_t sum = 0;
        size_t count = 0;
        for (size_t j = start; j < end; ++j) {
            if (j >= FFT_DC_SPIKE_START && j < FFT_DC_SPIKE_END) continue;
            sum += fft_256[j];
            ++count;
        }
        wave_16[i] = (count > 0) ? static_cast<uint8_t>(sum / count) : 0;
    }
}

void PatternMatcher::normalize_from_lg(
    const uint8_t* lg_buffer_240,
    uint8_t* wave_16
) noexcept {
    if (lg_buffer_240 == nullptr || wave_16 == nullptr) return;

    // Skip first/last edge pixels (filter rolloff + DC spike contamination).
    // Pixels 238-239 are already zeroed (DC spike). Total usable = 232 pixels.
    constexpr size_t LG_EDGE_SKIP_PX = 4;
    constexpr size_t valid_start = LG_EDGE_SKIP_PX;
    constexpr size_t valid_end = COMPOSITE_SIZE - LG_EDGE_SKIP_PX;
    constexpr size_t usable_px = valid_end - valid_start;

    for (size_t i = 0; i < PATTERN_WAVEFORM_SIZE; ++i) {
        const size_t start = valid_start + (i * usable_px / PATTERN_WAVEFORM_SIZE);
        const size_t end   = valid_start + ((i + 1) * usable_px / PATTERN_WAVEFORM_SIZE);

        uint32_t sum = 0;
        size_t count = 0;
        for (size_t j = start; j < end; ++j) {
            sum += lg_buffer_240[j];
            ++count;
        }
        wave_16[i] = (count > 0) ? static_cast<uint8_t>(sum / count) : 0;
    }
}

PatternMatchResult PatternMatcher::match_from_lg(
    const uint8_t* lg_buffer_240,
    FreqHz current_freq
) noexcept {
    if (lg_buffer_240 == nullptr || pattern_count_ == 0 || patterns_ == nullptr) {
        return PatternMatchResult::no_match();
    }

    uint8_t normalized[PATTERN_WAVEFORM_SIZE];
    normalize_from_lg(lg_buffer_240, normalized);

    PatternMatchResult best;

    for (size_t i = 0; i < pattern_count_; ++i) {
        const SignalPattern& pattern = patterns_[i];
        if (!pattern.is_enabled()) continue;

        if (current_freq > 0 && pattern.center_freq > 0) {
            const FreqHz half_range = (pattern.range_width > 0)
                ? (pattern.range_width / 2)
                : static_cast<FreqHz>(FREQUENCY_BANDWIDTH_HZ);
            const FreqHz diff = (current_freq > pattern.center_freq)
                ? (current_freq - pattern.center_freq)
                : (pattern.center_freq - current_freq);
            if (diff > half_range) continue;
        }

        const uint16_t score = compute_similarity(normalized, pattern.waveform);

        if (score > best.score) {
            best.score = score;
            best.pattern_index = i;
        }
    }

    if (best.score > 0 && pattern_count_ > 0) {
        const uint16_t threshold = (patterns_[best.pattern_index].match_threshold > 0)
            ? patterns_[best.pattern_index].match_threshold
            : static_cast<uint16_t>(DEFAULT_PATTERN_SIMILARITY_THRESHOLD);
        if (best.score >= threshold) {
            best.matched = true;
        }
    }

    return best;
}

uint16_t PatternMatcher::compute_similarity(
    const uint8_t* wave_16,
    const uint8_t* pattern_wave
) noexcept {
    uint8_t peak_a = 0;
    uint8_t peak_b = 0;
    for (size_t i = 0; i < PATTERN_WAVEFORM_SIZE; ++i) {
        if (wave_16[i] > peak_a) peak_a = wave_16[i];
        if (pattern_wave[i] > peak_b) peak_b = pattern_wave[i];
    }
    if (peak_a == 0 || peak_b == 0) return 0;

    // Amplitude gate: reject matches where the live signal is too weak
    // relative to the pattern. Prevents noise-level signals from matching
    // strong saved patterns via pure shape similarity.
    if (peak_a < (peak_b / PATTERN_MIN_AMPLITUDE_RATIO)) return 0;

    uint32_t diff_sum = 0;
    for (size_t i = 0; i < PATTERN_WAVEFORM_SIZE; ++i) {
        const uint8_t a_norm = static_cast<uint8_t>(
            (static_cast<uint32_t>(wave_16[i]) * 255U) / peak_a);
        const uint8_t b_norm = static_cast<uint8_t>(
            (static_cast<uint32_t>(pattern_wave[i]) * 255U) / peak_b);
        const int16_t d = static_cast<int16_t>(a_norm) - static_cast<int16_t>(b_norm);
        diff_sum += (d >= 0) ? static_cast<uint16_t>(d) : static_cast<uint16_t>(-d);
    }

    constexpr uint32_t MAX_SAD = PATTERN_WAVEFORM_SIZE * 255;
    const uint32_t clamped = (diff_sum > MAX_SAD) ? MAX_SAD : diff_sum;
    const uint32_t score = ((MAX_SAD - clamped) * 1000U) / MAX_SAD;
    return static_cast<uint16_t>(score);
}

PatternMatchResult PatternMatcher::match(
    const uint8_t* spectrum_256,
    FreqHz current_freq
) noexcept {
    if (spectrum_256 == nullptr || pattern_count_ == 0 || patterns_ == nullptr) {
        return PatternMatchResult::no_match();
    }

    uint8_t normalized[PATTERN_WAVEFORM_SIZE];
    normalize(spectrum_256, normalized);

    PatternMatchResult best;

    for (size_t i = 0; i < pattern_count_; ++i) {
        const SignalPattern& pattern = patterns_[i];
        if (!pattern.is_enabled()) continue;

        // Frequency proximity filter.
        if (current_freq > 0 && pattern.center_freq > 0) {
            const FreqHz half_range = (pattern.range_width > 0)
                ? (pattern.range_width / 2)
                : static_cast<FreqHz>(FREQUENCY_BANDWIDTH_HZ);
            const FreqHz diff = (current_freq > pattern.center_freq)
                ? (current_freq - pattern.center_freq)
                : (pattern.center_freq - current_freq);
            if (diff > half_range) continue;
        }

        const uint16_t score = compute_similarity(normalized, pattern.waveform);

        if (score > best.score) {
            best.score = score;
            best.pattern_index = i;
        }
    }

    if (best.score > 0 && pattern_count_ > 0) {
        const uint16_t threshold = (patterns_[best.pattern_index].match_threshold > 0)
            ? patterns_[best.pattern_index].match_threshold
            : static_cast<uint16_t>(DEFAULT_PATTERN_SIMILARITY_THRESHOLD);
        if (best.score >= threshold) {
            best.matched = true;
        }
    }

    return best;
}

} // namespace drone_analyzer
