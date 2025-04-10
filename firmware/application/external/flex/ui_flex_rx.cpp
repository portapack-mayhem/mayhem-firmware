/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui_flex_rx.hpp"
#include "audio.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "file_path.hpp"
#include "portapack.hpp"
#include "portapack_shared_memory.hpp"

namespace ui::external_app::flex_rx {

void FlexLogger::log_decoded(const Timestamp& timestamp, const std::string& data) {
    log_file.write_entry(timestamp, data);
}

void FlexRxView::focus() {
    field_frequency.focus();
}

FlexRxView::FlexRxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&rssi, &channel, &field_rf_amp, &field_lna, &field_vga,
                  &field_volume, &field_frequency, &check_log, &text_debug,
                  &console, &sync_status});

    field_frequency.set_value(929614000);
    field_frequency.set_step(100);

    check_log.set_value(logging);
    check_log.on_select = [this](Checkbox&, bool v) {
        logging = v;
    };

    logger = std::make_unique<FlexLogger>();
    if (logger)
        logger->append(logs_dir / u"FLEX.TXT");

    baseband::set_flex();

    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();

    portapack::receiver_model.enable();
}

FlexRxView::~FlexRxView() {
    audio::output::stop();
    portapack::receiver_model.disable();
    baseband::shutdown();
}

uint8_t diff_bit_count(uint32_t left, uint32_t right) {
    uint32_t diff = left ^ right;
    uint8_t count = 0;
    for (size_t i = 0; i < sizeof(diff) * 8; ++i) {
        if (((diff >> i) & 0x1) == 1)
            ++count;
    }
    return count;
}

void FlexRxView::on_packet(const FlexPacketMessage& message) {
    const auto& packet = message.packet;
    const auto& batch = packet.batch();
    auto timestamp = to_string_datetime(packet.timestamp(), HM);
    auto bitrate = packet.bitrate();
    std::string str_log = timestamp + " " + to_string_dec_uint(bitrate) + " ";

    if (batch.size() < 8) return; // Ensure full block

    uint32_t frame_info = batch[0];
    size_t vsa = (frame_info >> 10) & 0x3F;
    size_t asa = ((frame_info >> 8) & 0x03) + 1;

    for (size_t j = asa; j < vsa && j < batch.size(); j++) {
        uint32_t address_word = batch[j];
        uint32_t data = address_word >> 11;
        uint32_t parity = address_word & 0x7FF;

        uint32_t syndrome = 0;
        uint32_t temp = data;
        for (int k = 0; k < 21; ++k) {
            if (temp & 1) syndrome ^= (0x7FF >> k);
            temp >>= 1;
        }
        syndrome ^= parity;

        if (syndrome != 0) {
            uint32_t error_pos = 0;
            bool correctable = false;
            for (int k = 0; k < 10; ++k) {
                if (syndrome == (static_cast<uint32_t>(1) << k)) {
                    error_pos = k;
                    correctable = true;
                    break;
                }
            }
            if (correctable && error_pos < 21) {
                data ^= (1 << error_pos);
            } else {
                continue;
            }
        }

        bool long_address = (data & 0x7FFFF) < 0x008001 || ((data & 0x7FFFF) > 0x1E0000 && (data & 0x7FFFF) < 0x1F0001) || (data & 0x7FFFF) > 0x1F7FFE;
        size_t vb = vsa + j - asa;
        if (vb >= batch.size()) continue;

        uint32_t vector_word = batch[vb];
        uint32_t vector_data = vector_word >> 11;
        uint32_t vector_parity = vector_word & 0x7FF;

        syndrome = 0;
        temp = vector_data;
        for (int k = 0; k < 21; ++k) {
            if (temp & 1) syndrome ^= (0x7FF >> k);
            temp >>= 1;
        }
        syndrome ^= vector_parity;

        if (syndrome != 0) {
            uint32_t error_pos = 0;
            bool correctable = false;
            for (int k = 0; k < 10; ++k) {
                if (syndrome == (static_cast<uint32_t>(1) << k)) {
                    error_pos = k;
                    correctable = true;
                    break;
                }
            }
            if (correctable && error_pos < 21) {
                vector_data ^= (1 << error_pos);
            } else {
                continue;
            }
        }

        uint8_t vector_type = (vector_data >> 4) & 0x07;
        if (long_address && j + 1 >= batch.size()) continue;

        std::string display_text;
        uint32_t address;
        if (!long_address) {
            address = (data & 0x7FFFF) - 32768;
            display_text = to_string_dec_uint(address);
        } else {
            uint32_t second_word = batch[j + 1];
            uint32_t second_data = second_word >> 11;
            syndrome = 0;
            temp = second_data;
            for (int k = 0; k < 21; ++k) {
                if (temp & 1) syndrome ^= (0x7FF >> k);
                temp >>= 1;
            }
            syndrome ^= (second_word & 0x7FF);
            if (syndrome == 0) {
                address = ((second_data ^ 0x1FFFFF) << 15) + 2068480 + (data & 0x7FFFF);
                display_text = to_string_dec_uint(address);
            } else {
                continue;
            }
            j++;
        }

        console.writeln(display_text);
        if (logger && logging) logger->log_decoded(packet.timestamp(), str_log + display_text);

        if (vector_type == 5) { // Alpha message
            int w1 = (vector_data >> 7) & 0x7F;
            int w2 = ((vector_data >> 14) & 0x7F) + w1 - 1;
            if (w1 < 0 || w2 < 0 || static_cast<size_t>(w1) >= batch.size() || static_cast<size_t>(w2) >= batch.size()) continue;

            std::string message_text;
            for (int k = w1; k <= w2; k++) {
                uint32_t msg_word = batch[k];
                data = msg_word >> 11;
                parity = msg_word & 0x7FF;

                syndrome = 0;
                temp = data;
                for (int m = 0; m < 21; ++m) {
                    if (temp & 1) syndrome ^= (0x7FF >> m);
                    temp >>= 1;
                }
                syndrome ^= parity;

                if (syndrome != 0) {
                    uint32_t error_pos = 0;
                    bool correctable = false;
                    for (int m = 0; m < 10; ++m) {
                        if (syndrome == (static_cast<uint32_t>(1) << m)) {
                            error_pos = m;
                            correctable = true;
                            break;
                        }
                    }
                    if (correctable && error_pos < 21) {
                        data ^= (1 << error_pos);
                    } else {
                        continue;
                    }
                }

                for (int m = 0; m < 20; m += 7) {
                    uint8_t byte = (data >> (13 - m)) & 0x7F;
                    if (byte >= 32 && byte < 127) message_text += (char)byte;
                }
            }
            if (!message_text.empty()) {
                console.writeln(message_text);
                if (logger && logging) logger->log_decoded(packet.timestamp(), str_log + message_text);
            }
        }
    }
    console_color++;
}

void FlexRxView::on_stats(const FlexStatsMessage& stats) {
    if (stats.has_sync != last_has_sync) {
        std::string str_console = "\x1B\x31";
        if (stats.has_sync) {
            str_console += "SYNC ACQUIRED - Baud: " + to_string_dec_uint(stats.baud_rate);
        } else if (last_has_sync) {
            str_console += "SYNC LOST";
        }
        console.writeln(str_console);
        
        sync_status.set_color(stats.has_sync ? Color::green() : Color::red());
    }
    
    if (stats.baud_rate != last_baud_rate || 
        stats.has_sync != last_has_sync ||
        stats.current_frames != last_frames ||
        stats.current_bits != last_bits) {
        
        std::string debug_text = "BR:" + to_string_dec_uint(stats.baud_rate) +
                                " Sync:" + (stats.has_sync ? "Y" : "N") +
                                " Fr:" + to_string_dec_uint(stats.current_frames);
        text_debug.set(debug_text);
        
        last_baud_rate = stats.baud_rate;
        last_has_sync = stats.has_sync;
        last_frames = stats.current_frames;
        last_bits = stats.current_bits;
    }
}

void FlexRxView::on_freqchg(int64_t freq) {
    field_frequency.set_value(freq);
}

}  // namespace ui::external_app::flex_rx