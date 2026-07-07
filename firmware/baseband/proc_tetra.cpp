#include "proc_tetra.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <algorithm>

TetraProcessor::TetraProcessor() {
    decim_0.configure(taps_25k0_tetra_decim_0.taps);
    decim_1.configure(taps_25k0_tetra_decim_1.taps);

    baseband_thread.start();
    configured = true;
}

void TetraProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer_0);
    const auto channel_out = decim_1.execute(decim_0_out, dst_buffer_1);

    feed_channel_stats(channel_out);

    for (size_t i = 0; i < channel_out.count; i++) {
        complex16_t sample = channel_out.p[i];

        // Carrier PLL Rotation
        uint8_t phase_idx = (pll_phase >> 24) & 0xFF;
        uint8_t cos_idx = (phase_idx + 64) & 0xFF;
        int16_t rotated_real = (sample.real() * sine_table_i8[cos_idx] - sample.imag() * sine_table_i8[phase_idx]) >> 7;
        int16_t rotated_imag = (sample.real() * sine_table_i8[phase_idx] + sample.imag() * sine_table_i8[cos_idx]) >> 7;
        complex16_t rotated_sample = {rotated_real, rotated_imag};

        uint32_t old_phase = symbol_phase;
        symbol_phase += symbol_phase_inc;

        // Half-symbol timing
        if ((old_phase < 0x80000000) && (symbol_phase >= 0x80000000)) {
            mid_sample = rotated_sample;
        }

        // Full-symbol timing
        if (symbol_phase < old_phase) {
            prev_prompt = prompt_sample;
            prompt_sample = rotated_sample;

            // Gardner Timing Error Detector
            int32_t err_i = (mid_sample.real() * (prompt_sample.real() - prev_prompt.real()));
            int32_t err_q = (mid_sample.imag() * (prompt_sample.imag() - prev_prompt.imag()));
            int32_t timing_error = err_i + err_q;

            symbol_phase_inc += (timing_error >> 16);

            // CLAMPING: Prevent the NCO from drifting into the noise!
            if (symbol_phase_inc < 1600000000) symbol_phase_inc = 1600000000;
            if (symbol_phase_inc > 1620000000) symbol_phase_inc = 1620000000;

            process_symbol(prompt_sample);
        }
    }
}

void TetraProcessor::process_symbol(const complex16_t& current_sample) {
    // Differential phase demodulation
    int32_t dot_i = (current_sample.real() * delay_sample.real()) + (current_sample.imag() * delay_sample.imag());
    int32_t dot_q = (current_sample.imag() * delay_sample.real()) - (current_sample.real() * delay_sample.imag());
    delay_sample = current_sample;

    // Phase error for Carrier PLL
    int32_t phase_err = 0;
    if (dot_i > 0 && dot_q > 0)
        phase_err = dot_q - dot_i;
    else if (dot_i < 0 && dot_q > 0)
        phase_err = dot_q + dot_i;
    else if (dot_i < 0 && dot_q < 0)
        phase_err = -dot_q + dot_i;
    else
        phase_err = -dot_q - dot_i;

    pll_freq += (phase_err * pll_beta) >> 8;

    // CLAMPING: Prevent frequency tracking from flying away
    if (pll_freq > 400000000) pll_freq = 400000000;
    if (pll_freq < -400000000) pll_freq = -400000000;

    pll_phase += ((phase_err * pll_alpha) >> 8) + pll_freq;

    // ETSI EN 300 392-2 pi/4 DQPSK bit mapping
    uint8_t dibit = 0;
    if (dot_i > 0 && dot_q > 0)
        dibit = 0b00;
    else if (dot_i < 0 && dot_q > 0)
        dibit = 0b01;
    else if (dot_i < 0 && dot_q < 0)
        dibit = 0b11;
    else
        dibit = 0b10;

    // Insert bits into history buffer and sync register
    for (int b = 1; b >= 0; b--) {
        uint8_t bit_val = (dibit >> b) & 0x01;

        size_t byte_idx = (history_write_idx / 8) % 128;
        if ((history_write_idx % 8) == 0) bit_history_buffer[byte_idx] = 0;

        bit_history_buffer[byte_idx] |= (bit_val << (7 - (history_write_idx % 8)));
        history_write_idx = (history_write_idx + 1) % 1024;
        bit_count++;

        sync_register = ((sync_register << 1) | bit_val);

        uint64_t sync =
            sync_register & 0x3FFFFFFFFFULL;

        uint32_t err_pos =
            __builtin_popcountll(sync ^ Y_SYNC);

        uint32_t err_neg =
            __builtin_popcountll(
                sync ^
                (~Y_SYNC & 0x3FFFFFFFFFULL));

        if (!pending_dsb.valid && bit_count >= Y_SYNC_BITS && (err_pos <= 4 || err_neg <= 4)) {
            const uint64_t sync_start = bit_count - Y_SYNC_BITS;

            if (sync_start >= SYNC_OFFSET) {
                pending_dsb.valid = true;
                pending_dsb.burst_start = sync_start - SYNC_OFFSET;
                pending_dsb.ready_at = pending_dsb.burst_start + BURST_BITS;
                pending_dsb.inverted = err_neg < err_pos;
                pending_dsb.errors = std::min(err_pos, err_neg);
            }
        }

        if (ENABLE_DNB_MESSAGES) {
            const uint32_t train_mask = (1UL << DNB_TRAIN_BITS) - 1;
            const uint32_t train = sync_register & train_mask;
            const uint32_t n_err_pos = __builtin_popcount(train ^ N_SYNC);
            const uint32_t n_err_neg = __builtin_popcount(train ^ (~N_SYNC & train_mask));
            const uint32_t p_err_pos = __builtin_popcount(train ^ P_SYNC);
            const uint32_t p_err_neg = __builtin_popcount(train ^ (~P_SYNC & train_mask));

            if (!pending_dnb.valid && bit_count >= DNB_TRAIN_BITS && (n_err_pos <= 1 || n_err_neg <= 1 || p_err_pos <= 1 || p_err_neg <= 1)) {
                const bool p_train = std::min(p_err_pos, p_err_neg) < std::min(n_err_pos, n_err_neg);
                const uint32_t pos_err = p_train ? p_err_pos : n_err_pos;
                const uint32_t neg_err = p_train ? p_err_neg : n_err_neg;

                const uint64_t train_start = bit_count - DNB_TRAIN_BITS;
                const uint64_t block1_start = train_start - 230;
                const uint64_t block2_start = train_start + 38;

                if (train_start >= 230) {
                    pending_dnb.valid = true;
                    pending_dnb.block1_start = block1_start;
                    pending_dnb.block2_start = block2_start;
                    pending_dnb.ready_at = block2_start + DNB_BLK_BITS;
                    pending_dnb.inverted = neg_err < pos_err;
                    pending_dnb.p_train = p_train;
                    pending_dnb.errors = std::min(pos_err, neg_err);
                }
            }

            if (pending_dnb.valid && bit_count >= pending_dnb.ready_at)
                push_dnb_to_ui();
        }

        if (pending_dsb.valid && bit_count >= pending_dsb.ready_at)
            push_burst_to_ui();
    }
}

uint8_t TetraProcessor::history_bit(uint64_t absolute_bit) const {
    const size_t p = absolute_bit % 1024;

    return (bit_history_buffer[p >> 3] >>
            (7 - (p & 7))) &
           1;
}

void TetraProcessor::push_burst_to_ui() {
    std::array<uint8_t, 63> burst{};

    for (size_t i = 0; i < BURST_BITS; i++) {
        uint8_t b = history_bit(pending_dsb.burst_start + i);

        if (pending_dsb.inverted)
            b ^= 1;

        burst[i >> 3] |=
            b << (7 - (i & 7));
    }

    shared_memory.application_queue.push(
        TetraBurstMessage(
            burst.data(),
            pending_dsb.inverted,
            pending_dsb.errors));

    pending_dsb.valid = false;
}

void TetraProcessor::push_dnb_to_ui() {
    std::array<uint8_t, 54> burst{};

    for (size_t i = 0; i < DNB_BLK_BITS; i++) {
        uint8_t b = history_bit(pending_dnb.block1_start + i);
        if (pending_dnb.inverted)
            b ^= 1;
        burst[i >> 3] |= b << (7 - (i & 7));
    }

    for (size_t i = 0; i < DNB_BLK_BITS; i++) {
        uint8_t b = history_bit(pending_dnb.block2_start + i);
        if (pending_dnb.inverted)
            b ^= 1;
        const size_t out_bit = DNB_BLK_BITS + i;
        burst[out_bit >> 3] |= b << (7 - (out_bit & 7));
    }

    shared_memory.application_queue.push(
        TetraDnbMessage(
            burst.data(),
            pending_dnb.inverted,
            pending_dnb.errors,
            pending_dnb.p_train));

    pending_dnb.valid = false;
}

void TetraProcessor::on_message(const Message* const msg) {
    (void)msg;
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<TetraProcessor>()};
    event_dispatcher.run();
    return 0;
}