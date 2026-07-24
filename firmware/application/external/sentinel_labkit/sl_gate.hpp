/*
 * sl_gate.hpp — the pre-emission gate (portable, header-only).
 *
 * The single choke point every real transmission must pass, mirroring
 * labkit/drivers.py::_gate. A run may key the radio only if ALL hold:
 *   1. the plan is valid (generator caps);
 *   2. the *emitted footprint* (center +/- max(bw, sample_rate)/2) touches NO
 *      GNSS-protected span — the one hard, non-overridable boundary; and
 *   3. a valid operator attestation is present.
 * There are deliberately no other in-code gates (no allowlists, power/band
 * caps-as-gates, or interlocks) — that matches the project's design boundaries.
 *
 * The UI/radio layer MUST call pre_emit_gate() and honor `allowed` before enabling
 * the transmitter. Dry-run validation calls the same function so the on-screen verdict
 * matches what an armed run would do.
 */
#ifndef SENTINEL_LABKIT_SL_GATE_HPP
#define SENTINEL_LABKIT_SL_GATE_HPP

#include "sl_attest.hpp"
#include "sl_bands.hpp"
#include "sl_effect.hpp"

namespace sentinel_labkit {

struct GateResult {
    bool allowed;
    const char* reason;  // "ok" when allowed; the blocking reason otherwise
    bool gnss_blocked;   // true iff the emitted footprint hit a GNSS span
    float emitted_low_mhz;
    float emitted_high_mhz;
};

// Evaluate the gate for a plan at a given sample rate with a given attestation.
// Pure: no side effects, no radio access — safe to call for a live dry-run preview.
inline GateResult pre_emit_gate(const EffectPlan& plan, float sample_rate_hz,
                                const Attestation& att) {
    GateResult r;
    emitted_span_mhz(plan.center_mhz, plan.bandwidth_mhz, sample_rate_hz,
                     r.emitted_low_mhz, r.emitted_high_mhz);

    ValidationResult v = validate(plan);
    const char* hit = gnss_hit(r.emitted_low_mhz, r.emitted_high_mhz);
    r.gnss_blocked = (hit != nullptr);

    if (!v.ok) {
        r.allowed = false;
        r.reason = v.reason;
        return r;
    }
    if (hit != nullptr) {                 // GNSS hard block — never overridable
        r.allowed = false;
        r.reason = hit;
        return r;
    }
    if (!att.valid()) {
        r.allowed = false;
        r.reason = "valid operator attestation required";
        return r;
    }
    r.allowed = true;
    r.reason = "ok";
    return r;
}

}  // namespace sentinel_labkit

#endif  // SENTINEL_LABKIT_SL_GATE_HPP
