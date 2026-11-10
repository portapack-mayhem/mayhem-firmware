/*
 * Copyright (C) 2026 justin080
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

/* No IPA-CP clones in this file: a clone of a library function (for example
 * std::string's constructor .constprop.0) is local to this object but carries
 * the library name, so the external app linker rule would not catch it and it
 * would land in the main firmware. */
#pragma GCC optimize("no-ipa-cp")

#include "ui_seewetter.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "file_path.hpp"
#include "string_format.hpp"
#include "portapack.hpp"

using namespace portapack;

namespace ui::external_app::seewetter {

/* ---------------- SeewetterSpectrum ---------------- */

SeewetterSpectrum::SeewetterSpectrum(Rect parent_rect)
    : Widget{parent_rect} {
}

void SeewetterSpectrum::update(const uint8_t* bins, int16_t centre_hz, uint16_t shift_hz, bool locked) {
    for (size_t i = 0; i < ::seewetter::SPECTRUM_BINS; i++) bins_[i] = bins[i];
    centre_hz_ = centre_hz;
    shift_hz_ = shift_hz;
    locked_ = locked;
    set_dirty();
}

int SeewetterSpectrum::hz_to_x(int32_t hz, int x0, int bin_w) const {
    // bin i covers (i - BINS/2) * 31.25 Hz
    const int32_t bin_x100 = (hz * 10000) / ::seewetter::SPECTRUM_HZ_PER_BIN_X100 +
                             (::seewetter::SPECTRUM_BINS / 2) * 100;
    return x0 + (int)((bin_x100 * bin_w) / 100) + bin_w / 2;
}

void SeewetterSpectrum::paint(Painter& painter) {
    const auto r = screen_rect();
    const int bins = ::seewetter::SPECTRUM_BINS;
    int bin_w = r.width() / bins;
    if (bin_w < 1) bin_w = 1;
    const int x0 = r.left() + (r.width() - bin_w * bins) / 2;
    const int h = r.height();
    const Color bg = Color::black();

    if (x0 > r.left()) {
        painter.fill_rectangle({r.left(), r.top(), x0 - r.left(), h}, bg);
        painter.fill_rectangle({x0 + bin_w * bins, r.top(), r.right() - (x0 + bin_w * bins), h}, bg);
    }

    for (int i = 0; i < bins; i++) {
        // value = (dB above noise + 3) * 6, 255 = +39.5 dB
        int bar = (bins_[i] * h) / 256;
        if (bar > h) bar = h;
        const int x = x0 + i * bin_w;
        if (h - bar > 0) painter.fill_rectangle({x, r.top(), bin_w, h - bar}, bg);
        if (bar > 0) {
            const Color c = (bins_[i] > 120) ? Color::yellow() : (bins_[i] > 60 ? Color::cyan() : Color::blue());
            painter.fill_rectangle({x, r.top() + h - bar, bin_w, bar}, c);
        }
    }

    // receive frequency (centre) tick
    const int xc = hz_to_x(0, x0, bin_w);
    painter.draw_vline({xc, r.top()}, 4, Color::grey());
    painter.draw_vline({xc, r.top() + h - 4}, 4, Color::grey());

    // the two tones the decoder listens on
    const Color mc = locked_ ? Color::green() : Color::red();
    const int32_t half = shift_hz_ / 2;
    for (int s = -1; s <= 1; s += 2) {
        const int x = hz_to_x(centre_hz_ + s * half, x0, bin_w);
        if (x >= r.left() && x < r.right())
            painter.draw_vline({x, r.top()}, h, mc);
    }
}

/* ---------------- SeewetterView ---------------- */

void SeewetterView::focus() {
    field_frequency.focus();
}

SeewetterView::SeewetterView(NavigationView& nav)
    : nav_{nav} {
    add_children({&field_frequency,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
                  &field_volume,
                  &labels,
                  &options_station,
                  &options_baud,
                  &options_shift,
                  &options_polarity,
                  &options_afc,
                  &options_squelch,
                  &options_usos,
                  &options_log,
                  &button_clear,
                  &spectrum,
                  &text_status,
                  &console});

    field_frequency.set_step(100);

    options_station.on_change = [this](size_t, OptionsField::value_t v) {
        if (v != 0) field_frequency.set_value(v);
    };
    field_frequency.updated = [this](rf::Frequency f) {
        options_station.set_by_value(f);
    };
    options_station.set_by_value(receiver_model.target_frequency());

    // restore saved settings without triggering a config for each field
    options_baud.set_by_value(baud_);
    options_shift.set_by_value(shift_);
    options_polarity.set_by_value(polarity_);
    options_afc.set_by_value(afc_);
    options_squelch.set_by_value(squelch_);
    options_usos.set_by_value(usos_);
    options_log.set_by_value(log_);
    decoder.set_usos(usos_ != 0);

    baseband::run_image(portapack::spi_flash::image_tag_seewetter);

    /* The receiver runs wide (same front end setup as the FT8 app) and the
     * baseband narrows it. set_modulation() is not called: it would replace the
     * baseband image. */
    receiver_model.set_sampling_rate(sampling_rate);
    receiver_model.set_baseband_bandwidth(baseband_bandwidth);
    receiver_model.enable();

    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();

    send_config();

    options_baud.on_change = [this](size_t, OptionsField::value_t v) {
        baud_ = v;
        send_config();
    };
    options_shift.on_change = [this](size_t, OptionsField::value_t v) {
        shift_ = v;
        send_config();
    };
    options_polarity.on_change = [this](size_t, OptionsField::value_t v) {
        polarity_ = v;
        send_config();
    };
    options_afc.on_change = [this](size_t, OptionsField::value_t v) {
        afc_ = v;
        send_config();
    };
    options_squelch.on_change = [this](size_t, OptionsField::value_t v) {
        squelch_ = v;
        send_config();
    };
    options_usos.on_change = [this](size_t, OptionsField::value_t v) {
        usos_ = v;
        decoder.set_usos(v != 0);
    };
    options_log.on_change = [this](size_t, OptionsField::value_t v) {
        log_ = v;
        set_logging(v != 0);
    };
    button_clear.on_select = [this](Button&) {
        console.clear(true);
    };

    set_logging(log_ != 0);
}

SeewetterView::~SeewetterView() {
    if (log_open && !log_line.empty()) log_file.write_entry(log_line);
    receiver_model.disable();
    baseband::shutdown();
    audio::output::stop();
}

/* Static and named, so it lives in the app's own data section. A local
 * RTTYDataMessage would be copied from an unnamed .rodata template. */
static RTTYDataMessage config_message{};

void SeewetterView::send_config() {
    RTTYDataMessage& message = config_message;
    message.stopbits = ::seewetter::KIND_CONFIG;
    message.baud = (uint16_t)baud_;
    message.shift = (uint16_t)shift_;
    message.data[0] = (uint8_t)polarity_;
    message.data[1] = (uint8_t)afc_;
    message.data[2] = (uint8_t)squelch_;
    message.data_len = 3;
    baseband::set_rtty_config(message);
}

void SeewetterView::set_logging(bool enable) {
    /* The file is opened once and stays open until the app is closed; switching
     * logging off only stops writing (re-opening the same File would leak it). */
    if (enable && !log_file_ready) {
        const auto error = log_file.append(logs_dir / u"SEEWETTER.TXT");
        log_file_ready = !error.is_valid();
    }
    if (!enable && log_open && !log_line.empty()) {
        log_file.write_entry(log_line);
        log_line.clear();
    }
    const bool was_open = log_open;
    log_open = enable && log_file_ready;
    if (log_open && !was_open)
        log_file.write_entry("--- Seewetter RX " + to_string_dec_uint(receiver_model.target_frequency() / 100) + "00 Hz ---");
}

void SeewetterView::on_message(const RTTYDataMessage* message) {
    if (message->stopbits == ::seewetter::KIND_TEXT)
        on_text(message);
    else if (message->stopbits == ::seewetter::KIND_STATUS)
        on_status(message);
}

void SeewetterView::on_text(const RTTYDataMessage* message) {
    std::string text;
    for (size_t i = 0; i < message->data_len && i < RTTYDataMessage::max_len; i++) {
        const char c = decoder.decode(message->data[i]);
        if (c == 0) continue;
        text += c;
        if (log_open) {
            if (c == '\n') {
                if (!log_line.empty()) log_file.write_entry(log_line);
                log_line.clear();
            } else if (log_line.size() < 200) {
                log_line += c;
            }
        }
    }
    if (!text.empty()) console.write(text);
}

void SeewetterView::on_status(const RTTYDataMessage* message) {
    const uint16_t flags = message->shift;
    const bool locked = flags & ::seewetter::FLAG_LOCKED;
    const bool afc_on = flags & ::seewetter::FLAG_AFC;
    const int16_t afc_hz = message->mark_tone;
    const int16_t centre_hz = message->space_tone;

    // markers: where the decoder listens (AFC on) or where the signal is (AFC off)
    spectrum.update(message->data, afc_on ? afc_hz : 0, (uint16_t)shift_, locked);

    std::string s;
    if (!locked) {
        s = "Suche...  ";
    } else {
        const int16_t shown = afc_on ? afc_hz : centre_hz;
        s = (afc_on ? "AFC" : "Abw") + std::string(shown >= 0 ? "+" : "-") +
            to_string_dec_uint(shown >= 0 ? shown : -shown) + "Hz ";
    }
    s += "Q" + to_string_dec_uint(message->baud) + "% ";
    s += (flags & ::seewetter::FLAG_INVERTED) ? "INV " : "NORM ";
    s += (flags & ::seewetter::FLAG_SQUELCH_OPEN) ? "RX" : "--";
    if (log_open) s += " LOG";

    const Style* style = Theme::getInstance()->fg_red;
    if (locked && (flags & ::seewetter::FLAG_SQUELCH_OPEN))
        style = Theme::getInstance()->fg_green;
    else if (locked)
        style = Theme::getInstance()->fg_yellow;
    text_status.set_style(style);
    text_status.set(s);
}

}  // namespace ui::external_app::seewetter
