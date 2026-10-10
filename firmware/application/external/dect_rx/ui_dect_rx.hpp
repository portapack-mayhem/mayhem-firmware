/*
 * DECT RX application UI for the PortaPack Mayhem DECT RX.
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

#ifndef __UI_DECT_RX_H__
#define __UI_DECT_RX_H__

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "radio_state.hpp"
#include "message.hpp"

using namespace ui;

namespace ui::external_app::dect_rx {

class DECTRxView : public View {
   public:
    DECTRxView(NavigationView& nav);
    ~DECTRxView();

    void focus() override;

    std::string title() const override { return "DECT RX"; }

   private:
    NavigationView& nav_;

    RxRadioState radio_state_{
        /*freq*/ 1921536000,
        /*bandwidth*/ 3500000,
        /*sampling rate*/ 4608000};

    RFAmpField field_rf_amp{{13 * 8, UI_POS_Y(0)}};
    LNAGainField field_lna{{15 * 8, UI_POS_Y(0)}};
    VGAGainField field_vga{{18 * 8, UI_POS_Y(0)}};
    RSSI rssi{{UI_POS_X(21), 0, 9 * 8, 4}};
    Channel channel{{UI_POS_X(21), 5, 9 * 8, 4}};
    RxFrequencyField field_frequency{{UI_POS_X(0), UI_POS_Y(0)}, nav_};

    OptionsField field_band{
        {UI_POS_X(0), UI_POS_Y(1)},
        4,
        {
            {"US", 0},
            {"EU", 1},
        }};

    OptionsField field_channel{
        {UI_POS_X(6), UI_POS_Y(1)},
        6,
        {
            {"Ch0", 0},
            {"Ch1", 1},
            {"Ch2", 2},
            {"Ch3", 3},
            {"Ch4", 4},
            {"Ch5", 5},
            {"Ch6", 6},
            {"Ch7", 7},
            {"Ch8", 8},
            {"Ch9", 9},
        }};

    Text text_status{
        {UI_POS_X(0), UI_POS_Y(2), screen_width, UI_POS_DEFAULT_HEIGHT},
        "starting"};

    Console console{
        {0, UI_POS_Y(3), screen_width, UI_POS_HEIGHT_REMAINING(3)}};

    uint8_t band_{0};
    uint32_t last_logged_id_[8][5]{};
    bool last_logged_valid_[8]{};

    void apply_selection();
    void on_part_update(const DECTPartInfoMessage* message);
    void on_status(const DECTStatusMessage* message);
    void on_freqchg(int64_t freq);

    MessageHandlerRegistration message_handler_parts{
        Message::ID::DECTPartInfo,
        [this](Message* const p) {
            this->on_part_update(static_cast<const DECTPartInfoMessage*>(p));
        }};

    MessageHandlerRegistration message_handler_status{
        Message::ID::DECTStatus,
        [this](Message* const p) {
            this->on_status(static_cast<const DECTStatusMessage*>(p));
        }};

    MessageHandlerRegistration message_handler_freqchg{
        Message::ID::FreqChangeCommand,
        [this](Message* const p) {
            const auto message = static_cast<const FreqChangeCommandMessage*>(p);
            this->on_freqchg(message->freq);
        }};
};

}  // namespace ui::external_app::dect_rx

#endif /*__UI_DECT_RX_H__*/
