#ifndef SWEEP_PROCESSOR_HPP
#define SWEEP_PROCESSOR_HPP

#include <cstdint>
#include <cstddef>
#include "message.hpp"
#include "drone_types.hpp"
#include "constants.hpp"

namespace drone_analyzer {

/**
 * @brief Maps FFT bins to composite sweep pixels (Looking Glass pattern).
 * @note Pure function — no UI dependencies, no state, no heap.
 * @note Lower sideband: screen pixels 0..119 map to FFT bins 134..253
 * @note Upper sideband: screen pixels 120..237 map to FFT bins 2..119
 * @note Pixels 238..239 unused (skip DC spike bins 120..121)
 * @note M0 baseband handles FFT; M4 only maps bins to pixels.
 */
class SweepProcessor {
public:
    static constexpr uint8_t UPPER_OFFSET = SWEEP_FFT_MAP_CROSSOVER - 2;
    static constexpr uint8_t UPPER_PIXEL_END = SWEEP_PIXELS_PER_SLICE - 2;

    /**
     * @brief Process one FFT frame into a composite pixel buffer.
     * @param spectrum       256-bin FFT power values from baseband
     * @param composite      Output pixel buffer (COMPOSITE_SIZE bytes)
     * @param pixel_index    In/out: current pixel position
     * @param pixel_max      In/out: running max power for current pixel
     * @param bins_hz_acc    In/out: accumulated Hz since last pixel boundary
     * @param pixel_step_hz  Hz per pixel
     * @param f_min          Sweep start frequency (for exception check)
     * @param exception_radius_hz Exclusion radius around exception frequencies
     * @param exceptions     Exception frequency array
     * @param num_exceptions Number of valid exception entries
     * @return Updated pixel_index
     */
    static uint16_t process_frame(
        const ChannelSpectrum& spectrum,
        uint8_t* composite,
        uint16_t& pixel_index,
        uint8_t& pixel_max,
        FreqHz& bins_hz_acc,
        FreqHz pixel_step_hz,
        FreqHz f_min,
        FreqHz exception_radius_hz,
        const FreqHz* exceptions,
        uint8_t num_exceptions
    ) noexcept;

private:
    [[nodiscard]] static bool is_exception_freq(
        FreqHz hz,
        FreqHz exception_radius_hz,
        const FreqHz* exceptions,
        uint8_t num_exceptions
    ) noexcept;
};

} // namespace drone_analyzer

#endif // SWEEP_PROCESSOR_HPP
