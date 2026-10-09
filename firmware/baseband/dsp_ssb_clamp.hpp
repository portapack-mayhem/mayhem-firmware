#ifndef DSP_SSB_CLAMP_HPP
#define DSP_SSB_CLAMP_HPP

#include <cmath>

namespace dsp {
namespace modulate {

// Called once per reconstructed component, before the existing C8 conversion.
inline float ssb_clamp(float raw, float scaled) {
    if (!std::isfinite(raw) || !std::isfinite(scaled)) {
        return 0.0f;  // Separate nonfinite fallback, not a finite endpoint clamp.
    }

    if (scaled > 127.0f) {
        return 127.0f;
    }
    if (scaled < -128.0f) {
        return -128.0f;
    }
    return scaled;
}

}  // namespace modulate
}  // namespace dsp
#endif
