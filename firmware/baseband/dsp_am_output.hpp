#ifndef DSP_AM_OUTPUT_HPP
#define DSP_AM_OUTPUT_HPP
#include <cmath>
#include <cstdint>
namespace dsp {
namespace am {
// Full-rate beep safety only: deliberately no observer/counter access.
inline int8_t beep_output(float modulation, bool carrier) {
    const float scalar = modulation + (carrier ? 63.0f : 0.0f);
    // Finite carrier addition cannot turn a nonfinite modulation into a finite
    // scalar, so one final finite check covers both values.
    if (!std::isfinite(scalar)) return 0;
    if (scalar > 127.0f) return 127;
    if (scalar < -128.0f) return -128;
    return static_cast<int8_t>(scalar);
}

// Microphone uses the same endpoint/nonfinite protection as the full-rate beep.
inline int8_t output(float modulation, bool carrier) {
    return beep_output(modulation, carrier);
}
}  // namespace am
}  // namespace dsp
#endif
