/*
 * Copyright (C) 2025 Sarah Rose
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

#ifndef __UI_MDC_TX_H__
#define __UI_MDC_TX_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "message.hpp"
#include "modems.hpp"

namespace ui::external_app::mdc_tx {

class MdcTxView : public View {
   public:
    MdcTxView(NavigationView& nav);
    ~MdcTxView();
    MdcTxView(const MdcTxView&) = delete;
    MdcTxView& operator=(const MdcTxView&) = delete;

    void focus() override;
    std::string title() const override { return "MDC-1200 TX"; }

   private:
    NavigationView& nav_;

    // TxRadioState MUST be declared before SettingsManager
    TxRadioState radio_state_{
        462562500,
        1750000,
        AFSK_TX_SAMPLERATE};
    app_settings::SettingsManager settings_{
        "tx_mdc_tx",
        app_settings::Mode::TX};

    Labels labels_{{
        {{UI_POS_X(0), UI_POS_Y(0)}, "Op:", ui::Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(1)}, "ID:", ui::Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(13), UI_POS_Y(1)}, "Arg:", ui::Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(2)}, "Src:", ui::Theme::getInstance()->fg_light->foreground},
    }};

    // Only one OptionsField allowed per external app
    // Stun=Op 0x2B Arg 0x00, Unstun=Op 0x2B Arg 0x0C (auto-set via on_change)
    OptionsField field_op{
        {UI_POS_X(4), UI_POS_Y(0)},
        14,
        {{"PTT ID", 0x01},
         {"Emergency", 0x00},
         {"Radio Check", 0x63},
         {"Stun", 0x2B},
         {"Unstun", 0xFE},  // sentinel: op=0x2B, arg=0x0C
         {"Call Alert", 0x81},
         {"Sel Call 1", 0x80},
         {"Sel Call 2", 0x82},
         {"Status Req", 0x22},
         {"Status Resp", 0x06},
         {"Rem Monitor", 0x11},
         {"Message", 0x07}}};

    // Unit ID: two decimal bytes (0-255) combined into a 16-bit ID
    NumberField field_id_hi{{UI_POS_X(4), UI_POS_Y(1)}, 3, {0, 255}, 1, '0'};
    NumberField field_id_lo{{UI_POS_X(7), UI_POS_Y(1)}, 3, {0, 255}, 1, '0'};

    // Argument byte (decimal 0-255, auto-set by op selection)
    NumberField field_arg{{UI_POS_X(17), UI_POS_Y(1)}, 3, {0, 255}, 1, '0'};

    // Source ID: two decimal bytes (0-255) combined into a 16-bit ID
    NumberField field_src_hi{{UI_POS_X(4), UI_POS_Y(2)}, 3, {0, 255}, 1, '0'};
    NumberField field_src_lo{{UI_POS_X(7), UI_POS_Y(2)}, 3, {0, 255}, 1, '0'};

    Text text_status{{0, UI_POS_Y(3), UI_POS_MAXWIDTH, UI_POS_DEFAULT_HEIGHT}, "Ready"};

    TransmitterView tx_view{
        (int16_t)UI_POS_Y_BOTTOM(4),
        12500,
        0};

    bool tx_active_{false};

    // Class-owned scratch buffers — keep large allocations off the stack
    uint8_t tx_stream_[128]{};
    uint8_t lbits_[112]{};

    void pack_codeword(uint8_t cw[14]);
    int build_single(uint8_t op, uint8_t arg, uint8_t id_hi, uint8_t id_lo);
    int build_double(uint8_t op1, uint8_t arg1, uint8_t id1_hi, uint8_t id1_lo, uint8_t op2, uint8_t arg2, uint8_t id2_hi, uint8_t id2_lo);
    void start_tx();
    void stop_tx();
    void on_tx_progress(uint32_t progress, bool done);

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto msg = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(msg.progress, msg.done);
        }};
};

}  // namespace ui::external_app::mdc_tx

#endif
