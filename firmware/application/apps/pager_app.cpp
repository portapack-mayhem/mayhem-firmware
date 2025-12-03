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

#include "rtc_time.hpp"
#include "pager_app.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"
#include "string_format.hpp"
#include "utility.hpp"
#include "file_path.hpp"
#include "usb_serial_asyncmsg.hpp"

using namespace portapack;
using namespace pocsag;
namespace pmem = portapack::persistent_memory;

void PagerLogger::log_raw_data(const pocsag::POCSAGPacket& packet, const uint32_t frequency) {
    std::string entry = "Raw: F:" + to_string_dec_uint(frequency) + "Hz " +
                        to_string_dec_uint(packet.bitrate()) + " Codewords:";

    // Raw hex dump of all the codewords
    for (size_t c = 0; c < 16; c++)
        entry += to_string_hex(packet[c], 8) + " ";

    log_file.write_entry(packet.timestamp(), entry);
}

void PagerLogger::log_decoded(Timestamp timestamp, const std::string& text) {
    log_file.write_entry(timestamp, text);
}

namespace ui {

PagerSettingsView::PagerSettingsView(
    NavigationView& nav,
    PagerSettings& settings)
    : settings_{settings} {
    add_children(
        {&labels,
         &opt_mode,
         &opt_baud_rate,
         &check_log,
         &check_log_raw,
         &check_small_font,
         &check_hide_bad,
         &check_hide_addr_only,
         &opt_filter_mode,
         &field_filter_address,
         &button_save});

    opt_mode.set_selected_index(settings_.pager_mode);
    opt_baud_rate.set_by_value(settings_.baud_rate);
    check_log.set_value(settings_.enable_logging);
    check_log_raw.set_value(settings_.enable_raw_log);
    check_small_font.set_value(settings_.enable_small_font);
    check_hide_bad.set_value(settings_.hide_bad_data);
    check_hide_addr_only.set_value(settings_.hide_addr_only);
    opt_filter_mode.set_by_value(settings_.filter_mode);
    field_filter_address.set_value(settings_.filter_address);

    button_save.on_select = [this, &nav](Button&) {
        settings_.pager_mode = opt_mode.selected_index();
        settings_.enable_logging = check_log.value();
        settings_.enable_raw_log = check_log_raw.value();
        settings_.enable_small_font = check_small_font.value();
        settings_.hide_bad_data = check_hide_bad.value();
        settings_.hide_addr_only = check_hide_addr_only.value();
        settings_.filter_mode = opt_filter_mode.selected_index_value();
        settings_.filter_address = field_filter_address.to_integer();
        settings_.baud_rate = opt_baud_rate.selected_index_value();
        nav.pop();
    };
}

PagerAppView::PagerAppView(NavigationView& nav)
    : nav_{nav} {
    
    // Load the new unified baseband image
    // baseband::run_image(portapack::spi_flash::image_tag_pager);
    baseband::run_image_from_file(u"PPAG.bin");

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
         &widget_baud,
         &widget_bits,
         &widget_frames,
         &button_filter_last,
         &button_config,
         &console});

    // No app settings, use fallbacks from pmem.
    if (!app_settings_.loaded()) {
        settings_.filter_address = pmem::pocsag_ignore_address();
        settings_.filter_mode = (settings_.filter_address == 0)
                                    ? FILTER_NONE
                                    : FILTER_DROP;
        settings_.pager_mode = AUTO; // Default
    }

    logger.append(logs_dir / u"PAGER.TXT");

    field_squelch.set_value(receiver_model.squelch_level());
    field_squelch.on_change = [this](int32_t v) {
        receiver_model.set_squelch_level(v);
    };

    button_filter_last.on_select = [this](Button&) {
        if (settings_.filter_mode == FILTER_NONE)
            settings_.filter_mode = FILTER_DROP;
        settings_.filter_address = last_address;
        refresh_ui();
    };

    button_config.on_select = [this](Button&) {
        nav_.push<PagerSettingsView>(settings_);
        nav_.set_on_pop([this]() { refresh_ui(); });
    };

    refresh_ui();

    if (pmem::beep_on_packets())
        audio::set_rate(audio::Rate::Hz_24000);

    audio::output::start();
    receiver_model.enable();
    
    apply_config();
}

void PagerAppView::focus() {
    field_frequency.focus();
}

PagerAppView::~PagerAppView() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();

    // Save pmem settings.
    pmem::set_pocsag_ignore_address(settings_.filter_address);
    pmem::set_pocsag_last_address(last_address);  // For POCSAG TX.
}

void PagerAppView::apply_config() {
    if (settings_.pager_mode == AUTO) {
        // Send a magic baud rate -2 to indicate AUTO mode to PagerProcessor
        baseband::set_pocsag(-2); 
    } else if (settings_.pager_mode == POCSAG) {
        baseband::set_pocsag((int8_t)settings_.baud_rate);
    } else {
        baseband::set_flex_config(); // Helper function or custom message
        // baseband::set_flex_config() calls FlexConfigure (ID 84)
    }
}

void PagerAppView::refresh_ui() {
    // Set console font style.
    console.set_style(
        settings_.enable_small_font
            ? Theme::getInstance()->bg_darkest_small
            : Theme::getInstance()->bg_darkest);

    // Update filter button text.
    std::string btn_text = "Filter Last";
    switch (settings_.filter_mode) {
        case FILTER_DROP:
            btn_text = "Ignore Last";
            break;

        case FILTER_KEEP:
            btn_text = "Keep Last";
            break;

        case FILTER_NONE:
        default:
            btn_text = "Filter Last";
            break;
    }
    
    apply_config();
    button_filter_last.set_text(btn_text);
    
    // Update Frequency if needed based on mode?
    // For now assuming user sets frequency manually or freqman
}

bool PagerAppView::ignore_address(uint32_t address) const {
    switch (settings_.filter_mode) {
        case FILTER_DROP:
            return address == settings_.filter_address;

        case FILTER_KEEP:
            return address != settings_.filter_address;

        case FILTER_NONE:
        default:
            return false;
    }
}

void PagerAppView::handle_decoded(Timestamp timestamp, const std::string& prefix) {
    bool bad_data = pocsag_state.errors >= 3;

    // Too many errors for reliable decode.
    if (bad_data && hide_bad_data()) {
        console.write("\n" STR_COLOR_MAGENTA + prefix + " Too many decode errors.");
        last_address = 0;
        return;
    }

    // Ignore address? TODO: could filter earlier.
    if (ignore_address(pocsag_state.address)) {
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
        return Theme::getInstance()->bg_darkest->foreground;

    switch (state.mode) {
        case STATE_CLEAR:
            return Theme::getInstance()->fg_cyan->foreground;
        case STATE_HAVE_ADDRESS:
            return Theme::getInstance()->fg_yellow->foreground;
        case STATE_GETTING_MSG:
            return Theme::getInstance()->fg_green->foreground;
    }

    return Theme::getInstance()->fg_red->foreground;
}

void PagerAppView::on_packet_pocsag(const POCSAGPacketMessage* message) {
    if (settings_.pager_mode != POCSAG && settings_.pager_mode != AUTO) return;

    const uint32_t roundVal = 50;
    const uint32_t bitrate_rounded = roundVal * ((message->packet.bitrate() + (roundVal / 2)) / roundVal);
    auto bitrate = to_string_dec_uint(bitrate_rounded);
    auto timestamp = to_string_datetime(message->packet.timestamp(), HM);
    auto prefix = timestamp + " " + bitrate;

    // Display packet count
    ++packet_count;
    text_packet_count.set(to_string_dec_uint(packet_count));

    if (logging_raw())
        logger.log_raw_data(message->packet, receiver_model.target_frequency());

    if (message->packet.flag() != NORMAL) {
        console.writeln("\n" STR_COLOR_RED + prefix + " CRC ERROR: " + pocsag::flag_str(message->packet.flag()));
        last_address = 0;
    } else {
        image_status.set_foreground(Theme::getInstance()->fg_magenta->foreground);
        pocsag_state.codeword_index = 0;
        pocsag_state.errors = 0;

        // Handle multiple messages
        while (pocsag_decode_batch(message->packet, pocsag_state))
            handle_decoded(message->packet.timestamp(), prefix);

        handle_decoded(message->packet.timestamp(), prefix);
    }

    image_status.set_foreground(get_status_color(pocsag_state));

    if (pmem::beep_on_packets()) {
        baseband::request_audio_beep(1000, 24000, 60);
    }
}

void PagerAppView::on_stats_pocsag(const POCSAGStatsMessage* stats) {
    if (settings_.pager_mode != POCSAG && settings_.pager_mode != AUTO) return;
    widget_baud.set_rate(stats->baud_rate);
    widget_bits.set_bits(stats->current_bits);
    widget_frames.set_frames(stats->current_frames);
    widget_frames.set_sync(stats->has_sync);
}

void PagerAppView::on_packet_flex(const FlexPacketMessage* message) {
    if (settings_.pager_mode != FLEX && settings_.pager_mode != AUTO) return;

    // Display packet count
    ++packet_count;
    text_packet_count.set(to_string_dec_uint(packet_count));

    std::string text = "\n";
    text += STR_COLOR_GREEN;
    text += "FLEX ";
    text += to_string_dec_uint(message->packet.bitrate);
    text += " #";
    text += to_string_dec_uint(message->packet.capcode);
    text += ": ";
    text += STR_COLOR_WHITE;
    text += message->packet.message;
    
    console.writeln(text);
    
    if (logging()) {
        logger.log_decoded(rtc_time::now(), text);
    }

    if (pmem::beep_on_packets()) {
        baseband::request_audio_beep(1000, 24000, 60);
    }
}

void PagerAppView::on_debug_flex(const FlexDebugMessage* message) {
    if (settings_.pager_mode != FLEX && settings_.pager_mode != AUTO) return;

    // Build debug string but do NOT write it to the on-screen console.
    std::string text;
    text += "DBG: ";
    text += message->text;
    text += " ";
    text += to_string_hex(message->val1, 8);
    text += " ";
    text += to_string_hex(message->val2, 8);

    // Send FLEX debug messages only to async serial output when enabled.
    UsbSerialAsyncmsg::asyncmsg(text);
}

void PagerAppView::on_freqchg(int64_t freq) {
    field_frequency.set_value(freq);
}

void BaudIndicator::paint(Painter& painter) {
    auto p = screen_pos();
    char top = '-';
    char bot = '-';

    if (rate_ > 0) {
        auto r = rate_ / 100;
        top = (r / 10) + '0';
        bot = (r % 10) + '0';
    }

    painter.draw_char(p, *Theme::getInstance()->bg_darkest_small, top);
    painter.draw_char({p.x(), p.y() + 8}, *Theme::getInstance()->bg_darkest_small, bot);
}

void BitsIndicator::paint(Painter&) {
    auto p = screen_pos();
    for (size_t i = 0; i < sizeof(bits_) * 8; ++i) {
        auto is_set = ((bits_ >> i) & 0x1) == 1;

        int x = p.x() + (i / height);
        int y = p.y() + (i % height);
        display.draw_pixel({x, y}, is_set ? Theme::getInstance()->bg_darkest->foreground : Theme::getInstance()->bg_darkest->background);
    }
}

void FrameIndicator::paint(Painter& painter) {
    auto p = screen_pos();
    painter.draw_rectangle({p, {2, height}}, has_sync_ ? Theme::getInstance()->fg_green->foreground : Theme::getInstance()->bg_medium->background);

    for (size_t i = 0; i < height; ++i) {
        auto p2 = p + Point{2, 15 - (int)i};
        painter.draw_hline(p2, 2, i < frame_count_ ? Theme::getInstance()->bg_darkest->foreground : Theme::getInstance()->bg_darkest->background);
    }
}

} /* namespace ui */

