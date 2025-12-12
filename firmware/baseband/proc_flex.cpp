#include "proc_flex.hpp"
#include "event_m4.hpp"
#include "audio_dma.hpp"
#include "pocsag.hpp"
#include "dsp_fir_taps.hpp"
#include "portapack_shared_memory.hpp"

#include <cmath>
#include <cstring>
#include <cstdio>  // for snprintf

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
    fiw.fix3 = (fiw_val >> 15) & 0x3F;

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
            if (++state.sync2_count == sync.baud * 25 / 1000) {
                state.data_count = 0;
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

                state.Current = flex::State::DATA;
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

    for (int i = 0; i < 88; i++) {
        int decode_error = bch_fix_errors(&phaseptr[i]);
        if (decode_error > 2) return;
        phaseptr[i] &= 0x001FFFFF;  // Extract message bits
    }

    uint32_t biw = phaseptr[0];
    if (biw == 0 || biw == 0x001FFFFF) return;

    int voffset = (biw >> 10) & 0x3f;
    int aoffset = ((biw >> 8) & 0x03) + 1;

    for (int i = aoffset; i < voffset; i++) {
        int j = voffset + i - aoffset;
        if (phaseptr[i] == 0x00000000 || phaseptr[i] == 0x001FFFFF) continue;

        parse_capcode(phaseptr[i]);
        if (decode.long_address) continue;  // Skip long addresses for now

        if (decode.capcode > 4297068542ll || decode.capcode < 0) continue;

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
        int len = (viw >> 14) & 0x7F;
        int mw2 = mw1 + (len - 1);

        if (mw1 == 0 && mw2 == 0) continue;
        if (decode.type == flex::PageType::TONE) mw1 = mw2 = 0;

        if (decode.type == flex::PageType::ALPHANUMERIC || decode.type == flex::PageType::SECURE) {
            if (mw1 > 87 || mw2 > 87) continue;
            parse_alphanumeric(phaseptr, PhaseNo, mw1, mw2, 0);
        } else if (decode.type == flex::PageType::STANDARD_NUMERIC || decode.type == flex::PageType::SPECIAL_NUMERIC || decode.type == flex::PageType::NUMBERED_NUMERIC) {
            parse_numeric(phaseptr, PhaseNo, j);
        } else if (decode.type == flex::PageType::TONE) {
            parse_tone_only(phaseptr, PhaseNo, j);
        } else {
            // Unknown or unsupported
        }
    }
}

void FlexProcessor::parse_capcode(uint32_t aw1) {
    decode.long_address = (aw1 < 0x008001L) || (aw1 > 0x1E0000L) || (aw1 > 0x1E7FFEL);
    decode.capcode = aw1 - 0x8000;
}

void FlexProcessor::parse_alphanumeric(uint32_t* phaseptr, char, int mw1, int mw2, int) {
    char message[128] = {0};  // Fixed buffer for message
    int currentChar = 0;

    // int frag = (phaseptr[mw1] >> 11) & 0x03;
    // int cont = (phaseptr[mw1] >> 0x0A) & 0x01;
    // Helper logic for fragmentation (ignored for basic display)

    mw1++;

    for (int i = mw1; i <= mw2; i++) {
        unsigned int dw = phaseptr[i];
        unsigned char ch;

        // Extract chars (7-bit ASCII)
        // If i > mw1 (not first word) or fragment check (simplified here)
        if (i > mw1) {
            ch = dw & 0x7F;
            if (ch != 0x03 && currentChar < 127) message[currentChar++] = ch;
        }

        ch = (dw >> 7) & 0x7F;
        if (ch != 0x03 && currentChar < 127) message[currentChar++] = ch;

        ch = (dw >> 14) & 0x7F;
        if (ch != 0x03 && currentChar < 127) message[currentChar++] = ch;
    }
    message[currentChar] = '\0';

    flex::FlexPacket packet;
    packet.bitrate = sync.baud;
    packet.capcode = decode.capcode;
    packet.function = 0;  // TODO extract function if available
    packet.type = 5;      // ALPHANUMERIC
    packet.status = 0;    // OK
    memcpy(packet.message, message, currentChar + 1);

    send_packet(packet);
}

void FlexProcessor::parse_numeric(uint32_t* phaseptr, char, int j) {
    // Simplified numeric parsing
    char message[128] = {0};
    const char flex_bcd[] = "0123456789 U -][";

    int w1 = phaseptr[j] >> 7;
    int w2 = w1 >> 7;
    w1 = w1 & 0x7f;
    w2 = (w2 & 0x07) + w1;

    int dw;
    // Handle short vs long logic if needed (simplified)
    dw = phaseptr[w1];
    w1++;
    w2++;

    unsigned char digit = 0;
    int count = 4;  // Standard numeric skip
    if (decode.type == flex::PageType::NUMBERED_NUMERIC)
        count += 10;
    else
        count += 2;

    int idx = 0;
    for (int i = w1; i <= w2; i++) {
        for (int k = 0; k < 21; k++) {
            digit = (digit >> 1) & 0x0F;
            if (dw & 0x01) digit ^= 0x08;
            dw >>= 1;
            if (--count == 0) {
                if (digit != 0x0C && idx < 127) {
                    message[idx++] = flex_bcd[digit];
                }
                count = 4;
            }
        }
        dw = phaseptr[i];
    }
    message[idx] = '\0';

    flex::FlexPacket packet;
    packet.bitrate = sync.baud;
    packet.capcode = decode.capcode;
    packet.function = 0;
    packet.type = 3;  // NUMERIC
    packet.status = 0;
    memcpy(packet.message, message, idx + 1);

    send_packet(packet);
}

void FlexProcessor::parse_tone_only(uint32_t*, char, int) {
    flex::FlexPacket packet;
    packet.bitrate = sync.baud;
    packet.capcode = decode.capcode;
    packet.function = 0;
    packet.type = 2;  // TONE
    packet.status = 0;
    snprintf(packet.message, sizeof(packet.message), "Tone Only");

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
