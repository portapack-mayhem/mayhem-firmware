#include "proc_flex.hpp"
#include "event_m4.hpp"
#include "audio_dma.hpp"
#include "pocsag.hpp"
#include "dsp_fir_taps.hpp"
#include "portapack_shared_memory.hpp"

#include <cmath>
#include <cstring>

// BCD character table for FLEX numeric messages (index 0-15)
static const char flex_bcd[] = "0123456789.U -][";

// Lightweight string helpers (no snprintf/heap on bare-metal M4)
namespace {

char* str_append(char* dst, const char* end, const char* src) {
    while (*src && dst < end - 1) *dst++ = *src++;
    *dst = '\0';
    return dst;
}

char* str_uint(char* dst, const char* end, uint32_t val, int min_digits = 1) {
    char tmp[11];
    int i = 0;
    if (val == 0) {
        tmp[i++] = '0';
    } else {
        while (val > 0) {
            tmp[i++] = '0' + (val % 10);
            val /= 10;
        }
    }
    while (i < min_digits) tmp[i++] = '0';
    for (int j = i - 1; j >= 0 && dst < end - 1; j--) *dst++ = tmp[j];
    *dst = '\0';
    return dst;
}

char* str_hex(char* dst, const char* end, uint32_t val, int digits) {
    static const char hex[] = "0123456789ABCDEF";
    for (int i = digits - 1; i >= 0 && dst < end - 1; i--)
        *dst++ = hex[(val >> (i * 4)) & 0xF];
    *dst = '\0';
    return dst;
}

}  // namespace

// Constants from demod_flex.c
#define FREQ_SAMP 24000  // Our sample rate
#define DC_OFFSET_FILTER 0.010
#define PHASE_LOCKED_RATE 0.045
#define PHASE_UNLOCKED_RATE 0.050
#define LOCK_LEN 24
#define IDLE_THRESHOLD 0
#define DEMOD_TIMEOUT 100
#define FLEX_SYNC_MARKER 0xA6C6AAAAul
#define SLICE_THRESHOLD 0.667

// Implement EccContainer here to avoid linking pocsag.cpp which pulls in app headers
using namespace pocsag;

EccContainer::EccContainer() {
    setup_ecc();
}

void EccContainer::setup_ecc() {
    unsigned int srr = 0x3b4;
    unsigned int i, n, j, k;

    for (i = 0; i <= 20; i++) {
        ecs[i] = srr;
        if ((srr & 0x01) != 0)
            srr = (srr >> 1) ^ 0x3B4;
        else
            srr = srr >> 1;
    }

    for (i = 0; i < 1024; i++) bch[i] = 0;

    for (n = 0; n <= 20; n++) {
        for (i = 0; i <= 20; i++) {
            j = (i << 5) + n;
            k = ecs[n] ^ ecs[i];
            bch[k] = j + 0x2000;
        }
    }

    for (n = 0; n <= 20; n++) {
        k = ecs[n];
        j = n + (0x1f << 5);
        bch[k] = j + 0x1000;
    }

    for (n = 0; n <= 20; n++) {
        for (i = 0; i < 10; i++) {
            k = ecs[n] ^ (1 << i);
            j = n + (0x1f << 5);
            bch[k] = j + 0x2000;
        }
    }

    for (n = 0; n < 10; n++) {
        k = 1 << n;
        bch[k] = 0x3ff + 0x1000;
    }

    for (n = 0; n < 10; n++) {
        for (i = 0; i < 10; i++) {
            if (i != n) {
                k = (1 << n) ^ (1 << i);
                bch[k] = 0x3ff + 0x2000;
            }
        }
    }
}

int EccContainer::error_correct(uint32_t& val) {
    int i, synd, errl, acc, pari, ecc, b1, b2;

    errl = 0;
    pari = 0;

    ecc = 0;
    for (i = 31; i >= 11; --i) {
        if (val & (1 << i)) {
            ecc = ecc ^ ecs[31 - i];
            pari = pari ^ 0x01;
        }
    }

    acc = 0;
    for (i = 10; i >= 1; --i) {
        acc = acc << 1;
        if (val & (1 << i)) {
            acc = acc ^ 0x01;
        }
    }

    synd = ecc ^ acc;
    errl = 0;

    if (synd != 0) {
        if (bch[synd] != 0) {
            b1 = bch[synd] & 0x1f;
            b2 = bch[synd] >> 5;
            b2 = b2 & 0x1f;

            if (b2 != 0x1f) {
                val ^= 0x01 << (31 - b2);
                ecc = ecc ^ ecs[b2];
            }

            if (b1 != 0x1f) {
                val ^= 0x01 << (31 - b1);
                ecc = ecc ^ ecs[b1];
            }

            errl = bch[synd] >> 12;
        } else {
            errl = 3;
        }

        if (errl == 1) pari = pari ^ 0x01;
    }

    if (errl == 4) errl = 3;

    return errl;
}

namespace {

// Helpers
unsigned int popcount(unsigned int n) {
    // Simple popcount for 32-bit integer
    n = n - ((n >> 1) & 0x55555555);
    n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
    return (((n + (n >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

uint32_t bit_reverse_32(uint32_t x) {
    x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
    x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
    x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
    x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
    x = (x >> 16) | (x << 16);
    return x;
}

}  // namespace

void FlexProcessor::send_debug(const char* text, uint32_t v1, uint32_t v2) {
    if (shared_memory.application_queue.is_empty()) return;
    FlexDebugMessage message(v1, v2, text);
    shared_memory.application_queue.push(message);
}

void FlexProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    // Heartbeat debug every ~1 second (24000Hz / 4096 buffer size * ~6)
    static int debug_count = 0;
    debug_count++;
    if (debug_count > 1000) {
        send_debug("Running", 0, 0);
        debug_count = 0;
    }

    // Decimate and demodulate: 3.072MHz -> 24kHz
    auto decim_0_out = decim_0_iq.execute(buffer, dst_buffer);
    auto decim_1_out = decim_1_iq.execute(decim_0_out, dst_buffer);
    auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);
    auto audio = demod.execute(channel_out, audio_buffer);

    process_audio(audio);
}

void FlexProcessor::process_audio(const buffer_f32_t& audio) {
    for (size_t i = 0; i < audio.count; ++i) {
        flex_demodulate(audio.p[i]);
    }
}

void FlexProcessor::flex_demodulate(double sample) {
    if (build_symbol(sample) == 1) {
        demodulator.nonconsec = 0;
        demodulator.symbol_count++;
        // modulation.symbol_rate = ... // Unused in main logic usually, just stats

        /*Determine the modal symbol*/
        int j;
        int decmax = 0;
        int modal_symbol = 0;
        for (j = 0; j < 4; j++) {
            if (demodulator.symcount[j] > decmax) {
                modal_symbol = j;
                decmax = demodulator.symcount[j];
            }
        }
        demodulator.symcount[0] = 0;
        demodulator.symcount[1] = 0;
        demodulator.symcount[2] = 0;
        demodulator.symcount[3] = 0;

        if (demodulator.locked) {
            /*Process the symbol*/
            flex_sym(modal_symbol);
        } else {
            /*Check for lock pattern*/
            /*Shift symbols into buffer, symbols are converted so that the max and min symbols map to 1 and 2 i.e each contain a single 1 */
            demodulator.lock_buf = (demodulator.lock_buf << 2) | (modal_symbol ^ 0x1);
            uint64_t lock_pattern = demodulator.lock_buf ^ 0x6666666666666666ull;
            uint64_t lock_mask = (1ull << (2 * LOCK_LEN)) - 1;

            if ((lock_pattern & lock_mask) == 0 || ((~lock_pattern) & lock_mask) == 0) {
                demodulator.locked = 1;
                demodulator.lock_buf = 0;
                demodulator.symbol_count = 0;
                demodulator.sample_count = 0;
            }
        }

        /*Time out after X periods with no zero crossing*/
        demodulator.timeout++;
        if (demodulator.timeout > DEMOD_TIMEOUT) {
            demodulator.locked = 0;
        }
    }
}

int FlexProcessor::build_symbol(double sample) {
    const int64_t phase_max = 100 * demodulator.sample_freq;
    const int64_t phase_rate = phase_max * demodulator.baud / demodulator.sample_freq;
    const double phasepercent = 100.0 * demodulator.phase / phase_max;

    demodulator.sample_count++;

    /*Remove DC offset (FIR filter)*/
    if (state.Current == flex::State::SYNC1) {
        modulation.zero = (modulation.zero * (FREQ_SAMP * DC_OFFSET_FILTER) + sample) / ((FREQ_SAMP * DC_OFFSET_FILTER) + 1);
    }
    sample -= modulation.zero;

    if (demodulator.locked) {
        if (state.Current == flex::State::SYNC1) {
            demodulator.envelope_sum += std::abs(sample);
            demodulator.envelope_count++;
            modulation.envelope = demodulator.envelope_sum / demodulator.envelope_count;
        }
    } else {
        modulation.envelope = 0;
        demodulator.envelope_sum = 0;
        demodulator.envelope_count = 0;
        demodulator.baud = 1600;
        demodulator.timeout = 0;
        demodulator.nonconsec = 0;
        state.Current = flex::State::SYNC1;
    }

    /* MID 80% SYMBOL PERIOD */
    if (phasepercent > 10 && phasepercent < 90) {
        if (sample > 0) {
            if (sample > modulation.envelope * SLICE_THRESHOLD)
                demodulator.symcount[3]++;
            else
                demodulator.symcount[2]++;
        } else {
            if (sample < -modulation.envelope * SLICE_THRESHOLD)
                demodulator.symcount[0]++;
            else
                demodulator.symcount[1]++;
        }
    }

    /* ZERO CROSSING */
    if ((demodulator.sample_last < 0 && sample >= 0) || (demodulator.sample_last >= 0 && sample < 0)) {
        double phase_error = 0.0;
        if (phasepercent < 50) {
            phase_error = demodulator.phase;
        } else {
            phase_error = demodulator.phase - phase_max;
        }

        if (demodulator.locked) {
            demodulator.phase -= phase_error * PHASE_LOCKED_RATE;
        } else {
            demodulator.phase -= phase_error * PHASE_UNLOCKED_RATE;
        }

        if (phasepercent > 10 && phasepercent < 90) {
            demodulator.nonconsec++;
            if (demodulator.nonconsec > 20 && demodulator.locked) {
                demodulator.locked = 0;
            }
        } else {
            demodulator.nonconsec = 0;
        }

        demodulator.timeout = 0;
    }
    demodulator.sample_last = sample;

    /* END OF SYMBOL PERIOD */
    demodulator.phase += phase_rate;

    if (demodulator.phase > phase_max) {
        demodulator.phase -= phase_max;
        return 1;
    } else {
        return 0;
    }
}

unsigned int FlexProcessor::flex_sync(unsigned char sym) {
    int retval = 0;
    sync.syncbuf = (sync.syncbuf << 1) | ((sym < 2) ? 1 : 0);

    retval = flex_sync_check(sync.syncbuf);
    if (retval != 0) {
        sync.polarity = 0;
    } else {
        retval = flex_sync_check(~sync.syncbuf);
        if (retval != 0) {
            sync.polarity = 1;
        }
    }
    return retval;
}

unsigned int FlexProcessor::flex_sync_check(uint64_t buf) {
    // 64-bit FLEX sync code: AAAA:BBBBBBBB:CCCC
    unsigned int marker = (buf & 0x0000FFFFFFFF0000ULL) >> 16;
    unsigned short codehigh = (buf & 0xFFFF000000000000ULL) >> 48;
    unsigned short codelow = ~(buf & 0x000000000000FFFFULL);

    int retval = 0;
    // Hamming distance check (popcount of XOR)
    unsigned int diff_marker = popcount(marker ^ FLEX_SYNC_MARKER);
    unsigned int diff_code = popcount(codelow ^ codehigh);

    if (diff_marker < 4 && diff_code < 4) {
        retval = codehigh;
    } else {
        retval = 0;
    }
    return retval;
}

void FlexProcessor::decode_mode(unsigned int sync_code) {
    struct FlexModeDef {
        int sync;
        unsigned int baud;
        unsigned int levels;
    } flex_modes[] = {
        {0x870C, 1600, 2},
        {0xB068, 1600, 4},
        {0x7B18, 3200, 2},
        {0xDEA0, 3200, 4},
        {0x4C7C, 3200, 4},
        {0, 0, 0}};

    for (int i = 0; flex_modes[i].sync != 0; i++) {
        unsigned int diff = popcount((unsigned int)flex_modes[i].sync ^ sync_code);
        if (diff < 4) {
            sync.sync = sync_code;
            sync.baud = flex_modes[i].baud;
            sync.levels = flex_modes[i].levels;
            return;
        }
    }
    // Default
    sync.baud = 1600;
    sync.levels = 2;
}

void FlexProcessor::read_2fsk(unsigned int sym, uint32_t* dat) {
    *dat = (*dat >> 1) | ((sym > 1) ? 0x80000000 : 0);
}

int FlexProcessor::bch_fix_errors(uint32_t* data_to_fix) {
    // Reverse bits for EccContainer (POCSAG MSB-first expectation vs FLEX LSB-first in our representation)
    uint32_t reversed = bit_reverse_32(*data_to_fix);
    int result = ecc.error_correct(reversed);
    if (result == 0 || result == 1 || result == 2) {
        *data_to_fix = bit_reverse_32(reversed);
    }
    return result;
}

int FlexProcessor::decode_fiw() {
    uint32_t fiw_val = fiw.rawdata;
    int decode_error = bch_fix_errors(&fiw_val);

    if (decode_error > 2) {
        return 1;
    }

    fiw.checksum = fiw_val & 0xF;
    fiw.cycleno = (fiw_val >> 4) & 0xF;
    fiw.frameno = (fiw_val >> 8) & 0x7F;
    fiw.roaming = (fiw_val >> 15) & 0x01;
    fiw.repeat = (fiw_val >> 16) & 0x01;
    fiw.traffic = (fiw_val >> 17) & 0x0F;

    unsigned int checksum = (fiw_val & 0xF);
    checksum += ((fiw_val >> 4) & 0xF);
    checksum += ((fiw_val >> 8) & 0xF);
    checksum += ((fiw_val >> 12) & 0xF);
    checksum += ((fiw_val >> 16) & 0xF);
    checksum += ((fiw_val >> 20) & 0x01);
    checksum &= 0xF;

    if (checksum == 0xF) {
        return 0;
    } else {
        return 1;
    }
}

int FlexProcessor::read_data(unsigned char sym) {
    int bit_a = (sym > 1);
    int bit_b = 0;
    if (sync.levels == 4) {
        bit_b = (sym == 1) || (sym == 2);
    }

    if (sync.baud == 1600) {
        data.phase_toggle = 0;
    }

    unsigned int idx = ((data.data_bit_counter >> 5) & 0xFFF8) | (data.data_bit_counter & 0x0007);
    if (idx >= 88) return 0;  // Boundary check

    if (data.phase_toggle == 0) {
        data.PhaseA.buf[idx] = (data.PhaseA.buf[idx] >> 1) | (bit_a ? 0x80000000 : 0);
        data.PhaseB.buf[idx] = (data.PhaseB.buf[idx] >> 1) | (bit_b ? 0x80000000 : 0);
        data.phase_toggle = 1;

        if ((data.data_bit_counter & 0xFF) == 0xFF) {
            if (data.PhaseA.buf[idx] == 0x00000000 || data.PhaseA.buf[idx] == 0xffffffff) data.PhaseA.idle_count++;
            if (data.PhaseB.buf[idx] == 0x00000000 || data.PhaseB.buf[idx] == 0xffffffff) data.PhaseB.idle_count++;
        }
    } else {
        data.PhaseC.buf[idx] = (data.PhaseC.buf[idx] >> 1) | (bit_a ? 0x80000000 : 0);
        data.PhaseD.buf[idx] = (data.PhaseD.buf[idx] >> 1) | (bit_b ? 0x80000000 : 0);
        data.phase_toggle = 0;

        if ((data.data_bit_counter & 0xFF) == 0xFF) {
            if (data.PhaseC.buf[idx] == 0x00000000 || data.PhaseC.buf[idx] == 0xffffffff) data.PhaseC.idle_count++;
            if (data.PhaseD.buf[idx] == 0x00000000 || data.PhaseD.buf[idx] == 0xffffffff) data.PhaseD.idle_count++;
        }
    }

    if (sync.baud == 1600 || data.phase_toggle == 0) {
        data.data_bit_counter++;
    }

    int idle = 0;
    if (sync.baud == 1600) {
        if (sync.levels == 2) {
            idle = (data.PhaseA.idle_count > IDLE_THRESHOLD);
        } else {
            idle = ((data.PhaseA.idle_count > IDLE_THRESHOLD) && (data.PhaseB.idle_count > IDLE_THRESHOLD));
        }
    } else {
        if (sync.levels == 2) {
            idle = ((data.PhaseA.idle_count > IDLE_THRESHOLD) && (data.PhaseC.idle_count > IDLE_THRESHOLD));
        } else {
            idle = ((data.PhaseA.idle_count > IDLE_THRESHOLD) && (data.PhaseB.idle_count > IDLE_THRESHOLD) && (data.PhaseC.idle_count > IDLE_THRESHOLD) && (data.PhaseD.idle_count > IDLE_THRESHOLD));
        }
    }
    return idle;
}

void FlexProcessor::flex_sym(unsigned char sym) {
    unsigned char sym_rectified;
    if (sync.polarity) {
        sym_rectified = 3 - sym;
    } else {
        sym_rectified = sym;
    }

    switch (state.Current) {
        case flex::State::SYNC1: {
            unsigned int sync_code = flex_sync(sym);
            if (sync_code != 0) {
                decode_mode(sync_code);
                if (sync.baud != 0 && sync.levels != 0) {
                    state.Current = flex::State::FIW;
                    send_debug("SYNC1 Found", sync.baud, sync_code);
                } else {
                    state.Current = flex::State::SYNC1;
                }
            } else {
                state.Current = flex::State::SYNC1;
            }
            state.fiwcount = 0;
            fiw.rawdata = 0;
            break;
        }
        case flex::State::FIW: {
            state.fiwcount++;
            if (state.fiwcount >= 16) {
                read_2fsk(sym_rectified, &fiw.rawdata);
            }
            if (state.fiwcount == 48) {
                if (decode_fiw() == 0) {
                    state.sync2_count = 0;
                    state.sync2_shiftreg = 0;
                    state.sync2_c_pos = -1;
                    state.sync2_cinv_pos = -1;
                    demodulator.baud = sync.baud;
                    state.Current = flex::State::SYNC2;
                    send_debug("FIW OK", fiw.frameno, fiw.cycleno);
                } else {
                    state.Current = flex::State::SYNC1;
                    send_debug("FIW Fail", fiw.rawdata, 0);
                }
            }
            break;
        }
        case flex::State::SYNC2: {
            /* S2 structure: BS2 + C(16 bits) + inv.BS2 + inv.C(16 bits)
             * Total duration: 25ms at the data symbol rate.
             *
             * We scan for the 16-bit C pattern (0xED84) using a shift
             * register.  If found, we validate timing.  If not found,
             * we fall back to the nominal 25ms skip (current behavior).
             *
             * Only the MSB (bit_a) matters for C detection - it's a
             * 2-level pattern even in 4FSK modes. */
            unsigned char s2_sym = sync.polarity ? (3 - sym) : sym;
            int bit_a = (s2_sym > 1) ? 1 : 0;
            state.sync2_shiftreg = (state.sync2_shiftreg << 1) | bit_a;
            state.sync2_count++;

            /* Check for C pattern match (Hamming distance <= 2) */
            if (state.sync2_count >= 16) {
                uint16_t diff_c = state.sync2_shiftreg ^ 0xED84;
                uint16_t diff_cinv = state.sync2_shiftreg ^ 0x127B;
                int errs_c = __builtin_popcount(diff_c);
                int errs_cinv = __builtin_popcount(diff_cinv);

                if (errs_c <= 2 && state.sync2_c_pos < 0)
                    state.sync2_c_pos = (int)state.sync2_count;
                if (errs_cinv <= 2 && state.sync2_cinv_pos < 0)
                    state.sync2_cinv_pos = (int)state.sync2_count;
            }

            /* Nominal S2 duration in symbols */
            unsigned int s2_nominal = sync.baud * 25 / 1000;

            /* Data starts after inv.C ends.  If we detected inv.C,
             * use its position as the true data boundary.  Otherwise
             * fall back to the nominal count. */
            unsigned int s2_end = s2_nominal;
            if (state.sync2_cinv_pos > 0) {
                unsigned int cinv_end = (unsigned int)state.sync2_cinv_pos;
                int diff = (int)cinv_end - (int)s2_nominal;
                if (diff >= -1 && diff <= 1)
                    s2_end = cinv_end;
            }

            if (state.sync2_count == s2_end) {
                // Clear phase data
                for (int i = 0; i < 88; i++) {
                    data.PhaseA.buf[i] = 0;
                    data.PhaseB.buf[i] = 0;
                    data.PhaseC.buf[i] = 0;
                    data.PhaseD.buf[i] = 0;
                }
                data.PhaseA.idle_count = 0;
                data.PhaseB.idle_count = 0;
                data.PhaseC.idle_count = 0;
                data.PhaseD.idle_count = 0;
                data.phase_toggle = 0;
                data.data_bit_counter = 0;
                state.data_count = 0;

                state.sync2_shiftreg = 0;
                state.sync2_c_pos = -1;
                state.sync2_cinv_pos = -1;

                state.Current = flex::State::DATA;
            }
            /* Safety: don't get stuck past nominal */
            if (state.sync2_count > s2_nominal + 1) {
                state.sync2_shiftreg = 0;
                state.sync2_c_pos = -1;
                state.sync2_cinv_pos = -1;
                state.Current = flex::State::SYNC1;
            }
            break;
        }
        case flex::State::DATA: {
            int idle = read_data(sym_rectified);
            if (++state.data_count == sync.baud * 1760 / 1000 || idle) {
                decode_data();
                demodulator.baud = 1600;
                state.Current = flex::State::SYNC1;
                state.data_count = 0;
            }
            break;
        }
    }
}

void FlexProcessor::decode_data() {
    if (sync.baud == 1600) {
        if (sync.levels == 2) {
            decode_phase('A');
        } else {
            decode_phase('A');
            decode_phase('B');
        }
    } else {
        if (sync.levels == 2) {
            decode_phase('A');
            decode_phase('C');
        } else {
            decode_phase('A');
            decode_phase('B');
            decode_phase('C');
            decode_phase('D');
        }
    }
}

void FlexProcessor::decode_phase(char PhaseNo) {
    uint32_t* phaseptr = nullptr;
    switch (PhaseNo) {
        case 'A':
            phaseptr = data.PhaseA.buf;
            break;
        case 'B':
            phaseptr = data.PhaseB.buf;
            break;
        case 'C':
            phaseptr = data.PhaseC.buf;
            break;
        case 'D':
            phaseptr = data.PhaseD.buf;
            break;
        default:
            return;
    }

    /* Check if phase is all idle BEFORE BCH correction.
     * Idle fill uses alternating 0xFFFFFFFF and 0x00000000 words.
     * If every word is one of these two patterns, the phase has no
     * real data - skip it to avoid BCH "correcting" idle into garbage. */
    {
        int all_idle = 1;
        for (int i = 0; i < 88; i++) {
            if (phaseptr[i] != 0xFFFFFFFF && phaseptr[i] != 0x00000000) {
                all_idle = 0;
                break;
            }
        }
        if (all_idle) return;
    }

    /* BCH decode each word. Mark uncorrectable words but continue. */
    uint8_t word_bad[88] = {0};
    for (int i = 0; i < 88; i++) {
        int decode_error = bch_fix_errors(&phaseptr[i]);
        if (decode_error > 2) {
            word_bad[i] = 1;
            phaseptr[i] = 0;
        }
        phaseptr[i] &= 0x001FFFFF;
    }

    /* BIW must be good to proceed */
    if (word_bad[0]) return;

    uint32_t biw = phaseptr[0];
    if (biw == 0 || biw == 0x001FFFFF) return;

    int voffset = (biw >> 10) & 0x3f;
    int aoffset = ((biw >> 8) & 0x03) + 1;
    int prio_count = (biw >> 4) & 0x0F;  // number of priority address words

    if (voffset < aoffset || voffset >= 88) return;

    /* Always send BIW1 packet so the app knows we decoded a frame.
     * This updates the status bar even for idle frames. */
    {
        flex::FlexPacket bpkt{};
        bpkt.type = 9;
        bpkt.bitrate = sync.baud * (sync.levels == 4 ? 2 : 1);
        bpkt.cycle = fiw.cycleno;
        bpkt.frame = fiw.frameno;
        bpkt.phase = PhaseNo;
        bpkt.is_inverted = sync.polarity;
        bpkt.fiw_roaming = fiw.roaming;
        bpkt.function = 0;
        bpkt.biw_field = 0xFF;
        bpkt.message[0] = '\0';
        send_packet(bpkt);
    }

    /* Parse BIW words (indices 1 through aoffset-1).
     * Each BIW word has a 3-bit type field (bits 4-6) that determines content.
     * Send each as a BIW event packet. */
    for (int bw = 1; bw < aoffset && bw < 88; bw++) {
        if (word_bad[bw]) continue;
        uint32_t bword = phaseptr[bw];
        uint32_t btype = (bword >> 4) & 0x07;

        /* Skip reserved types (3, 4, 6) */
        if (btype == 3 || btype == 4 || btype == 6) continue;

        flex::FlexPacket bpkt{};
        bpkt.type = 9;  // BIW event
        bpkt.bitrate = sync.baud * (sync.levels == 4 ? 2 : 1);
        bpkt.cycle = fiw.cycleno;
        bpkt.frame = fiw.frameno;
        bpkt.phase = PhaseNo;
        bpkt.is_inverted = sync.polarity;
        bpkt.fiw_roaming = fiw.roaming;
        bpkt.function = bw;      // BIW word index
        bpkt.biw_field = btype;  // BIW type (0,1,2,5,7)
        bpkt.message[0] = '\0';

        switch (btype) {
            case 0:  // SSID1: v1=lid, v2=cz
                bpkt.biw_v1 = (bword >> 12) & 0x01FF;
                bpkt.biw_v2 = (bword >> 7) & 0x1F;
                break;
            case 1:  // Date: v1=year(+1994), v2=month, v3=day
                bpkt.biw_v1 = ((bword >> 7) & 0x1F) + 1994;
                bpkt.biw_v2 = (bword >> 17) & 0x0F;
                bpkt.biw_v3 = (bword >> 12) & 0x1F;
                break;
            case 2:  // Time: v1=hour, v2=minute, v3=sec_raw(0-7)
                bpkt.biw_v1 = (bword >> 7) & 0x1F;
                bpkt.biw_v2 = (bword >> 12) & 0x3F;
                bpkt.biw_v3 = (bword >> 18) & 0x07;
                break;
            case 5:  // SysInfo: v1=a_type, v2=info(10 bits)
                bpkt.biw_v1 = (bword >> 7) & 0x0F;
                bpkt.biw_v2 = (bword >> 11) & 0x03FF;
                break;
            case 7:  // SSID2: v1=country, v2=tmf
                bpkt.biw_v1 = (bword >> 11) & 0x03FF;
                bpkt.biw_v2 = (bword >> 7) & 0x0F;
                break;
            default:
                continue;
        }
        send_packet(bpkt);
    }

    /* Pre-scan: count valid vector words using 4-bit nibble checksum.
     * Tone-only addresses sit at the end of the address field with no
     * corresponding vector.  We find the last vector that passes checksum.
     * Note: for long addresses, the 2nd vector word (Vy) is a message word
     * that won't pass checksum - so we count all passing words, not just
     * consecutive ones from the start. */
    int n_valid_vecs = 0;
    for (int vi = 0; vi < (voffset - aoffset); vi++) {
        int wi = voffset + vi;
        if (wi >= 88) break;
        uint32_t vw = phaseptr[wi];
        uint32_t csum = (vw & 0xF) + ((vw >> 4) & 0xF) + ((vw >> 8) & 0xF) +
                        ((vw >> 12) & 0xF) + ((vw >> 16) & 0xF) + ((vw >> 20) & 0x1);
        if ((csum & 0xF) == 0xF)
            n_valid_vecs = vi + 1;  // track highest passing index + 1
    }

    /* No addresses if voffset == aoffset */
    if (voffset <= aoffset) return;

    int vec_count = 0;
    int addr_count = 0;  // tracks address word position for priority detection
    for (int i = aoffset; i < voffset; i++) {
        int j = voffset + vec_count;
        if (j >= 88) break;
        if (phaseptr[i] == 0x00000000 || phaseptr[i] == 0x001FFFFF) continue;

        /* Address word - all 21 information bits are address data
         * per 3.8.2.  Address type is determined by value range
         * (Table 3.8.1-1).  Temporary addresses are range
         * 0x1F7800-0x1F780F (3.8.2.3), identified via addr_type. */
        int is_priority = (addr_count < prio_count) ? 1 : 0;

        parse_capcode(phaseptr[i]);
        decode.is_priority = is_priority;
        addr_count++;

        if (decode.long_address) {
            /* Long address: 2 address words, 2 vector words.
             * Read second address word and compute capcode from set. */
            if (i + 1 >= voffset) break;  // truncated
            uint32_t aw1 = phaseptr[i];
            uint32_t aw2 = phaseptr[i + 1];
            if (aw2 == 0x00000000 || aw2 == 0x001FFFFF) {
                i++;
                addr_count++;  // second address word counts
                vec_count += 2;
                continue;
            }

            int64_t cap = 0;
            if (aw1 >= 0x000001 && aw1 <= 0x008000 &&
                aw2 >= 0x1F7FFF && aw2 <= 0x1FFFFE) {
                /* Set 1-2 */
                cap = (int64_t)aw1 + (int64_t)(0x1FFFFF - aw2) * 32768LL + 2068480LL;
            } else if (aw1 >= 0x000001 && aw1 <= 0x008000 &&
                       aw2 >= 0x1E0001 && aw2 <= 0x1F0000) {
                /* Set 1-3 / 1-4 */
                cap = (int64_t)aw1 + (int64_t)(aw2 - 1933312) * 32768LL + 2068480LL;
            } else if (aw1 >= 0x1F7FFF && aw1 <= 0x1FFFFE &&
                       aw2 >= 0x1E0001 && aw2 <= 0x1F0000) {
                /* Set 2-3 */
                cap = (int64_t)(aw1 - 2064383) + (int64_t)(aw2 - 1867776) * 32768LL + 2068479LL;
            } else {
                /* Unknown set - skip */
                i++;
                addr_count++;  // second address word counts
                vec_count += 2;
                continue;
            }

            decode.capcode = cap;
            i++;           // consumed 2 address words
            addr_count++;  // second address word also counts

            /* Long addresses always have vectors - they cannot be tone-only.
             * (Tone-only is only for short addresses at the end of AF.)
             * The second vector word (Vy) contains the first message word,
             * not a checksummed vector, so skip the pre-scan check here. */
            vec_count += 2;               // consumed 2 vector words
            j = voffset + vec_count - 2;  // point to first vector word of pair
        } else {
            if (decode.capcode > 4297068542ll || decode.capcode <= 0) continue;

            /* Tone-only: address beyond valid vector range */
            if (vec_count >= n_valid_vecs) {
                parse_tone_only(phaseptr, PhaseNo, 0);
                continue;
            }
            vec_count++;
        }

        uint32_t viw = phaseptr[j];
        int type_val = (viw >> 4) & 0x07;

        switch (type_val) {
            case 0:
                decode.type = flex::PageType::SECURE;
                break;
            case 1:
                decode.type = flex::PageType::SHORT_INSTRUCTION;
                break;
            case 2:
                decode.type = flex::PageType::TONE;
                break;
            case 3:
                decode.type = flex::PageType::STANDARD_NUMERIC;
                break;
            case 4:
                decode.type = flex::PageType::SPECIAL_NUMERIC;
                break;
            case 5:
                decode.type = flex::PageType::ALPHANUMERIC;
                break;
            case 6:
                decode.type = flex::PageType::BINARY;
                break;
            case 7:
                decode.type = flex::PageType::NUMBERED_NUMERIC;
                break;
        }

        int mw1 = (viw >> 7) & 0x7F;
        int len;
        /* Numeric types (3, 4, 7) have a 3-bit n field (bits 14-16)
         * encoding word_count - 1.  Bits 17-20 are the K checksum.
         * Alpha/hex/secure types use the full 7-bit field (bits 14-20). */
        if (type_val == 3 || type_val == 4 || type_val == 7)
            len = ((viw >> 14) & 0x07) + 1;
        else
            len = (viw >> 14) & 0x7F;
        int mw2 = mw1 + (len - 1);

        if (mw1 == 0 && mw2 == 0) continue;
        if (decode.type == flex::PageType::TONE) mw1 = mw2 = 0;

        if (decode.type == flex::PageType::ALPHANUMERIC || decode.type == flex::PageType::SECURE) {
            if (mw1 > 87 || mw2 > 87) continue;
            if (decode.long_address) {
                /* For long addresses, body[0] (header with K,C,F,N,R,M) is at
                 * Vy (j+1), not at mw1.  The vector's mw1 points to body[1]
                 * in the message field, and len includes body[0].
                 * parse_alphanumeric expects mw1 = header word index (it does
                 * mw1++ internally to skip header).  So pass mw1-1 so the
                 * skip lands on mw1 (first real data word). */
                parse_alphanumeric(phaseptr, word_bad, PhaseNo, mw1 - 1, mw2 - 1, 0);
            } else {
                parse_alphanumeric(phaseptr, word_bad, PhaseNo, mw1, mw2, 0);
            }
        } else if (decode.type == flex::PageType::STANDARD_NUMERIC || decode.type == flex::PageType::SPECIAL_NUMERIC || decode.type == flex::PageType::NUMBERED_NUMERIC) {
            parse_numeric(phaseptr, word_bad, PhaseNo, j);
        } else if (decode.type == flex::PageType::TONE) {
            /* Vector type 2: Short Message (3.9.2).
             * Sub-type t1t0 in bits 7-8, data d0-d11 in bits 9-20. */
            uint32_t t = (viw >> 7) & 0x03;
            uint32_t d = (viw >> 9) & 0x0FFF;

            flex::FlexPacket packet{};
            packet.bitrate = sync.baud * (sync.levels == 4 ? 2 : 1);
            packet.capcode = decode.capcode;
            packet.function = t;
            packet.cycle = fiw.cycleno;
            packet.frame = fiw.frameno;
            packet.phase = PhaseNo;
            packet.is_inverted = sync.polarity;
            packet.fiw_roaming = fiw.roaming;
            packet.addr_type = static_cast<uint8_t>(decode.addr_type);
            packet.is_priority = decode.is_priority;
            packet.type = 8;  // SHORT

            if (t == 0 && d == 0xCCC) {
                /* Tone-only: all digits are space (0xC) per STD-43A
                 * Table 3.9.2-1 note. For long addresses, also check Vy. */
                bool tone = true;
                if (decode.long_address && j + 1 < 88) {
                    uint32_t vy = phaseptr[j + 1] & 0xFFFFF;
                    if (vy != 0xCCCCC) tone = false;
                }
                if (tone)
                    strcpy(packet.message, "TONE");
                else
                    goto short_numeric;
            } else if (t == 0) {
            short_numeric:
                /* Numeric: 3 BCD digits from Vx (d0-d11).
                 * Long addresses: 5 more digits from Vy (d12-d31),
                 * 8 digits total. d32 is spare (set to 0). */
                char *p = packet.message, *e = p + sizeof(packet.message);
                p = str_append(p, e, "NUM ");
                *p++ = flex_bcd[(d >> 0) & 0xF];
                *p++ = flex_bcd[(d >> 4) & 0xF];
                *p++ = flex_bcd[(d >> 8) & 0xF];
                if (decode.long_address && j + 1 < 88) {
                    uint32_t vy = phaseptr[j + 1];
                    *p++ = flex_bcd[(vy >> 0) & 0xF];
                    *p++ = flex_bcd[(vy >> 4) & 0xF];
                    *p++ = flex_bcd[(vy >> 8) & 0xF];
                    *p++ = flex_bcd[(vy >> 12) & 0xF];
                    *p++ = flex_bcd[(vy >> 16) & 0xF];
                }
                *p = '\0';
            } else if (t == 1) {
                /* Source: S2S1S0 in d0-d2 */
                char *p = packet.message, *e = p + sizeof(packet.message);
                p = str_append(p, e, "SRC ");
                str_uint(p, e, d & 0x07);
            } else if (t == 2) {
                /* Numbered: S(3) + N(6) + R(1) */
                uint32_t src = d & 0x07;
                uint32_t n = (d >> 3) & 0x3F;
                uint32_t r = (d >> 9) & 0x01;
                char *p = packet.message, *e = p + sizeof(packet.message);
                p = str_append(p, e, "SRC ");
                p = str_uint(p, e, src);
                p = str_append(p, e, " N=");
                p = str_uint(p, e, n);
                p = str_append(p, e, " R=");
                str_uint(p, e, r);
            } else {
                /* Reserved */
                char *p = packet.message, *e = p + sizeof(packet.message);
                p = str_append(p, e, "RESERVED ");
                str_hex(p, e, d, 3);
            }
            send_packet(packet);
        } else if (decode.type == flex::PageType::BINARY) {
            /* HEX/Binary message.
             * Word 1 (mw1): K(12) C(1) F(2) N(6) = header
             * Word 2 (mw1+1, first frag only): R(1) M(1) D(1) H(1) B(4) I(1) rsvd(4) S(8)
             * Words 3+: data */
            if (mw1 > 87 || mw2 > 87) continue;

            /* Extract header from word 1 */
            uint8_t hex_c = 0, hex_f = 0, hex_n = 0;
            int hex_hdr_valid = 0;
            if (!word_bad[mw1]) {
                uint32_t hw1 = phaseptr[mw1];
                hex_c = (hw1 >> 12) & 0x01;
                hex_f = (hw1 >> 13) & 0x03;
                hex_n = (hw1 >> 15) & 0x3F;
                hex_hdr_valid = 1;
            }

            /* Extract word 2 flags (first fragment: F=3) */
            uint8_t hex_r = 0, hex_m = 0, hex_d = 0, hex_b = 0;
            int data_start = mw1 + 1;  // default: data starts after header
            if (hex_f == 3 && (mw1 + 1) <= mw2 && !word_bad[mw1 + 1]) {
                uint32_t hw2 = phaseptr[mw1 + 1];
                hex_r = (hw2 >> 0) & 0x01;
                hex_m = (hw2 >> 1) & 0x01;
                hex_d = (hw2 >> 2) & 0x01;
                hex_b = (hw2 >> 4) & 0x0F;
                data_start = mw1 + 2;  // skip both header words
            }

            /* Dump data words as hex */
            char message[256] = {0};
            char *mp = message, *me = message + 250;
            for (int w = data_start; w <= mw2 && mp < me; w++) {
                if (word_bad[w]) {
                    mp = str_append(mp, me, "?????? ");
                } else {
                    mp = str_hex(mp, me, phaseptr[w] & 0x1FFFFF, 5);
                    if (mp < me) *mp++ = ' ';
                    *mp = '\0';
                }
            }
            if (mp > message && *(mp - 1) == ' ') {
                mp--;
                *mp = '\0';
            }
            int pos = (int)(mp - message);

            flex::FlexPacket packet{};
            packet.bitrate = sync.baud * (sync.levels == 4 ? 2 : 1);
            packet.capcode = decode.capcode;
            packet.function = 0;
            packet.type = 6;  // HEX
            packet.status = 0;
            packet.cycle = fiw.cycleno;
            packet.frame = fiw.frameno;
            packet.phase = PhaseNo;
            packet.is_inverted = sync.polarity;
            packet.fiw_roaming = fiw.roaming;
            packet.addr_type = static_cast<uint8_t>(decode.addr_type);
            packet.is_priority = decode.is_priority;
            if (hex_hdr_valid) {
                packet.frag = hex_f;
                packet.more_frag = hex_c;
                packet.seq = hex_n;
                packet.has_flags = 1;
                if (hex_f == 3) {
                    packet.is_new = hex_r;
                    packet.maildrop = hex_m;
                    /* Store b and d in function field: low nibble=b, bit4=d */
                    packet.function = (hex_d << 4) | hex_b;
                }
            }
            memcpy(packet.message, message, pos + 1);
            send_packet(packet);
        } else if (decode.type == flex::PageType::SHORT_INSTRUCTION) {
            /* Short instruction: 14-bit data in vector bits 7-20.
             * i2i1i0 (bits 0-2 of data) = instruction type.
             * Remaining bits = instruction-specific data. */
            uint32_t instr_data = (viw >> 7) & 0x3FFF;
            uint32_t itype = instr_data & 0x07;

            flex::FlexPacket packet{};
            packet.bitrate = sync.baud * (sync.levels == 4 ? 2 : 1);
            packet.capcode = decode.capcode;
            packet.function = 0;
            packet.type = 1;  // INS
            packet.cycle = fiw.cycleno;
            packet.frame = fiw.frameno;
            packet.phase = PhaseNo;
            packet.is_inverted = sync.polarity;
            packet.fiw_roaming = fiw.roaming;
            packet.addr_type = static_cast<uint8_t>(decode.addr_type);
            packet.is_priority = decode.is_priority;

            if (itype == 0) {
                uint32_t tgt_frame = (instr_data >> 3) & 0x7F;
                uint32_t slot = (instr_data >> 10) & 0x0F;
                packet.biw_v1 = slot;
                packet.biw_v2 = tgt_frame;
                {
                    char *p = packet.message, *e = p + sizeof(packet.message);
                    p = str_append(p, e, "i=temp|slot=");
                    p = str_uint(p, e, slot);
                    p = str_append(p, e, "|target=");
                    str_uint(p, e, tgt_frame);
                }
            } else if (itype == 1) {
                uint32_t flags = (instr_data >> 3) & 0x7FF;
                {
                    char *p = packet.message, *e = p + sizeof(packet.message);
                    p = str_append(p, e, "i=event|flags=");
                    str_hex(p, e, flags, 3);
                }
            } else {
                {
                    char *p = packet.message, *e = p + sizeof(packet.message);
                    p = str_append(p, e, "i=rsvd|type=");
                    p = str_uint(p, e, itype);
                    p = str_append(p, e, "|raw=");
                    str_hex(p, e, instr_data, 4);
                }
            }
            send_packet(packet);
        }
    }
}

void FlexProcessor::parse_capcode(uint32_t aw1) {
    /* Classify address word by range. */
    decode.long_address = 0;
    decode.addr_type = flex::AddrType::SHORT;

    if ((aw1 >= 0x000001 && aw1 <= 0x008000) || /* LA1 */
        (aw1 >= 0x1E0001 && aw1 <= 0x1E8000) || /* LA3 */
        (aw1 >= 0x1E8001 && aw1 <= 0x1F0000) || /* LA4 */
        (aw1 >= 0x1F7FFF && aw1 <= 0x1FFFFE)) { /* LA2 */
        decode.long_address = 1;
        decode.addr_type = flex::AddrType::LONG;
    } else if (aw1 >= 0x1F7800 && aw1 <= 0x1F780F) {
        decode.addr_type = flex::AddrType::TEMPORARY;
    } else if (aw1 >= 0x1F7810 && aw1 <= 0x1F781F) {
        decode.addr_type = flex::AddrType::OPERATOR;
    } else if (aw1 >= 0x1F6800 && aw1 <= 0x1F77FF) {
        decode.addr_type = flex::AddrType::NETWORK;
    } else if (aw1 >= 0x1F2800 && aw1 <= 0x1F67FF) {
        decode.addr_type = flex::AddrType::INFO_SVC;
    } else if ((aw1 >= 0x1F0001 && aw1 <= 0x1F27FF) ||
               (aw1 >= 0x1F7820 && aw1 <= 0x1F7FFE)) {
        decode.addr_type = flex::AddrType::RESERVED;
    } else if (aw1 >= 0x008001 && aw1 <= 0x1E0000) {
        decode.addr_type = flex::AddrType::SHORT;
    } else {
        decode.addr_type = flex::AddrType::UNKNOWN;
    }

    decode.capcode = aw1 - 0x8000;
}

void FlexProcessor::parse_alphanumeric(uint32_t* phaseptr, const uint8_t* word_bad, char PhaseNo, int mw1, int mw2, int) {
    char message[256] = {0};
    int currentChar = 0;

    /* First message word is the header (K, C, F, N, R, M fields).
     * Extract flags before skipping to content. */
    uint8_t hdr_c = 0, hdr_f = 0, hdr_n = 0, hdr_r = 0, hdr_m = 0;
    uint8_t hdr_sig = 0;
    int hdr_valid = 0;
    if (mw1 >= 0 && mw1 < 88 && !word_bad[mw1]) {
        uint32_t hdr = phaseptr[mw1];
        hdr_c = (hdr >> 10) & 0x01;  // bit 10
        hdr_f = (hdr >> 11) & 0x03;  // bits 11-12
        hdr_n = (hdr >> 13) & 0x3F;  // bits 13-18
        hdr_r = (hdr >> 19) & 0x01;  // bit 19
        hdr_m = (hdr >> 20) & 0x01;  // bit 20
        hdr_valid = 1;
    }
    mw1++;

    /* Extract signature from first data word (bits 0-6) */
    if (mw1 >= 0 && mw1 < 88 && !word_bad[mw1]) {
        hdr_sig = phaseptr[mw1] & 0x7F;
    }

    for (int i = mw1; i <= mw2; i++) {
        unsigned int dw = phaseptr[i];
        unsigned char ch;
        int bad = (i >= 0 && i < 88) ? word_bad[i] : 1;

        if (i > mw1) {
            ch = dw & 0x7F;
            if (bad) {
                if (currentChar < 255) message[currentChar++] = '?';
            } else if (ch >= 0x20 || ch == 0x0A || ch == 0x0D) {
                if (currentChar < 255) message[currentChar++] = ch;
            } else if (ch == 0x03 || ch == 0x00) {
                if (currentChar < 255) message[currentChar++] = '\x03';
            }
        }

        ch = (dw >> 7) & 0x7F;
        if (bad) {
            if (currentChar < 255) message[currentChar++] = '?';
        } else if (ch >= 0x20 || ch == 0x0A || ch == 0x0D) {
            if (currentChar < 255) message[currentChar++] = ch;
        } else if (ch == 0x03 || ch == 0x00) {
            if (currentChar < 255) message[currentChar++] = '\x03';
        }

        ch = (dw >> 14) & 0x7F;
        if (bad) {
            if (currentChar < 255) message[currentChar++] = '?';
        } else if (ch >= 0x20 || ch == 0x0A || ch == 0x0D) {
            if (currentChar < 255) message[currentChar++] = ch;
        } else if (ch == 0x03 || ch == 0x00) {
            if (currentChar < 255) message[currentChar++] = '\x03';
        }
    }

    /* Post-process: trim trailing ETX/NUL padding, but if printable chars
     * appear after an ETX/NUL, show each ETX/NUL as '?' (invalid char). */
    {
        /* First find the last printable character */
        int last_printable = -1;
        for (int k = 0; k < currentChar; k++) {
            if (message[k] != '\x03') last_printable = k;
        }
        /* Now output up to last_printable, replacing ETX with '?' */
        int out = 0;
        for (int k = 0; k <= last_printable && out < 255; k++) {
            if (message[k] == '\x03')
                message[out++] = '?';
            else
                message[out++] = message[k];
        }
        currentChar = out;
    }
    message[currentChar] = '\0';

    flex::FlexPacket packet{};
    packet.bitrate = sync.baud * (sync.levels == 4 ? 2 : 1);
    packet.capcode = decode.capcode;
    packet.function = 0;
    packet.type = (decode.type == flex::PageType::SECURE) ? 0 : 5;
    packet.status = 0;
    packet.cycle = fiw.cycleno;
    packet.frame = fiw.frameno;
    packet.phase = PhaseNo;
    packet.is_inverted = sync.polarity;
    packet.fiw_roaming = fiw.roaming;
    packet.addr_type = static_cast<uint8_t>(decode.addr_type);
    packet.is_priority = decode.is_priority;
    if (hdr_valid) {
        packet.frag = hdr_f;
        packet.more_frag = hdr_c;
        packet.seq = hdr_n;
        packet.is_new = hdr_r;
        packet.maildrop = hdr_m;
        packet.sig = hdr_sig;
        packet.has_flags = 1;
        if (decode.type == flex::PageType::SECURE) {
            /* Secure: bits 19-20 are t1t0 (encoding type), not R/M */
            packet.sec_enc = (hdr_r) | (hdr_m << 1);  // t0=bit19, t1=bit20
            packet.is_new = 0;
            packet.maildrop = 0;
        }
    }
    memcpy(packet.message, message, currentChar + 1);

    send_packet(packet);
}

void FlexProcessor::parse_numeric(uint32_t* phaseptr, const uint8_t* word_bad, char PhaseNo, int j) {
    char message[256] = {0};

    /* Extract NNUM header fields from first message word if applicable.
     * Layout: K5K4(2) + N0-N5(6) + R0(1) + S0(1) + BCD digits... */
    uint8_t nnum_n = 0, nnum_r = 0, nnum_s = 0;
    int is_nnum = (decode.type == flex::PageType::NUMBERED_NUMERIC);

    int w1 = phaseptr[j] >> 7;
    int w2 = w1 >> 7;
    w1 = w1 & 0x7f;
    int n_field = w2 & 0x07;  // word_count - 1
    w2 = n_field + w1;

    // Bounds check: phase buffer is 88 words (indices 0-87)
    if (w1 > 87) return;
    if (w2 > 87) w2 = 87;

    /* For long addresses (3.9.1):
     * 1-word: b field points to Vy. body[0] at w1.
     * Multi-word: body[0] at Vy (j+1). b field points to MF body[1]. */
    int body0_idx;
    if (decode.long_address && n_field > 0)
        body0_idx = j + 1;  // Vy = 2nd vector word
    else
        body0_idx = w1;
    if (body0_idx < 0 || body0_idx >= 88) return;

    int dw = phaseptr[body0_idx];

    if (is_nnum) {
        /* Extract N, R, S from the first message word's BCD stream.
         * After K5K4 (2 bits), next 6 bits = N, then R, then S.
         * These are consumed by the skip count (count starts at 4+10=14). */
        nnum_n = (dw >> 2) & 0x3F;  // bits 2-7
        nnum_r = (dw >> 8) & 0x01;  // bit 8
        nnum_s = (dw >> 9) & 0x01;  // bit 9
    }

    unsigned char digit = 0;
    int count = 4;
    if (is_nnum)
        count += 10;  // skip K5K4(2) + N(6) + R(1) + S(1)
    else
        count += 2;  // skip K5K4(2)

    int idx = 0;

    /* Phase 1: decode body[0] bits.
     * For short addresses, body[0] is at w1 and we advance to w1+1.
     * For long addresses, body[0] is at Vy (j+1), then we continue from w1. */
    if (word_bad[body0_idx]) {
        /* Uncorrectable word — emit '?' for each digit slot */
        int data_bits = 21 - (count - 4); /* bits available after skip */
        int lost_digits = data_bits / 4;
        while (lost_digits-- > 0 && idx < 255)
            message[idx++] = '?';
        count = 4; /* reset for next word */
        digit = 0;
    } else {
        for (int k = 0; k < 21; k++) {
            digit = (digit >> 1) & 0x0F;
            if (dw & 0x01) digit ^= 0x08;
            dw >>= 1;
            if (--count == 0) {
                if (idx < 255) {
                    message[idx++] = flex_bcd[digit];
                }
                count = 4;
            }
        }
    }

    /* Phase 2: decode remaining body words from MF.
     * Short: body[1..n] at w1+1 .. w2.
     * Long: MF has n_field words at w1 .. w1+n_field-1.
     *   (n_field = total_words - 1; body[0] is at Vy, not in MF) */
    int start, end;
    if (decode.long_address) {
        start = w1;
        end = w1 + n_field - 1;  // empty when n_field=0
    } else {
        start = w1 + 1;
        end = w2;
    }
    for (int i = start; i <= end && i < 88; i++) {
        if (word_bad[i]) {
            /* Uncorrectable word — emit '?' for each digit slot (5 per word) */
            int lost_digits = 21 / 4; /* 5 digits per 21-bit word */
            while (lost_digits-- > 0 && idx < 255)
                message[idx++] = '?';
            count = 4;
            digit = 0;
            continue;
        }
        dw = phaseptr[i];
        for (int k = 0; k < 21; k++) {
            digit = (digit >> 1) & 0x0F;
            if (dw & 0x01) digit ^= 0x08;
            dw >>= 1;
            if (--count == 0) {
                if (idx < 255) {
                    message[idx++] = flex_bcd[digit];
                }
                count = 4;
            }
        }
    }

    /* Trim trailing BCD space padding (0x0C = ' ').
     * The encoder pads unused nibble slots with 0x0C */
    while (idx > 0 && message[idx - 1] == ' ')
        idx--;
    message[idx] = '\0';

    flex::FlexPacket packet{};
    packet.bitrate = sync.baud * (sync.levels == 4 ? 2 : 1);
    packet.capcode = decode.capcode;
    packet.function = 0;
    /* Set correct type: 3=NUM, 4=SNUM, 7=NNUM */
    if (decode.type == flex::PageType::SPECIAL_NUMERIC)
        packet.type = 4;
    else if (is_nnum)
        packet.type = 7;
    else
        packet.type = 3;
    packet.status = 0;
    packet.cycle = fiw.cycleno;
    packet.frame = fiw.frameno;
    packet.phase = PhaseNo;
    packet.is_inverted = sync.polarity;
    packet.fiw_roaming = fiw.roaming;
    packet.addr_type = static_cast<uint8_t>(decode.addr_type);
    packet.is_priority = decode.is_priority;
    if (is_nnum) {
        packet.seq = nnum_n;
        packet.is_new = nnum_r;
        packet.nnum_s = nnum_s;
        packet.has_flags = 1;
    }
    memcpy(packet.message, message, idx + 1);

    send_packet(packet);
}

void FlexProcessor::parse_tone_only(uint32_t*, char PhaseNo, int) {
    if (decode.capcode == 1) return;  // idle artifact
    flex::FlexPacket packet{};
    packet.bitrate = sync.baud * (sync.levels == 4 ? 2 : 1);
    packet.capcode = decode.capcode;
    packet.function = 0;
    packet.type = 2;  // TONE
    packet.status = 0;
    packet.cycle = fiw.cycleno;
    packet.frame = fiw.frameno;
    packet.phase = PhaseNo;
    packet.is_inverted = sync.polarity;
    packet.fiw_roaming = fiw.roaming;
    packet.addr_type = static_cast<uint8_t>(decode.addr_type);
    packet.is_priority = decode.is_priority;
    strcpy(packet.message, "");

    send_packet(packet);
}

void FlexProcessor::parse_unknown(uint32_t*, char, int, int) {
    // Ignored
}

void FlexProcessor::on_message(const Message* const message) {
    if (message->id == Message::ID::FlexConfigure) {
        configure();
    }
}

void FlexProcessor::configure() {
    decim_0_iq.configure(taps_11k0_decim_0.taps);
    decim_1_iq.configure(taps_11k0_decim_1.taps);
    channel_filter.configure(taps_11k0_channel.taps, 2);  // Decim 2 -> 24kHz output

    demod.configure(24000, 4800);
    demodulator.sample_freq = 24000;

    configured = true;
    send_debug("Configured", 0, 0);
}

void FlexProcessor::send_packet(const flex::FlexPacket& packet) {
    FlexPacketMessage message(packet);
    shared_memory.application_queue.push(message);
}

void FlexProcessor::send_stats() {
    // Stats
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<FlexProcessor>()};
    event_dispatcher.run();
    return 0;
}
