#include <cstdint>
#include <cstddef>
#include "sweep_processor.hpp"
#include "message.hpp"
#include "constants.hpp"

namespace drone_analyzer {

bool SweepProcessor::is_exception_freq(
    FreqHz hz,
    FreqHz exception_radius_hz,
    const FreqHz* exceptions,
    uint8_t num_exceptions
) noexcept {
    for (uint8_t i = 0; i < num_exceptions; ++i) {
        if (exceptions[i] == 0) continue;
        const FreqHz lo = (exceptions[i] > exception_radius_hz)
            ? (exceptions[i] - exception_radius_hz) : 0;
        const FreqHz hi = exceptions[i] + exception_radius_hz;
        if (hz >= lo && hz <= hi) return true;
    }
    return false;
}

uint16_t SweepProcessor::process_frame(
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
) noexcept {
    static constexpr FreqHz EACH_BIN_SIZE = SWEEP_SLICE_BW / 256;

    for (uint8_t bin = 0; bin < SWEEP_PIXELS_PER_SLICE; ++bin) {
        if (pixel_index >= COMPOSITE_SIZE) break;

        if (bin >= UPPER_PIXEL_END && bin >= SWEEP_FFT_MAP_CROSSOVER) continue;

        const uint8_t fft_bin = (bin < SWEEP_FFT_MAP_CROSSOVER)
            ? (SWEEP_FFT_MAP_START + bin)
            : (bin - UPPER_OFFSET);

        const uint8_t power = spectrum.db[fft_bin];
        if (power > pixel_max) pixel_max = power;
        bins_hz_acc += EACH_BIN_SIZE;

        while (bins_hz_acc >= pixel_step_hz && pixel_index < COMPOSITE_SIZE) {
            const FreqHz pixel_freq = f_min + static_cast<FreqHz>(pixel_index) * pixel_step_hz;
            if (!is_exception_freq(pixel_freq, exception_radius_hz, exceptions, num_exceptions)) {
                composite[pixel_index] = pixel_max;
            }
            ++pixel_index;
            pixel_max = 0;
            bins_hz_acc -= pixel_step_hz;
        }
    }

    return pixel_index;
}

} // namespace drone_analyzer
