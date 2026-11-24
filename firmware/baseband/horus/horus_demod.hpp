/*
 * Copyright (C) 2026 OE1LBS
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/*
 * Non-coherent 4FSK demodulator for Horus Binary (100 Bd, ~270 Hz tone spacing).
 *
 * Input is complex baseband at kFs. The four tones are located with an averaged
 * spectrum and a comb search (with per-tone refinement, since the RS41 Si4032 does
 * not space its tones exactly evenly). Each tone is then mixed to DC and integrated
 * over one symbol with a sliding sum; the symbol clock is recovered from the peak of
 * the strongest-tone energy, which is sharpest when the integration window lines up
 * with a symbol.
 *
 * Memory: no dynamic allocation of its own; it lives inside HorusRxProcessor, which
 * is about 28 KB in total (front end, demodulator and both framers). Float only
 * (Cortex-M4F).
 */

#ifndef __HORUS_DEMOD_H__
#define __HORUS_DEMOD_H__

#include <cstdint>
#include <cstddef>

namespace horus {

struct cf32 {
    float re;
    float im;
};

constexpr uint32_t kFs = 12000;   // input sample rate (complex)
constexpr uint32_t kBaud = 100;   // Horus Binary symbol rate
constexpr int kTs = kFs / kBaud;  // samples per symbol
constexpr int kTones = 4;
constexpr int kFftN = 512;            // 23.4 Hz bins at 12 kHz
constexpr float kSearchHz = 5000.0f;  // tones must lie within +-kSearchHz

struct DemodStatus {
    float tone_hz[kTones];  // estimated tone frequencies relative to the tuned frequency
    float snr_db;           // average strongest-tone vs other-tones energy ratio over recent symbols
    float signal_db;        // averaged comb power over noise floor in the spectrum
    bool tones_valid;       // a 4-tone signal has been seen at least once
};

class Demod4FSK {
   public:
    Demod4FSK();
    void reset();
    void set_tone_spacing(float hz) { tone_spacing_hz_ = hz; }

    // Feed one sample. Returns the demodulated symbol (0..3, lowest tone = 0) once
    // per symbol period, otherwise -1. Cheap and constant cost per sample.
    int process(cf32 x);

    // Advance the spectrum / tone estimation job by one bounded slice of work.
    // Call it regularly (once per baseband buffer); a full estimate takes
    // kSpectrumSteps calls and one is started every kFftN samples.
    void background_step();
    static constexpr int kSpectrumSteps = 13;

    const DemodStatus& status() const { return status_; }

   private:
    void comb_search();
    void refine_tones();
    void set_tone(int k, float hz);
    int decide();

    // spectrum / frequency estimation
    cf32 fft_in_[kFftN];    // being filled by process()
    cf32 fft_work_[kFftN];  // windowed copy, transformed in place by background_step()
    float window_[kFftN];
    cf32 twiddle_[kFftN / 2];
    float spec_[kFftN];  // exponentially averaged power spectrum (shifted order)
    int fft_fill_ = 0;
    int job_step_ = -1;  // -1: idle
    int comb_best_ = 0;
    float tone_spacing_hz_ = 270.0f;

    // per-tone mixers and sliding symbol integrators
    cf32 osc_[kTones];
    cf32 step_[kTones];
    cf32 ring_[kTones][kTs];
    cf32 acc_[kTones];
    int ring_pos_ = 0;

    // timing recovery: per-sample metric and decisions over the last symbol
    float metric_[kTs];
    uint8_t sym_[kTs];
    float e_best_[kTs];
    float e_rest_[kTs];
    int hist_pos_ = 0;
    int samples_to_eval_ = kTs;
    float tpos_ = 0.5f * kTs;

    // SNR bookkeeping
    float snr_sig_ = 0.0f;
    float snr_noise_ = 1e-12f;

    DemodStatus status_{};
};

}  // namespace horus

#endif /*__HORUS_DEMOD_H__*/
