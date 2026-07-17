#include "proc_wmbus.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"

WMBusProcessor::WMBusProcessor() {
    baseband_thread.start();
}

uint8_t WMBusProcessor::decode_3out6(uint8_t chips) {
    switch (chips & 0x3F) {
        case 0x16:
            return 0x0;
        case 0x0D:
            return 0x1;
        case 0x0E:
            return 0x2;
        case 0x0B:
            return 0x3;
        case 0x1C:
            return 0x4;
        case 0x19:
            return 0x5;
        case 0x1A:
            return 0x6;
        case 0x13:
            return 0x7;
        case 0x2C:
            return 0x8;
        case 0x25:
            return 0x9;
        case 0x26:
            return 0xA;
        case 0x23:
            return 0xB;
        case 0x34:
            return 0xC;
        case 0x31:
            return 0xD;
        case 0x32:
            return 0xE;
        case 0x29:
            return 0xF;
        default:
            return 0xFF;  // Hibás kód
    }
}

void WMBusProcessor::execute(const buffer_c8_t& buffer) {
    for (size_t i = 0; i < buffer.count; i++) {
        int16_t samp_i = buffer.p[i].real();
        int16_t samp_q = buffer.p[i].imag();

        // 1. I/Q DC Blocker
        i_dc_acc += samp_i - (i_dc_acc >> 10);
        q_dc_acc += samp_q - (q_dc_acc >> 10);
        int16_t clean_i = samp_i - (i_dc_acc >> 10);
        int16_t clean_q = samp_q - (q_dc_acc >> 10);

        // 2. I/Q 4-tap  filter
        i_sum += clean_i - i_hist[iq_idx];
        i_hist[iq_idx] = clean_i;

        q_sum += clean_q - q_hist[iq_idx];
        q_hist[iq_idx] = clean_q;
        iq_idx = (iq_idx + 1) & 3;

        int16_t filt_i = i_sum / 4;
        int16_t filt_q = q_sum / 4;

        // 3. FM Demodulator
        int32_t fm_val = (filt_q * prev_i) - (filt_i * prev_q);
        prev_i = filt_i;
        prev_q = filt_q;

        // 4. FM LPF 4-tap filter
        fm_sum += fm_val - fm_hist[fm_idx];
        fm_hist[fm_idx] = fm_val;
        fm_idx = (fm_idx + 1) & 3;
        int32_t fm_lpf = fm_sum / 4;

        // 5. fast DC Tracker
        // A preamble (01010101)
        fm_dc_avg = (fm_dc_avg * 63 + fm_lpf) / 64;
        int32_t ac_val = fm_lpf - fm_dc_avg;

        uint32_t abs_ac = (ac_val < 0) ? -ac_val : ac_val;
        if (abs_ac > peak_fm_val) peak_fm_val = abs_ac;

        // Slicer
        bool current_bit = (ac_val > 0);

        // 6. PLL
        if (current_bit != last_bit) {
            int32_t error = 500 - (int32_t)phase;
            phase = (phase + error / 4) % 1000;
            last_bit = current_bit;
        }

        // T-Mode 100kcps @ 1MHz -> phase shift  = 100 (1000 a max)
        // S-Mode 65.536kcps @ 1MHz -> phase shift = 65
        uint32_t phase_step = (op_mode == 2) ? 65 : 100;
        phase += phase_step;

        if (phase >= 1000) {
            phase -= 1000;
            consume_chip(current_bit ? 1 : 0);
        }
    }

    telemetry_counter += buffer.count;
    if (telemetry_counter >= baseband_fs / 2) {
        telemetry_counter = 0;
        WMBusPacketMessage dbg_msg;
        dbg_msg.length = 0;
        dbg_msg.data[0] = static_cast<uint8_t>(sync_state);

        uint32_t scaled_peak = peak_fm_val / 64;
        dbg_msg.data[1] = (scaled_peak > 255) ? 255 : static_cast<uint8_t>(scaled_peak);
        dbg_msg.data[2] = static_cast<uint8_t>(stat_syncs & 0xFF);
        dbg_msg.data[3] = static_cast<uint8_t>(stat_errors & 0xFF);
        dbg_msg.data[4] = op_mode;
        dbg_msg.data[5] = last_err_reason;
        dbg_msg.data[6] = last_err_data;

        shared_memory.application_queue.push(dbg_msg);
        peak_fm_val = 0;
    }
}

void WMBusProcessor::consume_chip(uint8_t bit) {
    chip_reg = (chip_reg << 1) | bit;

    if (sync_state == SyncState::UNSYNCED) {
        if (op_mode == 0 || op_mode == 1) {
            uint16_t sync_window = chip_reg & 0xFFFF;

            int err_norm = __builtin_popcount(sync_window ^ 0x543D);
            int err_inv = __builtin_popcount(sync_window ^ 0xABC2);

            if (err_norm <= 1 || err_inv <= 1) {
                inverted_iq = (err_inv <= 1);
                sync_state = SyncState::READ_LEN_H;
                chip_count = 0;
                payload_idx = 0;
                stat_syncs++;
            }
        } else if (op_mode == 2) {
            uint32_t sync_window = chip_reg & 0x3FFFF;
            int err_norm = __builtin_popcount(sync_window ^ 0x00A6A6);
            int err_inv = __builtin_popcount(sync_window ^ (~0x00A6A6 & 0x3FFFF));

            if (err_norm <= 1 || err_inv <= 1) {
                inverted_iq = (err_inv <= 1);
                sync_state = SyncState::READ_LEN_L;
                chip_count = 0;
                payload_idx = 0;
                stat_syncs++;
            }
        }
    } else {
        if (op_mode == 0) {  // T-MODE
            chip_count++;
            if (chip_count == 6) {
                chip_count = 0;
                uint8_t raw = chip_reg & 0x3F;
                if (inverted_iq) raw = (~raw) & 0x3F;

                uint8_t nibble = decode_3out6(raw);
                if (nibble == 0xFF) {
                    sync_state = SyncState::UNSYNCED;
                    stat_errors++;
                    last_err_reason = 1;  // 3v6 err
                    last_err_data = raw;
                    return;
                }

                if (sync_state == SyncState::READ_LEN_H || sync_state == SyncState::READ_DATA_H) {
                    current_byte = (nibble << 4);
                    sync_state = (sync_state == SyncState::READ_LEN_H) ? SyncState::READ_LEN_L : SyncState::READ_DATA_L;
                } else {
                    current_byte |= nibble;
                    handle_byte(current_byte);
                    if (sync_state == SyncState::READ_DATA_L) {
                        sync_state = SyncState::READ_DATA_H;
                    }
                }
            }
        } else if (op_mode == 1) {  // C-MODE
            uint32_t sync_window = chip_reg & 0xFFFFFF;

            // Format A: 0x55543D (norm) / 0xAAABC2 (inv)
            int err_a_norm = __builtin_popcount(sync_window ^ 0x55543D);
            int err_a_inv = __builtin_popcount(sync_window ^ 0xAAABC2);

            // Format B: 0x5554CB (norm) / 0xAAAB34 (inv)
            int err_b_norm = __builtin_popcount(sync_window ^ 0x5554CB);
            int err_b_inv = __builtin_popcount(sync_window ^ 0xAAAB34);

            if (err_a_norm <= 1 || err_a_inv <= 1 || err_b_norm <= 1 || err_b_inv <= 1) {
                inverted_iq = (err_a_inv <= 1 || err_b_inv <= 1);
                sync_state = SyncState::READ_LEN_L;
                chip_count = 0;
                payload_idx = 0;
                stat_syncs++;
            }
        } else if (op_mode == 2) {  // S-MODE
            chip_count++;
            if (chip_count == 16) {
                chip_count = 0;
                uint8_t byte = 0;
                for (int b = 0; b < 8; b++) {
                    uint8_t pair = (chip_reg >> ((7 - b) * 2)) & 0x03;
                    if (inverted_iq) pair = (~pair) & 0x03;

                    if (pair == 0x02)
                        byte |= (1 << (7 - b));
                    else if (pair == 0x01) { /* 0  */
                    } else {
                        sync_state = SyncState::UNSYNCED;
                        stat_errors++;
                        last_err_reason = 4;  // Manchester err
                        last_err_data = pair;
                        return;
                    }
                }
                handle_byte(byte);
            }
        }
    }
}

void WMBusProcessor::handle_byte(uint8_t byte) {
    if (sync_state == SyncState::READ_LEN_L || sync_state == SyncState::READ_LEN_H) {
        payload_length = byte;

        if (payload_length < 9 || payload_length > 250) {
            sync_state = SyncState::UNSYNCED;
            stat_errors++;
            last_err_reason = (payload_length < 9) ? 2 : 3;  // len error
            last_err_data = payload_length;
            return;
        }

        // CRC bytes
        physical_length = payload_length + 1 + 2;
        if (payload_length > 9) {
            uint8_t rem = payload_length - 9;
            physical_length += ((rem + 15) / 16) * 2;
        }

        payload_buffer[0] = byte;
        payload_idx = 1;

        sync_state = (op_mode == 0) ? SyncState::READ_DATA_H : SyncState::READ_DATA_L;
    } else {
        payload_buffer[payload_idx++] = byte;

        if (payload_idx >= physical_length) {
            WMBusPacketMessage msg;
            msg.length = payload_idx;
            for (int i = 0; i < payload_idx; i++) {
                msg.data[i] = payload_buffer[i];
            }
            shared_memory.application_queue.push(msg);

            sync_state = SyncState::UNSYNCED;
            last_err_reason = 0;  // ok!
        }
    }
}

void WMBusProcessor::on_message(const Message* const message) {
    if (message->id == Message::ID::WMBusPacketMessageID) {
        op_mode = ((WMBusPacketMessage*)message)->length;
        sync_state = SyncState::UNSYNCED;
        chip_count = 0;
        payload_idx = 0;
        last_err_reason = 0;
        last_err_data = 0;
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<WMBusProcessor>()};
    event_dispatcher.run();
    return 0;
}