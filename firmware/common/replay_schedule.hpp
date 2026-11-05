/* Replay-only CPU-aware scheduling. Capture oversampling is intentionally unchanged.
 * Estimated costs are calibrated offline against GCC 9.2.1 Cortex-M4 code,
 * rounded upward to 1000 cycles plus 4000 cycles for callback/stream overhead.
 * These are selection estimates, not measured worst-case hardware bounds.
 */
#ifndef __REPLAY_SCHEDULE_HPP__
#define __REPLAY_SCHEDULE_HPP__
#include <cstdint>

namespace replay {
struct Schedule {
    uint32_t source_rate{};
    uint32_t output_rate{};
    uint32_t estimated_cycles{};
    uint8_t total_factor{};
    uint8_t fir_stage_count{};
    uint8_t repetition_factor{};
    bool valid() const { return total_factor != 0; }
    uint32_t fir_factor() const { return 1u << fir_stage_count; }
};

// Estimated Cortex-M4 kcycles per 2048 output samples, including conversion,
// repetition and the fixed overhead reserve described above.
// Row r fixes total factor L = 2^r. Column n selects FIR factor M = 2^n
// and post-FIR repetition R = L/M: no FIR / maximum repetition at n=0,
// progressing to maximum FIR / no repetition at n=r.
// Only columns 0..r are used; remaining entries are unused padding.
inline constexpr uint16_t callback_cost_k[7][7] = {
    {50},
    {58, 256},
    {35, 138, 180},
    {23, 90, 105, 156},
    {17, 54, 77, 95, 146},
    {14, 36, 49, 73, 92, 143},
    {10, 27, 37, 49, 74, 92, 143}};

inline bool within_budget(uint32_t cycles, uint32_t output_rate) {
    // cycles <= 0.75 * (2048 / output_rate) * 200000000.
    // Equivalently, cycles * output_rate <= 307200000000, where
    // 307200000000 = 2048 samples/callback * 200000000 cycles/s * 0.75.
    // Configuration-time CPU estimate, not a worst-case execution guarantee.
    // Canonical MID intervals may intentionally exceed this conservative budget.
    return uint64_t(cycles) * output_rate <= 307200000000ULL;
}

// Honor explicit factors used by other Replay clients (e.g. CVS Spam).
inline Schedule for_total(uint32_t source_rate, uint32_t total_factor) {
    if (!source_rate || !total_factor || total_factor > 64 ||
        (total_factor & (total_factor - 1)) ||
        uint64_t(source_rate) * total_factor > 22000000)
        return {};
    // Canonical source-rate/total-factor pairs must use the same FIR/repetition
    // split on M0 and M4. Unmatched pairs use the cost-based selection below.
    if (source_rate >= 25000 && source_rate < 37500 && total_factor == 64)
        return {source_rate, source_rate * total_factor, callback_cost_k[6][5] * 1000u, 64, 5, 2};
    // These MID intervals preserve their FIR/R2 split even when its estimated
    // cost exceeds the conservative 75% model budget.
    if (source_rate >= 250000 && source_rate <= 400000 && total_factor == 8)
        return {source_rate, source_rate * total_factor, callback_cost_k[3][2] * 1000u, 8, 2, 2};
    if (source_rate > 400000 && source_rate <= 600000 && total_factor == 4)
        return {source_rate, source_rate * total_factor, callback_cost_k[2][1] * 1000u, 4, 1, 2};
    unsigned row = 0;
    for (auto f = total_factor; f > 1; f >>= 1) ++row;
    Schedule result{source_rate, source_rate * total_factor, 0,
                    static_cast<uint8_t>(total_factor), 0,
                    static_cast<uint8_t>(total_factor)};
    // Maximum affordable FIR factor; no policy work in the callback.
    for (unsigned n = 0; n <= row; ++n) {
        const uint32_t cost = callback_cost_k[row][n] * 1000u;
        if (within_budget(cost, result.output_rate)) {
            result.fir_stage_count = n;
            result.estimated_cycles = cost;
            result.repetition_factor = total_factor >> n;
        }
    }
    // Even unfiltered explicit configurations may exceed the model budget;
    // preserve their caller-selected rate and expose that estimate.
    if (!result.estimated_cycles) result.estimated_cycles = callback_cost_k[row][0] * 1000u;
    return result;
}

inline Schedule select(uint32_t source_rate) {
    if (!source_rate || source_rate > 5500000) return {};
    uint32_t total;
    if (source_rate >= 25000 && source_rate < 37500) {
        total = 64;
    } else if (source_rate < 250000) {
        total = source_rate < 25000 ? 64 : source_rate < 80000 ? 32
                                                               : 16;
    } else if (source_rate <= 400000) {
        total = 8;
    } else if (source_rate <= 600000) {
        total = 4;
    } else if (source_rate < 1750000) {
        total = 2;
    } else {
        total = 1;
    }
    return for_total(source_rate, total);
}
}  // namespace replay
#endif
