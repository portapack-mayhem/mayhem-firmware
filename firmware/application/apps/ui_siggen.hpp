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

#ifndef __SIGGEN_H__
#define __SIGGEN_H__

#include "app_settings.hpp"
#include "radio_state.hpp"
#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"

#include "portapack.hpp"
#include "message.hpp"

namespace ui {

class SigGenView : public View {
   public:
    SigGenView(NavigationView& nav);
    ~SigGenView();

    void focus() override;

    std::string title() const override { return "Signal gen"; };

   private:
    void start_tx();
    void update_config();
    void update_tone();
    void on_tx_progress(const uint32_t progress, const bool done);

    TxRadioState radio_state_{
        0 /* frequency */,
        1750000 /* bandwidth */,
        1536000 /* sampling rate */
    };
    app_settings::SettingsManager settings_{
        "tx_siggen", app_settings::Mode::TX};

    const std::string shape_strings[9] = {
        "CW  (No mod.) ",
        "Sine mod. FM",
        "Triangle mod.FM",  // max 15 character text space.
        "Saw up mod. FM",
        "Saw down mod.FM",
        "Square mod. FM",
        "Pseudo Noise FM",  // using 16 bits LFSR register, 16 order polynomial feedback.
        "BPSK 0,1,0,1...",
        "QPSK 00-01-10.."};

    bool auto_update{false};

    Labels labels{
        {{3 * 8, 4 + 10}, "Shape:", Theme::getInstance()->fg_light->foreground},
        {{6 * 8, 7 * 8}, "Tone:      Hz", Theme::getInstance()->fg_light->foreground},
        {{22 * 8, 15 * 8 + 4}, "s.", Theme::getInstance()->fg_light->foreground},
        {{8 * 8, 20 * 8}, "Modulation: FM", Theme::getInstance()->fg_light->foreground}};

    ImageOptionsField options_shape{
        {10 * 8, 4, 32, 32},
        Theme::getInstance()->bg_darkest->foreground,
        Theme::getInstance()->bg_darkest->background,
        {{&bitmap_sig_cw, 0},
         {&bitmap_sig_sine, 1},
         {&bitmap_sig_tri, 2},
         {&bitmap_sig_saw_up, 3},
         {&bitmap_sig_saw_down, 4},
         {&bitmap_sig_square, 5},
         {&bitmap_sig_noise, 6},
         {&bitmap_sig_noise, 7},    // Pending to add a correct BPSK icon.
         {&bitmap_sig_noise, 8}}};  // Pending to add a correct QPSK icon.

    Text text_shape{
        {15 * 8, 4 + 10, 15 * 8, 16},
        ""};

    SymField symfield_tone{
        {12 * 8, 7 * 8},
        5};

    Button button_update{
        {5 * 8, 10 * 8, 8 * 8, 3 * 8},
        "Update"};

    Checkbox checkbox_auto{
        {15 * 8, 10 * 8},
        4,
        "Auto"};

    Checkbox checkbox_stop{
        {5 * 8, 15 * 8},
        10,
        "Stop after"};

    NumberField field_stop{
        {20 * 8, 15 * 8 + 4},
        2,
        {1, 99},
        1,
        ' '};

    TransmitterView tx_view{
        16 * 16,
        10000,
        12};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.progress, message.done);
        }};
};

} /* namespace ui */

#endif /*__SIGGEN_H__*/
