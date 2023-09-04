/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "pocsag_app.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"
#include "string_format.hpp"
#include "utility.hpp"

using namespace portapack;
using namespace pocsag;
namespace pmem = portapack::persistent_memory;

void POCSAGLogger::log_raw_data(const pocsag::POCSAGPacket& packet, const uint32_t frequency) {
    std::string entry = "Raw: F:" + to_string_dec_uint(frequency) + "Hz " +
                        to_string_dec_uint(packet.bitrate()) + " Codewords:";

    // Raw hex dump of all the codewords
    for (size_t c = 0; c < 16; c++)
        entry += to_string_hex(packet[c], 8) + " ";

    log_file.write_entry(packet.timestamp(), entry);
}

void POCSAGLogger::log_decoded(Timestamp timestamp, const std::string& text) {
    log_file.write_entry(timestamp, text);
}

namespace ui {

POCSAGSettingsView::POCSAGSettingsView(
    NavigationView& nav,
    POCSAGSettings& settings)
    : settings_{settings} {
    add_children(
        {&check_beta,
         &check_log,
         &check_log_raw,
         &check_small_font,
         &check_hide_bad,
         &check_hide_addr_only,
         &check_ignore,
         &field_ignore,
         &button_save});

    check_beta.set_value(settings_.use_new_proc);
    check_log.set_value(settings_.enable_logging);
    check_log_raw.set_value(settings_.enable_raw_log);
    check_small_font.set_value(settings_.enable_small_font);
    check_hide_bad.set_value(settings_.hide_bad_data);
    check_hide_addr_only.set_value(settings_.hide_addr_only);
    check_ignore.set_value(settings_.enable_ignore);
    field_ignore.set_value(settings_.address_to_ignore);

    button_save.on_select = [this, &nav](Button&) {
        settings_.use_new_proc = check_beta.value();
        settings_.enable_logging = check_log.value();
        settings_.enable_raw_log = check_log_raw.value();
        settings_.enable_small_font = check_small_font.value();
        settings_.hide_bad_data = check_hide_bad.value();
        settings_.hide_addr_only = check_hide_addr_only.value();
        settings_.enable_ignore = check_ignore.value();
        settings_.address_to_ignore = field_ignore.value();

        nav.pop();
    };
}

POCSAGAppView::POCSAGAppView(NavigationView& nav)
    : nav_{nav} {
    if (settings_.use_new_proc)
        baseband::run_image(portapack::spi_flash::image_tag_pocsag2);
    else
        baseband::run_image(portapack::spi_flash::image_tag_pocsag);

    add_children(
        {&rssi,
         &audio,
         &field_rf_amp,
         &field_lna,
         &field_vga,
         &field_frequency,
         &field_squelch,
         &field_volume,
         &image_status,
         &text_packet_count,
         &widget_bits,
         &widget_frames,
         &button_ignore_last,
         &button_config,
         &console});

    // No app settings, use fallbacks.
    if (!app_settings_.loaded()) {
        field_frequency.set_value(initial_target_frequency);
        settings_.address_to_ignore = pmem::pocsag_ignore_address();
        settings_.enable_ignore = settings_.address_to_ignore > 0;
    }

    logger.append(LOG_ROOT_DIR "/POCSAG.TXT");

    field_squelch.set_value(receiver_model.squelch_level());
    field_squelch.on_change = [this](int32_t v) {
        receiver_model.set_squelch_level(v);
    };

    button_ignore_last.on_select = [this](Button&) {
        settings_.enable_ignore = true;
        settings_.address_to_ignore = last_address;
    };

    button_config.on_select = [this](Button&) {
        nav_.push<POCSAGSettingsView>(settings_);
        nav_.set_on_pop([this]() { refresh_ui(); });
    };

    refresh_ui();
    audio::output::start();
    receiver_model.enable();
    baseband::set_pocsag();
}

void POCSAGAppView::focus() {
    field_frequency.focus();
}

POCSAGAppView::~POCSAGAppView() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();

    // Save pmem settings. TODO: Even needed anymore?
    pmem::set_pocsag_ignore_address(settings_.address_to_ignore);
    pmem::set_pocsag_last_address(pocsag_state.address);  // For POCSAG TX.
}

void POCSAGAppView::refresh_ui() {
    console.set_style(
        settings_.enable_small_font
            ? &Styles::white_small
            : &Styles::white);
}

void POCSAGAppView::handle_decoded(Timestamp timestamp, const std::string& prefix) {
    bool bad_data = pocsag_state.errors >= 3;

    // Too many errors for reliable decode.
    if (bad_data && hide_bad_data()) {
        console.write("\n" STR_COLOR_MAGENTA + prefix + " Too many decode errors.");
        last_address = 0;
        return;
    }

    // Ignored address.
    if (ignore() && pocsag_state.address == settings_.address_to_ignore) {
        console.write("\n" STR_COLOR_CYAN + prefix + " Ignored: " + to_string_dec_uint(pocsag_state.address));
        last_address = pocsag_state.address;
        return;
    }

    // Color indicates the message has a lot of decoding errors.
    std::string color = bad_data ? STR_COLOR_MAGENTA : STR_COLOR_WHITE;

    std::string console_info = "\n" + color + prefix;
    console_info += " #" + to_string_dec_uint(pocsag_state.address);
    console_info += " F" + to_string_dec_uint(pocsag_state.function);

    if (pocsag_state.out_type == ADDRESS) {
        last_address = pocsag_state.address;

        if (!hide_addr_only()) {
            console.write(console_info);

            if (logging()) {
                logger.log_decoded(
                    timestamp,
                    to_string_dec_uint(pocsag_state.address) +
                        " F" + to_string_dec_uint(pocsag_state.function));
            }
        }

    } else if (pocsag_state.out_type == MESSAGE) {
        if (pocsag_state.address != last_address) {
            // New message
            last_address = pocsag_state.address;
            console.writeln(console_info);
            console.write(color + pocsag_state.output);
        } else {
            // Message continues...
            console.write(color + pocsag_state.output);
        }

        if (logging()) {
            logger.log_decoded(
                timestamp,
                to_string_dec_uint(pocsag_state.address) +
                    " F" + to_string_dec_uint(pocsag_state.function) +
                    " " + pocsag_state.output);
        }
    }
}

static Color get_status_color(const POCSAGState& state) {
    if (state.out_type == IDLE)
        return Color::white();

    switch (state.mode) {
        case STATE_CLEAR:
            return Color::cyan();
        case STATE_HAVE_ADDRESS:
            return Color::yellow();
        case STATE_GETTING_MSG:
            return Color::green();
    }

    // Shouldn't get here...
    return Color::red();
}

void POCSAGAppView::on_packet(const POCSAGPacketMessage* message) {
    const uint32_t roundVal = 50;
    const uint32_t bitrate_rounded = roundVal * ((message->packet.bitrate() + (roundVal / 2)) / roundVal);
    auto bitrate = to_string_dec_uint(bitrate_rounded);
    auto timestamp = to_string_datetime(message->packet.timestamp(), HM);
    auto prefix = timestamp + " " + bitrate;

    // Display packet count to be able to tell whether baseband sent a packet for a tone.
    ++packet_count;
    text_packet_count.set(to_string_dec_uint(packet_count));

    if (logging_raw())
        logger.log_raw_data(message->packet, receiver_model.target_frequency());

    if (message->packet.flag() != NORMAL) {
        console.writeln("\n" STR_COLOR_RED + prefix + " CRC ERROR: " + pocsag::flag_str(message->packet.flag()));
        last_address = 0;
    } else {
        // Set color before to be able to see if decode gets stuck.
        image_status.set_foreground(Color::magenta());
        pocsag_state.codeword_index = 0;
        pocsag_state.errors = 0;

        // Handle multiple messages (if any).
        while (pocsag_decode_batch(message->packet, pocsag_state))
            handle_decoded(message->packet.timestamp(), prefix);

        // Handle the remainder.
        handle_decoded(message->packet.timestamp(), prefix);
    }

    // Set status icon color to indicate state machine state.
    image_status.set_foreground(get_status_color(pocsag_state));
}

void POCSAGAppView::on_stats(const POCSAGStatsMessage* stats) {
    widget_bits.set_bits(stats->current_bits);
    widget_frames.set_frames(stats->current_frames);
    widget_frames.set_sync(stats->has_sync);
}

void BitsIndicator::paint(Painter&) {
    auto p = screen_pos();
    for (size_t i = 0; i < sizeof(bits_) * 8; ++i) {
        auto is_set = ((bits_ >> i) & 0x1) == 1;

        int x = p.x() + (i / height);
        int y = p.y() + (i % height);
        display.draw_pixel({x, y}, is_set ? Color::white() : Color::black());
    }
}

void FrameIndicator::paint(Painter& painter) {
    auto p = screen_pos();
    painter.draw_rectangle({p, {2, height}}, has_sync_ ? Color::green() : Color::grey());

    for (size_t i = 0; i < height; ++i) {
        auto p2 = p + Point{2, 16 - (int)i};
        painter.draw_hline(p2, 2, i < frame_count_ ? Color::white() : Color::black());
    }
}

} /* namespace ui */
