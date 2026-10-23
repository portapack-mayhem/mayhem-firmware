/*
FSK pulse detector for the weather app.
Turns the IQ samples of an FSK burst into (level, duration) pairs, the same way the OOK path does, so FSK protocols can be written like the OOK ones.
level true = upper frequency (mark), false = lower frequency (space).
Port of the "classic" FSK detector and the FM demodulator from rtl_433 ( https://github.com/merbanan/rtl_433 src/pulse_detect_fsk.c, src/baseband.c ). Thanks for their work!
*/

#ifndef __FPROTO_WEATHERFSK_H__
#define __FPROTO_WEATHERFSK_H__

#include <stdint.h>
#include <stdlib.h>

class WeatherFSKDetect {
   public:
    typedef void (*OutCallback)(void* ctx, bool level, uint32_t duration);

    WeatherFSKDetect(OutCallback cb, void* ctx)
        : out_cb{cb}, out_ctx{ctx} {}

    void set_sample_period(uint32_t ns) { ns_per_sample = ns; }

    // call it on the first sample of a burst
    void reset() {
        state = STATE_INIT;
        run_len = 0;
        pending_len = 0;
        f1_est = 0;
        f2_est = 0;
        fm_x1 = 0;
        fm_y1 = 0;
        last_re = 0;
        last_im = 0;
    }

    // call it for every sample within the burst
    void feed(int16_t re, int16_t im) {
        // instantaneous frequency: arg(s[n] * conj(s[n-1])), PI = INT16_MAX
        re >>= 1;  // keep the products in int32 range
        im >>= 1;
        int32_t pr = (int32_t)re * last_re + (int32_t)im * last_im;
        int32_t pi = (int32_t)im * last_re - (int32_t)re * last_im;
        last_re = re;
        last_im = im;
        int16_t fm_x0 = atan2_int16(pi, pr);
        // 1st order butterworth low pass, cutoff 0.1 * fs: y = 0.72654 * y1 + 0.13673 * (x0 + x1)
        int16_t fm = (int16_t)((23807 * (int32_t)fm_y1 + 4480 * ((int32_t)fm_x0 + fm_x1)) >> 15);
        fm_x1 = fm_x0;
        fm_y1 = fm;
        process(fm);
    }

    // call it when the burst is over, to flush the last pulses
    void end() {
        flush_pending();
        if (state != STATE_INIT && run_len > 0) emit(state == STATE_FH, run_len);
        state = STATE_INIT;
        run_len = 0;
    }

   private:
    static constexpr int32_t FM_DELTA = 6000;  // default estimate for frequency delta (~23kHz @ 250ksps)
    static constexpr int32_t EST_SLOW = 64;
    static constexpr int32_t EST_FAST = 16;
    static constexpr uint32_t MIN_PULSE_SAMPLES = 10;

    enum : uint8_t {
        STATE_INIT = 0,
        STATE_FH,  // at F1, the upper frequency
        STATE_FL,  // at F2, the lower frequency
    } state = STATE_INIT;

    OutCallback out_cb;
    void* out_ctx;
    uint32_t ns_per_sample = 4000;
    uint32_t run_len = 0;
    // last finished run, held back until the next one is long enough, so a spurious one can be merged back into it
    uint32_t pending_len = 0;
    bool pending_level = false;
    int32_t f1_est = 0;
    int32_t f2_est = 0;
    int16_t fm_x1 = 0;
    int16_t fm_y1 = 0;
    int16_t last_re = 0;
    int16_t last_im = 0;

    static int16_t atan2_int16(int32_t y, int32_t x) {
        constexpr int32_t I_PI_4 = INT16_MAX / 4;
        constexpr int32_t I_3_PI_4 = 3 * INT16_MAX / 4;
        if (!x && !y) return 0;
        int32_t abs_y = abs(y);
        // scale down to 17 bits, so the math fits into int32 (no 64 bit division on the M4)
        int32_t sh = 32 - __builtin_clz((uint32_t)(abs_y | abs(x))) - 17;
        if (sh > 0) {
            abs_y >>= sh;
            x >>= sh;
        }
        int32_t angle;
        if (x >= 0) {
            int32_t denom = abs_y + x;
            if (denom == 0) denom = 1;
            angle = I_PI_4 - I_PI_4 * (x - abs_y) / denom;
        } else {
            int32_t denom = abs_y - x;
            if (denom == 0) denom = 1;
            angle = I_3_PI_4 - I_PI_4 * (x + abs_y) / denom;
        }
        return (y < 0) ? -angle : angle;
    }

    void emit(bool level, uint32_t samples) {
        if (out_cb) out_cb(out_ctx, level, samples * ns_per_sample / 1000);
    }

    void flush_pending() {
        if (pending_len) emit(pending_level, pending_len);
        pending_len = 0;
    }

    // a run at the current frequency is over
    void finish_run(bool level) {
        if (run_len >= MIN_PULSE_SAMPLES) {
            flush_pending();
            pending_level = level;
            pending_len = run_len;
            run_len = 0;
        } else {
            // too short, it was a spike. Continue the previous run
            run_len += pending_len;
            pending_len = 0;
        }
    }

    void process(int16_t fm_n) {
        int32_t const f1_delta = abs(fm_n - f1_est);
        int32_t const f2_delta = abs(fm_n - f2_est);
        run_len++;
        switch (state) {
            case STATE_INIT:
                if (run_len < MIN_PULSE_SAMPLES) {
                    f1_est = f1_est / 2 + fm_n / 2;  // quick initial estimator
                } else if (f1_delta > FM_DELTA / 2) {
                    run_len--;
                    if (fm_n > f1_est) {
                        // initial frequency was the lower one
                        pending_level = false;
                        pending_len = run_len;
                        state = STATE_FH;
                        f2_est = f1_est;
                        f1_est = fm_n;
                    } else {
                        pending_level = true;
                        pending_len = run_len;
                        state = STATE_FL;
                        f2_est = fm_n;
                    }
                    run_len = 1;
                } else {
                    f1_est += fm_n / EST_FAST - f1_est / EST_FAST;
                }
                break;
            case STATE_FH:
                if (f1_delta > f2_delta) {
                    run_len--;
                    state = STATE_FL;
                    finish_run(true);
                    run_len++;
                } else if (fm_n > f1_est) {
                    f1_est += fm_n / EST_FAST - f1_est / EST_FAST;
                } else {
                    f1_est += fm_n / EST_SLOW - f1_est / EST_SLOW;
                }
                break;
            case STATE_FL:
                if (f2_delta > f1_delta) {
                    run_len--;
                    state = STATE_FH;
                    finish_run(false);
                    run_len++;
                } else if (fm_n < f2_est) {
                    f2_est += fm_n / EST_FAST - f2_est / EST_FAST;
                } else {
                    f2_est += fm_n / EST_SLOW - f2_est / EST_SLOW;
                }
                break;
        }
    }
};

#endif
