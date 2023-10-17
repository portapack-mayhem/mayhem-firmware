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

#include "ui_encoders.hpp"

#include "baseband_api.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui {

EncodersConfigView::EncodersConfigView(
    NavigationView&,
    Rect parent_rect) {
    using option_t = std::pair<std::string, int32_t>;
    std::vector<option_t> enc_options;
    size_t i;

    set_parent_rect(parent_rect);
    hidden(true);

    // Default encoder def
    encoder_def = &encoder_defs[0];

    add_children({&labels,
                  &options_enctype,
                  &field_repeat_min,
                  &field_clk,
                  &field_clk_step,
                  &field_frameduration,
                  &field_frameduration_step,
                  &text_format,
                  &waveform});

    // Load encoder types in option field
    for (i = 0; i < ENC_TYPES_COUNT; i++)
        enc_options.emplace_back(std::make_pair(encoder_defs[i].name, i));

    options_enctype.on_change = [this](size_t index, int32_t) {
        on_type_change(index);
    };

    options_enctype.set_options(enc_options);
    options_enctype.set_selected_index(0);

    // Selecting input clock changes symbol and word duration
    field_clk.on_change = [this](int32_t value) {
        // value is in kHz, new_value is in us
        int32_t new_value = (encoder_def->clk_per_symbol * 1000000) / (value * 1000);
        if (new_value != field_frameduration.value())
            field_frameduration.set_value(new_value * encoder_def->word_length, false);
    };

    field_clk_step.on_change = [this](size_t, int32_t value) {
        field_clk.set_step(value);
    };

    // Selecting word duration changes input clock and symbol duration
    field_frameduration.on_change = [this](int32_t value) {
        // value is in us, new_value is in kHz
        int32_t new_value = (value * 1000) / (encoder_def->word_length * encoder_def->clk_per_symbol);
        if (new_value != field_clk.value())
            field_clk.set_value(1000000 / new_value, false);
    };

    field_frameduration_step.on_change = [this](size_t, int32_t value) {
        field_frameduration.set_step(value);
    };
}

void EncodersConfigView::focus() {
    options_enctype.focus();
}

void EncodersConfigView::on_type_change(size_t index) {
    // Remove existing SymField controls.
    for (auto& symfield : symfields_word)
        remove_child(symfield.get());
    symfields_word.clear();

    encoder_def = &encoder_defs[index];
    field_clk.set_value(encoder_def->default_speed / 1000);
    field_repeat_min.set_value(encoder_def->repeat_min);

    // Add new SymFields.
    Point pos{2 * 8, 9 * 8};
    std::string format_string;
    uint8_t word_length = encoder_def->word_length;
    auto on_change_handler = [this](SymField&) {
        generate_frame();
    };

    for (uint8_t i = 0; i < word_length; i++) {
        auto symbol_type = encoder_def->word_format[i];
        symfields_word.push_back(std::make_unique<SymField>(pos, 1));
        auto& symfield = symfields_word.back();
        symfield->on_change = on_change_handler;

        switch (symbol_type) {
            case 'A':
                symfield->set_symbol_list(encoder_def->address_symbols);
                format_string += 'A';
                break;
            case 'D':
                symfield->set_symbol_list(encoder_def->data_symbols);
                format_string += 'D';
                break;
        }

        add_child(symfield.get());
        pos += Point{8, 0};
    }

    // Ugly :( Pad to erase
    format_string.append(24 - format_string.size(), ' ');
    text_format.set(format_string);

    generate_frame();
}

void EncodersConfigView::on_show() {
    options_enctype.set_selected_index(0);
    on_type_change(0);
}

void EncodersConfigView::draw_waveform() {
    size_t length = frame_fragments.length();

    for (size_t n = 0; n < length; n++)
        waveform_buffer[n] = (frame_fragments[n] == '0') ? 0 : 1;

    waveform.set_length(length);
    waveform.set_dirty();
}

void EncodersConfigView::generate_frame() {
    frame_fragments.clear();

    size_t i = 0;
    for (auto c : encoder_def->word_format) {
        if (c == 'S')
            frame_fragments += encoder_def->sync;
        else if (c == '\0')
            break;
        else {
            auto offset = symfields_word[i++]->get_offset(0);
            frame_fragments += encoder_def->bit_format[offset];
        }
    }

    draw_waveform();
}

uint8_t EncodersConfigView::repeat_min() {
    return field_repeat_min.value();
}

uint32_t EncodersConfigView::samples_per_bit() {
    return OOK_SAMPLERATE / ((field_clk.value() * 1000) / encoder_def->clk_per_fragment);
}

uint32_t EncodersConfigView::pause_symbols() {
    return encoder_def->pause_symbols;
}

void EncodersScanView::focus() {
    field_length.focus();
}

EncodersScanView::EncodersScanView(
    NavigationView&,
    Rect parent_rect) {
    set_parent_rect(parent_rect);
    hidden(true);

    add_children({&labels,
                  &field_length,
                  &bit_length_10,
                  &bit_length});

    field_length.set_value(8);
    bit_length_10.set_value(40);
    bit_length.set_value(0);
}

void EncodersView::focus() {
    tab_view.focus();
}

EncodersView::~EncodersView() {
    transmitter_model.disable();
    baseband::shutdown();
}

void EncodersView::update_progress() {
    std::string str_buffer;

    if (tx_mode == SINGLE || tx_mode == SCAN) {
        str_buffer = to_string_dec_uint(repeat_index) + "/" + to_string_dec_uint(repeat_min);
        text_status.set(str_buffer);
        progressbar.set_value(repeat_index);
    } else {
        text_status.set("Ready");
        progressbar.set_value(0);
    }
}

void EncodersView::on_tx_progress(const uint32_t progress, const bool done) {
    if (!done) {
        // Repeating...
        repeat_index = progress + 1;
        update_progress();
    } else {
        // make sure all samples are transmitted before disabling radio
        chThdSleepMilliseconds(10);

        // Done transmitting
        transmitter_model.disable();
        tx_mode = IDLE;
        text_status.set("Done");
        progressbar.set_value(0);
        tx_view.set_transmitting(false);
    }
}

void EncodersView::start_tx(const bool scan) {
    size_t bitstream_length = 0;
    int scan_width = 0;
    uint32_t samples_per_bit;

    if (scan) {
        tx_mode = SCAN;
        scan_width = view_scan.field_length.value();
        samples_per_bit =
            ((view_scan.bit_length_10.value() * 10 + view_scan.bit_length.value()) * OOK_SAMPLERATE) / 1000000UL;
        constexpr auto sym_len = 4;
        const uint32_t seq_len = ((1 << (scan_width - 1)) * 2) * ((uint64_t)samples_per_bit) * sym_len / 2048UL;
        progressbar.set_max(seq_len);
        repeat_min = seq_len;
    } else {
        tx_mode = SINGLE;
        samples_per_bit = view_config.samples_per_bit();
        view_config.generate_frame();
        bitstream_length = make_bitstream(view_config.frame_fragments);
        progressbar.set_max(repeat_min);
        repeat_min = view_config.repeat_min();
    }

    repeat_index = 1;
    update_progress();

    /* Setting TX LPF 1M75 in this TX OOK App  , We got same results as fw 1.7.4
     * Looking max BW of this app, we tested , Selecting OOK type 145026 with CLK 455K and max DEV. 150k,
     * and we got BW +-2Mhz , with that TX LPF 1M75, it is fine.*/
    transmitter_model.enable();

    baseband::set_ook_data(
        bitstream_length,
        samples_per_bit,
        repeat_min,
        view_config.pause_symbols(),
        scan_width);
}

EncodersView::EncodersView(
    NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_ook);

    add_children({&tab_view,
                  &view_config,
                  &view_scan,
                  &text_status,
                  &progressbar,
                  &tx_view});

    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
        };
    };

    tx_view.on_start = [this]() {
        tx_view.set_transmitting(true);
        start_tx(tab_view.selected());
    };

    tx_view.on_stop = [this]() {
        tx_view.set_transmitting(false);
        baseband::kill_ook();
        transmitter_model.disable();
    };
}

} /* namespace ui */
