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
            return 0xFF;
    }
}

void WMBusProcessor::execute(const buffer_c8_t& buffer) {
    op_mode = 0;  // static_cast<uint8_t>(shared_memory.squelch_level);

    for (size_t i = 0; i < buffer.count; i++) {
        int16_t samp_i = buffer.p[i].real();
        int16_t samp_q = buffer.p[i].imag();

        // 1. FM Demodulátor (A kódod polar_discriminator_t1_c1_inaccurate függvénye alapján)
        int32_t fm_val = (samp_q * prev_i) - (samp_i * prev_q);
        prev_i = samp_i;
        prev_q = samp_q;

        // 2. Aluláteresztő szűrő (Boxcar 4-tap)
        fm_sum += fm_val - fm_hist[hist_idx];
        fm_hist[hist_idx] = fm_val;
        hist_idx = (hist_idx + 1) & 3;

        // 3. IIR DC Blocker (Hasonló a t1_c1_remove_dc_offset_demod-hoz)
        dc_acc += fm_sum - (dc_acc >> 8);
        int32_t ac_val = fm_sum - (dc_acc >> 8);

        int32_t abs_ac = (ac_val < 0) ? -ac_val : ac_val;
        if (abs_ac > peak_fm_val) peak_fm_val = abs_ac;

        // 4. Slicer & Deglitch szűrő (A kódod deglitch_filter_t1_c1 tömbje alapján)
        // 7 mintás csúszóablak. Ha legalább 4 darab 1-es van benne, akkor az állapot 1.
        uint8_t raw_bit = (ac_val > 0) ? 1 : 0;
        deglitch_reg = (deglitch_reg << 1) | raw_bit;
        int ones = __builtin_popcount(deglitch_reg & 0x7F);
        uint8_t state = (ones >= 4) ? 1 : 0;

        // =====================================================================
        // 5. A SZENT GRÁL: RUN-LENGTH ALGORITMUS (Órajel helyett él-távolság)
        // =====================================================================
        if (state == current_state) {
            run_length++;
            if (run_length > 1000) run_length = 1000;  // Túlcsordulás védelem csend esetén
        } else {
            // Élváltás történt! Kiszámoljuk, hány chipet jelentett az előző futáshossz.
            if (run_length >= 3) {  // A túl pici glitcheket (zaj) ignoráljuk
                int num_chips = 0;

                if (op_mode == 2) {
                    // S-Mode: 65.5 kchips/s -> 15.26 minta / chip
                    num_chips = (run_length * 100 + 763) / 1526;
                } else {
                    // T és C Mode: 100 kchips/s -> 10 minta / chip
                    num_chips = (run_length + 5) / 10;
                }

                // Betoljuk a kinyert chipeket a MAC rétegbe
                for (int k = 0; k < num_chips; k++) {
                    consume_chip(current_state);
                }
            }
            // Átállunk az új állapotra, és újraindítjuk a számlálót
            current_state = state;
            run_length = 1;
        }
    }

    // 6. Telemetria (Másodpercenként 2x)
    telemetry_counter += buffer.count;
    if (telemetry_counter >= baseband_fs / 2) {
        telemetry_counter = 0;
        WMBusPacketMessage dbg_msg;
        dbg_msg.length = 0;
        dbg_msg.data[0] = static_cast<uint8_t>(sync_state);

        uint32_t scaled_peak = peak_fm_val / 256;  // Nyers szorzat miatt skálázás
        dbg_msg.data[1] = (scaled_peak > 255) ? 255 : static_cast<uint8_t>(scaled_peak);
        dbg_msg.data[2] = static_cast<uint8_t>(stat_syncs & 0xFF);
        dbg_msg.data[3] = static_cast<uint8_t>(stat_errors & 0xFF);
        dbg_msg.data[4] = op_mode;
        shared_memory.application_queue.push(dbg_msg);
        peak_fm_val = 0;
    }
}

void WMBusProcessor::consume_chip(uint8_t bit) {
    chip_reg = (chip_reg << 1) | bit;

    if (sync_state == SyncState::UNSYNCED) {
        if (op_mode == 0 || op_mode == 1) {  // T-Mode és C-Mode
            uint16_t sync_window = chip_reg & 0xFFFF;
            // 1 bit hibatűrés a szinkronszónál
            int err_norm = __builtin_popcount(sync_window ^ 0x543D);
            int err_inv = __builtin_popcount(sync_window ^ 0xABC2);

            if (err_norm <= 1 || err_inv <= 1) {
                inverted_iq = (err_inv <= 1);
                sync_state = SyncState::READ_LEN_H;
                chip_count = 0;
                payload_idx = 0;
                byte_half = false;
                stat_syncs++;
            }
        } else if (op_mode == 2) {  // S-Mode
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
        // === T-MODE (3-out-of-6) ===
        if (op_mode == 0) {
            chip_count++;
            if (chip_count == 6) {
                chip_count = 0;
                uint8_t raw = chip_reg & 0x3F;
                if (inverted_iq) raw = (~raw) & 0x3F;

                uint8_t nibble = decode_3out6(raw);
                if (nibble == 0xFF) {
                    sync_state = SyncState::UNSYNCED;
                    stat_errors++;
                    return;
                }

                if (sync_state == SyncState::READ_LEN_H || sync_state == SyncState::READ_DATA_H) {
                    current_byte = (nibble << 4);
                    sync_state = (sync_state == SyncState::READ_LEN_H) ? SyncState::READ_LEN_L : SyncState::READ_DATA_L;
                } else {
                    current_byte |= nibble;
                    handle_byte(current_byte);
                    if (sync_state != SyncState::UNSYNCED) {
                        sync_state = (sync_state == SyncState::READ_LEN_L) ? SyncState::READ_DATA_H : SyncState::READ_DATA_H;
                    }
                }
            }
        }
        // === C-MODE (NRZ) ===
        else if (op_mode == 1) {
            chip_count++;
            if (chip_count == 8) {
                chip_count = 0;
                uint8_t byte = chip_reg & 0xFF;
                if (inverted_iq) byte = ~byte;
                handle_byte(byte);
            }
        }
        // === S-MODE (Manchester) ===
        else if (op_mode == 2) {
            chip_count++;
            if (chip_count == 16) {
                chip_count = 0;
                uint8_t byte = 0;
                for (int b = 0; b < 8; b++) {
                    uint8_t pair = (chip_reg >> ((7 - b) * 2)) & 0x03;
                    if (inverted_iq) pair = (~pair) & 0x03;

                    if (pair == 0x02)
                        byte |= (1 << (7 - b));
                    else if (pair == 0x01) { /* 0 marad */
                    } else {
                        sync_state = SyncState::UNSYNCED;
                        stat_errors++;
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
            return;
        }

        physical_length = payload_length + 1 + 2;
        if (payload_length > 9) {
            uint8_t rem = payload_length - 9;
            physical_length += ((rem + 15) / 16) * 2;
        }

        payload_buffer[0] = byte;
        payload_idx = 1;
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
        }
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<WMBusProcessor>()};
    event_dispatcher.run();
    return 0;
}