/*
 * sl_effect.hpp — EffectPlan + validation (portable, header-only).
 *
 * A parameterized description of a controlled test signal: a waveform, an occupied
 * band, a power level, and a bounded duration. Faithful C++ port of
 * labkit/effects.py (Waveform, EffectPlan, the generator caps, and the finiteness /
 * range checks from EffectPlan.__post_init__).
 *
 * The Python raises ValueError on an invalid plan; on the device we return a
 * ValidationResult (bool + reason) instead — no exceptions, so it suits both the
 * Mayhem firmware and the host test.
 */
#ifndef SENTINEL_LABKIT_SL_EFFECT_HPP
#define SENTINEL_LABKIT_SL_EFFECT_HPP

#include <cmath>

namespace sentinel_labkit {

enum class Waveform { CW, BAND_NOISE, SWEEP };

inline const char* waveform_name(Waveform w) {
    switch (w) {
        case Waveform::CW: return "cw";
        case Waveform::BAND_NOISE: return "band_noise";
        case Waveform::SWEEP: return "sweep";
    }
    return "cw";
}

// Conservative generator caps, independent of authorization. Mirror labkit/effects.py.
static const float MAX_DURATION_S = 60.0f;
static const float MAX_BANDWIDTH_MHZ = 60.0f;
static const float MIN_BANDWIDTH_MHZ = 0.0f;

struct EffectPlan {
    Waveform waveform = Waveform::BAND_NOISE;
    float center_mhz = 0.0f;
    float bandwidth_mhz = 0.0f;
    float power_dbm = 0.0f;
    float duration_s = 0.0f;

    // Occupied span (the *plan* window, center +/- bw/2). NOTE: the GNSS guard is
    // evaluated against the wider emitted footprint (sl_bands::emitted_span_mhz), not
    // this window — see sl_gate.hpp.
    float low_mhz() const { return center_mhz - bandwidth_mhz / 2.0f; }
    float high_mhz() const { return center_mhz + bandwidth_mhz / 2.0f; }
};

struct ValidationResult {
    bool ok;
    const char* reason;  // empty when ok
};

// Validate a plan against the generator caps. Mirrors EffectPlan.__post_init__,
// same checks in the same order (so the same input yields the same verdict).
inline ValidationResult validate(const EffectPlan& p) {
    if (!(std::isfinite(p.center_mhz) && std::isfinite(p.bandwidth_mhz) &&
          std::isfinite(p.power_dbm))) {
        return {false, "center_mhz, bandwidth_mhz, and power_dbm must be finite"};
    }
    if (p.bandwidth_mhz < MIN_BANDWIDTH_MHZ) {
        return {false, "bandwidth_mhz must be >= 0"};
    }
    if (p.bandwidth_mhz > MAX_BANDWIDTH_MHZ) {
        return {false, "bandwidth exceeds generator cap (60 MHz)"};
    }
    if (!(p.duration_s > 0.0f && p.duration_s <= MAX_DURATION_S)) {
        return {false, "duration must be in (0, 60] s"};
    }
    if (p.center_mhz <= 0.0f) {
        return {false, "center_mhz must be positive"};
    }
    return {true, ""};
}

}  // namespace sentinel_labkit

#endif  // SENTINEL_LABKIT_SL_EFFECT_HPP
