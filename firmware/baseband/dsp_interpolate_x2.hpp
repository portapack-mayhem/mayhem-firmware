#ifndef DSP_INTERPOLATE_X2_HPP
#define DSP_INTERPOLATE_X2_HPP

#include "dsp_interpolate_x8.hpp"
#include "dsp_interpolate_x2_taps.hpp"

namespace dsp {
namespace interpolate {

// Additional 96 -> 192 ksps stage. The seven-tap prototype has two zero
// coefficients and symmetric even-phase taps. State persists between blocks.
class ComplexFIRInterpolate2 {
   public:
    using Sample = ComplexFIRInterpolate8::Sample;

    void execute(const Sample& input, Sample& even, Sample& odd) {
        even.i = ssb_x2_outer * (input.i + history_[2].i) +
                 ssb_x2_inner * (history_[0].i + history_[1].i);
        even.q = ssb_x2_outer * (input.q + history_[2].q) +
                 ssb_x2_inner * (history_[0].q + history_[1].q);
        odd = {ssb_x2_center * history_[0].i, ssb_x2_center * history_[0].q};
        history_[2] = history_[1];
        history_[1] = history_[0];
        history_[0] = input;
    }

   private:
    std::array<Sample, 3> history_{};
};

}  // namespace interpolate
}  // namespace dsp
#endif
