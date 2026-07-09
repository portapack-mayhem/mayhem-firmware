#include "proc_wpan.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"

WPANProcessor::WPANProcessor() {
    baseband_thread.start();
}

void WPANProcessor::execute(const buffer_c8_t& buffer) {
    // A nyers DMA puffer közvetlen olvasása, 0 darab másolás!
    const auto* p = buffer.p;
    const size_t count = buffer.count;

    for (size_t i = 0; i < count; i++) {
        int8_t i_samp = p[i].real();
        int8_t q_samp = p[i].imag();

        // 1. MSK Demoduláció (Keresztszorzás), teljesen Integer alapokon
        int32_t fm_val = (q_samp * prev_i) - (i_samp * prev_q);
        prev_i = i_samp;
        prev_q = q_samp;

        fm_sum += fm_val;
        phase_timer++;

        // 2. Belső Decimálás: 4 MHz -> 2 Mchip/s
        if (phase_timer == 2) {
            phase_timer = 0;

            // Két minta összege dönti el a chip bitjét (Zajszűrés)
            uint8_t chip_bit = (fm_sum > 0) ? 1 : 0;
            fm_sum = 0;

            chip_shift_reg = (chip_shift_reg << 1) | chip_bit;

            // 3. Ultra-Gyors DSSS Korrelátor & MAC Állapotgép
            if (sync_state == SyncState::UNSYNCED) {
                // Csak a Preambulumot (0-s szimbólum) keressük, 5 hibát engedélyezve a 32 bitből
                uint32_t diff = chip_shift_reg ^ SYMBOL_TABLE[0];
                int err = __builtin_popcount(diff);
                if (err <= 5 || err >= 27) {  // 27 jelenti, hogy az invertált hiba <= 5
                    sync_state = SyncState::EXPECT_SFD;
                    chip_countdown = 32;
                    sfd_half = false;
                    byte_half = false;
                }
            } else {
                chip_countdown--;
                if (chip_countdown == 0) {
                    chip_countdown = 32;

                    int best_sym = -1;
                    int min_err = 32;

                    // Szimbólum keresés
                    for (int s = 0; s < 16; s++) {
                        int err = __builtin_popcount(chip_shift_reg ^ SYMBOL_TABLE[s]);
                        if (err < min_err) {
                            min_err = err;
                            best_sym = s;
                        }
                        int inv_err = 32 - err;
                        if (inv_err < min_err) {
                            min_err = inv_err;
                            best_sym = s;
                        }
                    }

                    if (min_err <= 6) {
                        // MAC Réteg (Beágyazva a zéró ugrás érdekében)
                        if (sync_state == SyncState::EXPECT_SFD) {
                            if (best_sym == 0) {
                                // Még mindig a Preambulum jön
                            } else if (best_sym == 7) {
                                sfd_half = true;
                            } else if (sfd_half && best_sym == 10) {
                                sync_state = SyncState::EXPECT_LENGTH;
                                sfd_half = false;
                                byte_half = false;
                            } else {
                                sync_state = SyncState::UNSYNCED;
                            }
                        } else if (sync_state == SyncState::EXPECT_LENGTH) {
                            if (!byte_half) {
                                current_byte = best_sym;
                                byte_half = true;
                            } else {
                                current_byte |= (best_sym << 4);
                                payload_length = current_byte & 0x7F;  // Max 127 byte

                                if (payload_length == 0 || payload_length > 127) {
                                    sync_state = SyncState::UNSYNCED;
                                } else {
                                    payload_idx = 0;
                                    sync_state = SyncState::EXPECT_PAYLOAD;
                                }
                                byte_half = false;
                            }
                        } else if (sync_state == SyncState::EXPECT_PAYLOAD) {
                            if (!byte_half) {
                                current_byte = best_sym;
                                byte_half = true;
                            } else {
                                current_byte |= (best_sym << 4);
                                payload_buffer[payload_idx++] = current_byte;
                                byte_half = false;

                                if (payload_idx >= payload_length) {
                                    // CSOMAG KÉSZ! Kiküldés a UI-nak.
                                    WPANPacketMessage msg;
                                    msg.length = payload_length;
                                    for (int k = 0; k < payload_length; k++) {
                                        msg.data[k] = payload_buffer[k];
                                    }
                                    shared_memory.application_queue.push(msg);
                                    sync_state = SyncState::UNSYNCED;
                                }
                            }
                        }
                    } else {
                        // Túl nagy a zaj, elvesztettük a jelet
                        sync_state = SyncState::UNSYNCED;
                    }
                }
            }
        }
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<WPANProcessor>()};
    event_dispatcher.run();
    return 0;
}