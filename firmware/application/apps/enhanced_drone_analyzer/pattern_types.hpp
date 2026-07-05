#ifndef PATTERN_TYPES_HPP
#define PATTERN_TYPES_HPP

#include <cstdint>
#include <cstddef>
#include <array>

#include "drone_types.hpp"
#include "constants.hpp"

namespace drone_analyzer {

/**
 * @brief Per-pattern spectral features captured at save time.
 * @note Stored in pattern file (CSV) for diagnostic display only — PatternMatcher
 *       is now pure SAD-based and does NOT consume these.
 * @note margin (peak_value - noise_floor) drives the auto-tuned match_threshold
 *       at save time (see PatternManagerView::capture_and_save).
 * @note All fields are 0-255, packed to 4 bytes — no padding, no vtable.
 */
struct PatternFeatures {
    uint8_t peak_position;   // 16-bin space (0..15) — diagnostic only
    uint8_t peak_value;      // 0..255 — captured peak amplitude
    uint8_t noise_floor;     // 0..255 — captured noise floor
    uint8_t margin;          // peak_value - noise_floor — drives auto-threshold

    PatternFeatures() noexcept
        : peak_position(0), peak_value(0), noise_floor(0), margin(0) {}

    /**
     * @brief Reset all features to zero (used when reloading patterns).
     */
    void clear() noexcept {
        peak_position = 0; peak_value = 0; noise_floor = 0; margin = 0;
    }
};

/**
 * @brief Drone RF pattern for fingerprint matching.
 * @note Loaded from /EDA/PATTERNS/<name>.TXT, one CSV line per file.
 * @note CSV layout: name,wave[16],features[4],threshold,flags,center_freq,range_width
 *                   (25 fields). Old 29-field files still load — extra trailing
 *                   fields are ignored by the reader.
 * @note match_threshold is auto-tuned from the captured peak's SNR margin
 *       at save time (see PatternManagerView::capture_and_save).
 * @note center_freq + range_width drive the frequency-proximity filter in
 *       PatternMatcher::match().
 */
struct SignalPattern {
    char name[PATTERN_NAME_MAX_LEN];                  // 28 bytes
    uint8_t waveform[PATTERN_WAVEFORM_SIZE];          // 16 bytes — the actual comparison data
    PatternFeatures features;                         // 4 bytes — captured peak snapshot
    uint16_t match_threshold;                         // 2 bytes — 0..1000, auto-tuned from margin
    uint8_t flags;                                    // 1 byte — see Flags
    uint32_t created_time;                            // 4 bytes — chTimeNow() at save
    FreqHz center_freq;                               // 4 bytes — frequency proximity filter
    FreqHz range_width;                               // 4 bytes — full width around center_freq

    enum Flags : uint8_t {
        NONE    = 0x00,
        ENABLED = 0x01
    };

    SignalPattern() noexcept;

    [[nodiscard]] bool is_enabled() const noexcept {
        return (flags & ENABLED) != 0;
    }

    void set_enabled(bool enabled) noexcept {
        if (enabled) flags |= ENABLED;
        else         flags &= static_cast<uint8_t>(~ENABLED);
    }

    [[nodiscard]] bool is_valid() const noexcept {
        if (name[0] == '\0') return false;
        if (match_threshold == 0) return false;
        return true;
    }
};

/**
 * @brief Result of a pattern match attempt.
 */
struct PatternMatchResult {
    size_t pattern_index{0};
    uint16_t score{0};        // 0..1000 — base SAD-derived similarity
    bool matched{false};

    constexpr PatternMatchResult() noexcept = default;

    static PatternMatchResult no_match() noexcept {
        return {};
    }
};

} // namespace drone_analyzer

#endif // PATTERN_TYPES_HPP
