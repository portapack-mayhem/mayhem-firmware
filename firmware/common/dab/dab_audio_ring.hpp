/* DAB+ audio hand-over: the codec pulls. The audio DMA's transfer-complete ISR
 * calls render() per free transfer, so the decoder's bursts need only keep two
 * 24 kHz slots (80 ms) from running dry. Single producer (thread), single
 * consumer (ISR), one core: a compiler barrier orders samples before commit. */
#ifndef __DAB_AUDIO_RING_H__
#define __DAB_AUDIO_RING_H__

#include <cstddef>
#include <cstdint>

namespace dab {

class AudioRing {
    static void barrier() { __asm__ volatile("" ::: "memory"); }

   public:
    static constexpr size_t kCore = 960; /* one AAC frame, 40 ms at 24 kHz */
    static constexpr size_t kSlots = 2;
    static constexpr size_t kStorage = kCore * kSlots;

    /* Gain ramp in core samples (2.7 ms): a hole fades out from the last
     * sample and the audio after it fades back in, so neither edge steps. */
    static constexpr int32_t kFadeShift = 6;
    static constexpr int32_t kFade = 1 << kFadeShift;

    void attach(int16_t* storage) { storage_ = storage; }

    /* Baseband thread side. */

    uint32_t queued() const { return written_ - read_; }
    size_t free_slots() const { return kSlots - queued(); }

    int16_t* writable() const {
        if (queued() >= kSlots)
            return nullptr;
        return storage_ + (written_ % kSlots) * kCore;
    }

    /* The slot writable() returned must now hold kCore samples. */
    void commit() {
        barrier();
        written_ = written_ + 1;
    }

    /* Counters the ISR keeps; the thread reads and differences them. */
    uint32_t dry_transfers() const { return dry_transfers_; }
    uint32_t frames_played() const { return read_; }

    /* Audio DMA interrupt side. */

    /* n frames at twice the core rate, same sample on both channels. Even
     * outputs are the core sample, odd ones (-x[-1] + 9 x[0] + 9 x[1] - x[2]) / 16,
     * ~12 dB more image rejection than linear. The run loop is a fast path that
     * must match the per-sample step below. */
    void render(uint32_t* out, size_t n) {
        const uint32_t written = written_;
        uint32_t read = read_;
        uint32_t pos = pos_;
        int32_t gain = gain_, last = last_;
        int32_t d0 = d0_, d1 = d1_, d2 = d2_, d3 = d3_;
        bool dry = false;
        const int16_t* frame = storage_ + (read % kSlots) * kCore;
        uint32_t* const end = out + (n & ~size_t(1));

        while (out != end) {
            if (read != written && gain == kFade) {
                size_t run = size_t(end - out) / 2;
                if (run > kCore - pos)
                    run = kCore - pos;
                const int16_t* src = frame + pos;
                const int16_t* const stop = src + run;
                while (src != stop) {
                    d0 = d1;
                    d1 = d2;
                    d2 = d3;
                    d3 = *src++;
                    out[0] = pack(d1);
                    out[1] = pack(saturate((9 * (d1 + d2) - (d0 + d3) + 8) >> 4));
                    out += 2;
                }
                pos += run;
                last = d3;
                if (pos == kCore) {
                    pos = 0;
                    ++read;
                    frame = storage_ + (read % kSlots) * kCore;
                }
                continue;
            }

            int32_t c;
            if (read != written) {
                const int32_t s = frame[pos];
                if (++pos == kCore) {
                    pos = 0;
                    ++read;
                    frame = storage_ + (read % kSlots) * kCore;
                }
                last = s;
                if (gain < kFade)
                    ++gain;
                c = (s * gain) >> kFadeShift;
            } else {
                dry = true;
                if (gain > 0)
                    --gain;
                c = (last * gain) >> kFadeShift;
            }
            d0 = d1;
            d1 = d2;
            d2 = d3;
            d3 = c;
            out[0] = pack(d1);
            out[1] = pack(saturate((9 * (d1 + d2) - (d0 + d3) + 8) >> 4));
            out += 2;
        }

        read_ = read;
        pos_ = pos;
        gain_ = gain;
        last_ = last;
        d0_ = d0;
        d1_ = d1;
        d2_ = d2;
        d3_ = d3;
        if (dry)
            dry_transfers_ = dry_transfers_ + 1;
    }

    static uint32_t pack(int32_t v) {
        return uint32_t(uint16_t(v)) * 0x00010001u;
    }

    static int32_t saturate(int32_t v) {
#if defined(__ARM_FEATURE_SAT) || defined(__ARM_ARCH_7EM__)
        int32_t r;
        __asm__("ssat %0, #16, %1" : "=r"(r) : "r"(v));
        return r;
#else
        if (v > 32767)
            return 32767;
        if (v < -32768)
            return -32768;
        return v;
#endif
    }

   private:
    int16_t* storage_{nullptr};
    volatile uint32_t written_{0}; /* frames committed, thread-owned */
    volatile uint32_t read_{0};    /* frames consumed, ISR-owned */
    volatile uint32_t dry_transfers_{0};

    /* ISR-only state. */
    uint32_t pos_{0};
    int32_t gain_{0};
    int32_t last_{0};
    int32_t d0_{0}, d1_{0}, d2_{0}, d3_{0};
};

} /* namespace dab */

#endif /*__DAB_AUDIO_RING_H__*/
