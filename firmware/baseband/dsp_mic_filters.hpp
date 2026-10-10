// Mic TX reconstruction filters: validated coefficients and arithmetic.
#ifndef DSP_MIC_FILTERS_HPP
#define DSP_MIC_FILTERS_HPP

#include <array>
#include <cstddef>

namespace dsp {
namespace interpolate {

// SSB complex FIR interpolation x8: 12 -> 96 ksps.

// 64-tap equiripple, fs=96k, pass <=3k, stop >=9k, equal weights.
// Float32 coefficients; prototype sum=8 (unity reconstructed amplitude).
// phases[p][r] = h[p + 8*r]. No per-phase renormalization.

constexpr std::array<std::array<float, 8>, 8> ssb_x8_phases{{
    // BEGIN COEFFICIENTS
    {{-3.094396088e-04f, 5.664623342e-03f, -1.976640895e-02f, 6.177496538e-02f, 9.880051017e-01f, -4.380666837e-02f, 1.125104632e-02f, -1.924273674e-03f}},
    {{-2.909886185e-03f, 1.574029401e-02f, -5.793647468e-02f, 1.988908947e-01f, 9.335103035e-01f, -1.135775298e-01f, 3.216532245e-02f, -6.517549977e-03f}},
    {{-4.242599942e-03f, 2.692965046e-02f, -9.786115587e-02f, 3.584946394e-01f, 8.305451870e-01f, -1.480258405e-01f, 4.237276688e-02f, -8.317511529e-03f}},
    {{-6.390217692e-03f, 3.701278940e-02f, -1.320423633e-01f, 5.275136232e-01f, 6.902936697e-01f, -1.517064869e-01f, 4.315611720e-02f, -7.986653596e-03f}},
    {{-7.986653596e-03f, 4.315611720e-02f, -1.517064869e-01f, 6.902936697e-01f, 5.275136232e-01f, -1.320423633e-01f, 3.701278940e-02f, -6.390217692e-03f}},
    {{-8.317511529e-03f, 4.237276688e-02f, -1.480258405e-01f, 8.305451870e-01f, 3.584946394e-01f, -9.786115587e-02f, 2.692965046e-02f, -4.242599942e-03f}},
    {{-6.517549977e-03f, 3.216532245e-02f, -1.135775298e-01f, 9.335103035e-01f, 1.988908947e-01f, -5.793647468e-02f, 1.574029401e-02f, -2.909886185e-03f}},
    {{-1.924273674e-03f, 1.125104632e-02f, -4.380666837e-02f, 9.880051017e-01f, 6.177496538e-02f, -1.976640895e-02f, 5.664623342e-03f, -3.094396088e-04f}},
    // END COEFFICIENTS
}};

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

// SSB halfband interpolation x2: 96 -> 192 ksps.

// Seven-tap half-band: [outer, 0, inner, center, inner, 0, outer].
// 192 ksps, pass <=3 kHz, stop >=93 kHz; prototype gain 2.

constexpr float ssb_x2_outer = -6.295384467e-02f;
constexpr float ssb_x2_inner = 5.629527569e-01f;
constexpr float ssb_x2_center = 1.000002146e+00f;

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

namespace am {

// AM microphone FIR decimation /2: 24 -> 12 ksps.

// 35 taps, pass 0..3 kHz, stop 6..12 kHz, unity DC.

constexpr std::array<float, 35> decimate_taps{{
    3.247561108e-04f,
    6.688864960e-05f,
    -1.167249866e-03f,
    -1.671086764e-03f,
    1.140860608e-03f,
    5.263066385e-03f,
    3.226983594e-03f,
    -7.402597461e-03f,
    -1.403451152e-02f,
    -5.172403180e-04f,
    2.512288094e-02f,
    2.643547952e-02f,
    -1.826103404e-02f,
    -6.775522232e-02f,
    -3.785380721e-02f,
    1.075665057e-01f,
    2.915489078e-01f,
    3.759328127e-01f,
    2.915489078e-01f,
    1.075665057e-01f,
    -3.785380721e-02f,
    -6.775522232e-02f,
    -1.826103404e-02f,
    2.643547952e-02f,
    2.512288094e-02f,
    -5.172403180e-04f,
    -1.403451152e-02f,
    -7.402597461e-03f,
    3.226983594e-03f,
    5.263066385e-03f,
    1.140860608e-03f,
    -1.671086764e-03f,
    -1.167249866e-03f,
    6.688864960e-05f,
    3.247561108e-04f,
}};

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

// AM/DSB real FIR interpolation x8: 12 -> 96 ksps.
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

// AM/DSB halfband interpolation x2: 96 -> 192 ksps.
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

// AM microphone processing chain: decimation /2, interpolation x8 and x2.
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
