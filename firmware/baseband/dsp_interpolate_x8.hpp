#ifndef DSP_INTERPOLATE_X8_HPP
#define DSP_INTERPOLATE_X8_HPP

#include "dsp_interpolate_x8_taps.hpp"
#include <cstddef>

namespace dsp {
namespace interpolate {

// Causal 12 -> 96 ksps complex reconstruction. State belongs to the SSB
// modulator, not a TX buffer. Identical coefficients/delay for both components.
class ComplexFIRInterpolate8 {
   public:
    struct Sample {
        float i;
        float q;
    };
    using Output = std::array<Sample, 8>;

    void execute(float i, float q, Output& output) {
        head_ = (head_ + 1) & 7;
        history_[head_] = {i, q};
        for (size_t phase = 0; phase < 8; ++phase) {
            float out_i = 0.0f;
            float out_q = 0.0f;
            for (size_t tap = 0; tap < 8; ++tap) {
                const auto& sample = history_[(head_ - tap) & 7];
                const float coefficient = ssb_x8_phases[phase][tap];
                out_i += coefficient * sample.i;
                out_q += coefficient * sample.q;
            }
            output[phase] = {out_i, out_q};
        }
    }

   private:
    std::array<Sample, 8> history_{};
    size_t head_{7};
};

}  // namespace interpolate
}  // namespace dsp
#endif
