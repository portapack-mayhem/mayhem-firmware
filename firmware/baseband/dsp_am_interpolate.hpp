#ifndef DSP_AM_INTERPOLATE_HPP
#define DSP_AM_INTERPOLATE_HPP

#include "dsp_am_decimate_taps.hpp"
#include "dsp_interpolate_x8_taps.hpp"
#include "dsp_interpolate_x2_taps.hpp"
#include <array>
#include <cstddef>

namespace dsp {
namespace am {
// Dedicated real filters: the validated complex SSB implementation is untouched.
class Decimate2 {
   public:
    // Push both codec samples; retain the output aligned with the second one.
    float execute(float first, float second) {
        push(first);
        push(second);
        float y = decimate_taps[17] * history_[(head_ + 35 - 17) % 35];
        for (size_t k = 0; k < 17; ++k)
            y += decimate_taps[k] * (history_[(head_ + 35 - k) % 35] + history_[(head_ + 1 + k) % 35]);
        return y;
    }

   private:
    void push(float x) {
        head_ = (head_ + 1) % 35;
        history_[head_] = x;
    }
    std::array<float, 35> history_{};
    size_t head_{34};
};

class Interpolate8 {
   public:
    void execute(float x, std::array<float, 8>& out) {
        head_ = (head_ + 1) & 7;
        history_[head_] = x;
        for (size_t p = 0; p < 8; ++p) {
            float y = 0;
            for (size_t k = 0; k < 8; ++k)
                y += interpolate::ssb_x8_phases[p][k] * history_[(head_ - k) & 7];
            out[p] = y;
        }
    }

   private:
    std::array<float, 8> history_{};
    size_t head_{7};
};

class Interpolate2 {
   public:
    void execute(float x, float& even, float& odd) {
        even = interpolate::ssb_x2_outer * (x + history_[2]) + interpolate::ssb_x2_inner * (history_[0] + history_[1]);
        odd = interpolate::ssb_x2_center * history_[0];
        history_[2] = history_[1];
        history_[1] = history_[0];
        history_[0] = x;
    }

   private:
    std::array<float, 3> history_{};
};

class Microphone {
   public:
    void execute(float first, float second, float gain, std::array<float, 16>& out) {
        std::array<float, 8> intermediate;
        x8_.execute(decimator_.execute(first, second) * gain, intermediate);
        for (size_t p = 0; p < 8; ++p)
            x2_.execute(intermediate[p], out[2 * p], out[2 * p + 1]);
    }

   private:
    Decimate2 decimator_{};
    Interpolate8 x8_{};
    Interpolate2 x2_{};
};
}  // namespace am
}  // namespace dsp
#endif
