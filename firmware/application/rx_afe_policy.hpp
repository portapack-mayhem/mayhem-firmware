// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef __RX_AFE_POLICY_H__
#define __RX_AFE_POLICY_H__

#include <cstdint>

namespace rx_afe {
enum class NarrowbandPolicy {
    Auto,
    ForceBypass,
    ForceNarrow,
};

// Eligibility limit for FULL requested complex bandwidth (negative + positive
// frequency extent), not the external filter's physical cutoff specification.
constexpr uint32_t narrowband_max_requested_bandwidth_hz = 1'750'000;

struct CapturePolicy {
    uint32_t useful_bandwidth_hz;
    bool disable_quarter_shift;
    NarrowbandPolicy narrowband;
};

// Decide from the unshifted bandwidth only; F and the planned shift are not inputs.
constexpr CapturePolicy capture_policy(uint32_t sample_rate, bool enabled) {
    const auto bandwidth = static_cast<uint32_t>((uint64_t{sample_rate} * 3) / 4);
    const bool eligible = bandwidth <= narrowband_max_requested_bandwidth_hz;
    return {bandwidth, enabled && eligible,
            enabled ? (eligible ? NarrowbandPolicy::ForceNarrow : NarrowbandPolicy::ForceBypass)
                    : NarrowbandPolicy::Auto};
}
}  // namespace rx_afe

#endif
