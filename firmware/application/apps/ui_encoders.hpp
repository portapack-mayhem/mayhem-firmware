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

#include "ui.hpp"
#include "ui_tabview.hpp"
#include "ui_transmitter.hpp"
#include "transmitter_model.hpp"
#include "encoders.hpp"
#include "de_bruijn.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"

#include <memory>
#include <vector>

using namespace encoders;

namespace ui {

class EncodersConfigView : public View {
   public:
    EncodersConfigView(NavigationView& nav, Rect parent_rect);

    EncodersConfigView(const EncodersConfigView&) = delete;
    EncodersConfigView(EncodersConfigView&&) = delete;
    EncodersConfigView& operator=(const EncodersConfigView&) = delete;
    EncodersConfigView& operator=(EncodersConfigView&&) = delete;

    void focus() override;
    void on_show() override;

    uint8_t repeat_min();
    uint32_t samples_per_bit();
    uint32_t pause_symbols();
    void generate_frame();

    std::string frame_fragments = "0";

   private:
    int16_t waveform_buffer[550];
    const encoder_def_t* encoder_def{};

    void draw_waveform();
    void on_bitfield();
    void on_type_change(size_t index);

    Labels labels{
        {{1 * 8, 0}, "Type:", Theme::getInstance()->fg_light->foreground},
        {{17 * 8, 0}, "Repeat:", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 2 * 8}, "Clk:", Theme::getInstance()->fg_light->foreground},
        {{10 * 8, 2 * 8}, "kHz", Theme::getInstance()->fg_light->foreground},
        {{17 * 8, 2 * 8}, "Step:", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 4 * 8}, "Frame:", Theme::getInstance()->fg_light->foreground},
        {{13 * 8, 4 * 8}, "us", Theme::getInstance()->fg_light->foreground},
        {{17 * 8, 4 * 8}, "Step:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 7 * 8}, "Symbols:", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 14 * 8}, "Waveform:", Theme::getInstance()->fg_light->foreground}};

    OptionsField options_enctype{// Options are loaded at runtime
                                 {6 * 8, 0},
                                 7,
                                 {}};

    NumberField field_clk{
        {5 * 8, 2 * 8},
        4,
        {1, 1000},
        1,
        ' '};

    NumberField field_repeat_min{
        {24 * 8, 0},
        2,
        {1, 99},
        1,
        ' '};

    OptionsField field_clk_step{
        {22 * 8, 2 * 8},
        7,
        {{"1", 1},
         {"10", 10},
         {"100", 100}}};

    NumberField field_frameduration{
        {7 * 8, 4 * 8},
        5,
        {300, 99999},
        100,
        ' '};

    OptionsField field_frameduration_step{
        {22 * 8, 4 * 8},
        7,
        {{"1", 1},
         {"10", 10},
         {"100", 100},
         {"1000", 1000}}};

    std::vector<std::unique_ptr<SymField>> symfields_word{};

    Text text_format{
        {2 * 8, 11 * 8, 24 * 8, 16},
        ""};

    Waveform waveform{
        {0, 17 * 8, 240, 32},
        waveform_buffer,
        0,
        0,
        true,
        Theme::getInstance()->fg_yellow->foreground};
};

class EncodersScanView : public View {
   public:
    EncodersScanView(NavigationView& nav, Rect parent_rect);

    NumberField field_length{
        {8 * 8, 0},
        2,
        {3, 24},
        1,
        ' '};

    NumberField bit_length_10{
        {12 * 8, 2 * 8},
        2,
        {1, 88},
        1,
        ' '};

    NumberField bit_length{
        {14 * 8, 2 * 8},
        1,
        {0, 9},
        1,
        ' '};

    void focus() override;

   private:
    Labels labels{
        {{1 * 8, 0 * 8}, "Length:", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 2 * 8}, "Bit length:", Theme::getInstance()->fg_light->foreground},
        {{16 * 8, 2 * 8}, "us", Theme::getInstance()->fg_light->foreground},
    };
};

class EncodersView : public View {
   public:
    EncodersView(NavigationView& nav);
    ~EncodersView();

    void focus() override;

    std::string title() const override { return "OOK TX"; };

   private:
    NavigationView& nav_;

    enum tx_modes {
        IDLE = 0,
        SINGLE,
        SCAN
    };

    TxRadioState radio_state_{
        433920000 /* frequency */,
        1750000 /* bandwidth */,
        OOK_SAMPLERATE /* sampling rate */
    };
    app_settings::SettingsManager settings_{
        "tx_ook", app_settings::Mode::TX};

    tx_modes tx_mode = IDLE;
    uint32_t repeat_index{0};
    uint32_t repeat_min{0};

    void update_progress();
    void start_tx(const bool scan);
    void on_tx_progress(const uint32_t progress, const bool done);

    Rect view_rect = {0, 4 * 8, 240, 168};

    EncodersConfigView view_config{nav_, view_rect};
    EncodersScanView view_scan{nav_, view_rect};

    TabView tab_view{
        {"Config", Theme::getInstance()->fg_cyan->foreground, &view_config},
        {"de Bruijn", Theme::getInstance()->fg_green->foreground, &view_scan},
    };

    Text text_status{
        {2 * 8, 13 * 16, 128, 16},
        "Ready"};

    ProgressBar progressbar{
        {2 * 8, 13 * 16 + 20, 208, 16}};

    TransmitterView tx_view{
        16 * 16,
        50000,
        9};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.progress, message.done);
        }};
};

} /* namespace ui */
