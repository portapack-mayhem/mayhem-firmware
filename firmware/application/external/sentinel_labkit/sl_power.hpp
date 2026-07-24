/*
 * sl_power.hpp — relative power -> TX gain / amplitude (portable, header-only).
 *
 * On the PortaPack the emitted level is set by the HackRF TX VGA gain (0..47 dB, 1 dB
 * steps) and optionally a baseband amplitude scale. This ports the intent of
 * HackRFDriver's power path in labkit/drivers.py: plan.power_dbm actually drives the
 * emitted RF (it is not a label). Absolute dBm still requires bench calibration; this
 * is a documented *relative* map so a power sweep produces genuinely different RF.
 */
#ifndef SENTINEL_LABKIT_SL_POWER_HPP
#define SENTINEL_LABKIT_SL_POWER_HPP

#include <cmath>

namespace sentinel_labkit {

static const int TX_GAIN_MIN = 0;
static const int TX_GAIN_MAX = 47;   // HackRF TX VGA range (dB)

// Map power_dbm to a TX VGA gain (dB), clamped to [0, 47]. Linear 1 dB : 1 dB with a
// +20 dB offset so a typical -20..+27 dBm sweep spans the full VGA range. The operator
// calibrates absolute output on the bench; the *relative* steps are faithful.
inline int power_dbm_to_tx_gain(float power_dbm) {
    long g = std::lround(power_dbm + 20.0f);
    if (g < TX_GAIN_MIN) g = TX_GAIN_MIN;
    if (g > TX_GAIN_MAX) g = TX_GAIN_MAX;
    return static_cast<int>(g);
}

// Baseband amplitude scale (0..1) relative to full_scale_dbm, mirroring
// HackRFDriver._power_scale's default (no calibration map) path. Powers above the
// reference clip to full scale (a DAC cannot exceed it).
inline float amplitude_scale(float power_dbm, float full_scale_dbm) {
    float amp = std::pow(10.0f, (power_dbm - full_scale_dbm) / 20.0f);
    return amp > 1.0f ? 1.0f : amp;
}

}  // namespace sentinel_labkit

#endif  // SENTINEL_LABKIT_SL_POWER_HPP
