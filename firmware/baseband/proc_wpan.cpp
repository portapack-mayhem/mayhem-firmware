#include "proc_wpan.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"

WPANProcessor::WPANProcessor() {
    demod_fm.configure(baseband_fs, 500000);
    baseband_thread.start();
}

void WPANProcessor::execute(const buffer_c8_t& buffer) {
    // --- JAVÍTVA: Biztonsági fék a Hard Fault (fagyás) ellen ---
    size_t count = std::min(buffer.count, c16_buf.size());

    for (size_t i = 0; i < count; i++) {
        c16_buf[i] = {
            (int16_t)(buffer.p[i].real() << 8),
            (int16_t)(buffer.p[i].imag() << 8)};
    }

    buffer_c16_t c16_view{c16_buf.data(), count};
    buffer_s16_t fm_view{fm_buf.data(), count};

    demod_fm.execute(c16_view, fm_view);
    feed_channel_stats(c16_view);

    for (size_t i = 0; i < count; i++) {
        float chip_soft = (float)fm_buf[i] / 4000.0f;
        clock_recovery(chip_soft);
    }
}

void WPANProcessor::consume_chip(float chip_soft) {
    uint8_t chip_bit = (chip_soft > 0.0f) ? 1 : 0;

    chip_shift_reg = (chip_shift_reg << 1) | chip_bit;

    if (chip_countdown > 0) {
        chip_countdown--;
        if (chip_countdown == 0) {
            correlate_symbol();
        }
    } else {
        correlate_symbol();
    }
}

void WPANProcessor::correlate_symbol() {
    int best_symbol = -1;
    int min_errors = 32;

    // --- BRUTÁLIS CPU OPTIMALIZÁCIÓ ---
    // Ha nem vagyunk szinkronban, csak a nullás szimbólumot (Preambulum) keressük a zajban!
    // Ezzel megmentjük a processzort a túlterheléstől és az esetleges megakadásoktól.
    int search_max = (sync_state == SyncState::UNSYNCED) ? 1 : 16;

    for (int i = 0; i < search_max; i++) {
        uint32_t diff = chip_shift_reg ^ SYMBOL_TABLE[i];
        int errors = __builtin_popcount(diff);

        if (errors < min_errors) {
            min_errors = errors;
            best_symbol = i;
        }

        int inv_errors = 32 - errors;
        if (inv_errors < min_errors) {
            min_errors = inv_errors;
            best_symbol = i;
        }
    }

    if (min_errors <= 6) {
        chip_countdown = 32;

        if (sync_state == SyncState::UNSYNCED) {
            if (best_symbol == 0) {
                preamble_count++;
                if (preamble_count >= 3) {
                    sync_state = SyncState::EXPECT_SFD;
                }
            } else {
                preamble_count = 0;
            }
        } else {
            process_symbol(best_symbol);
        }
    } else {
        if (chip_countdown == 0) {
            sync_state = SyncState::UNSYNCED;
            preamble_count = 0;
        }
    }
}

void WPANProcessor::process_symbol(uint8_t symbol) {
    switch (sync_state) {
        case SyncState::EXPECT_SFD:
            if (symbol == 7) {
                sfd_half = true;
            } else if (sfd_half && symbol == 10) {
                sync_state = SyncState::EXPECT_LENGTH;
                sfd_half = false;
                byte_half = false;
            } else if (symbol == 0) {
                sfd_half = false;
            } else {
                sync_state = SyncState::UNSYNCED;
            }
            break;

        case SyncState::EXPECT_LENGTH:
            if (!byte_half) {
                current_byte = symbol;
                byte_half = true;
            } else {
                current_byte |= (symbol << 4);
                payload_length = current_byte & 0x7F;

                if (payload_length == 0 || payload_length > 127) {
                    sync_state = SyncState::UNSYNCED;
                } else {
                    payload_idx = 0;
                    sync_state = SyncState::EXPECT_PAYLOAD;
                }
                byte_half = false;
            }
            break;

        case SyncState::EXPECT_PAYLOAD:
            if (!byte_half) {
                current_byte = symbol;
                byte_half = true;
            } else {
                current_byte |= (symbol << 4);
                payload_buffer[payload_idx++] = current_byte;
                byte_half = false;

                if (payload_idx >= payload_length) {
                    WPANPacketMessage msg;
                    msg.length = payload_length;

                    for (int i = 0; i < payload_length; i++) {
                        msg.data[i] = payload_buffer[i];
                    }

                    shared_memory.application_queue.push(msg);
                    sync_state = SyncState::UNSYNCED;
                }
            }
            break;

        default:
            sync_state = SyncState::UNSYNCED;
            break;
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<WPANProcessor>()};
    event_dispatcher.run();
    return 0;
}