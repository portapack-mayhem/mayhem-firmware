/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#ifndef __BLE_APP_H__
#define __BLE_APP_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"

class BLELogger 
{
   public:
    Optional<File::Error> append(const std::string& filename) 
    {
        return log_file.append(filename);
    }

    void log_raw_data(const std::string& data);

   private:
    LogFile log_file{};
};

namespace ui 
{
    class BLERxView : public View 
    {
        public:
        BLERxView(NavigationView& nav);
        ~BLERxView();

        void focus() override;

        std::string title() const override { return "BLE RX"; };

        private:
        void on_data(uint32_t value, bool is_data);

        NavigationView& nav_;
        RxRadioState radio_state_{
            4000000 /* bandwidth */,
            4000000 /* sampling rate */,
            ReceiverModel::Mode::WidebandFMAudio};
        app_settings::SettingsManager settings_{
            "BLE Rx", app_settings::Mode::RX};

        uint8_t console_color{0};
        uint32_t prev_value{0};

        typedef enum
        {
            ParsingAccessAddress = 0,
            ParsingType,
            ParsingSize,
        } ParseState;

        ParseState parsestate = ParsingAccessAddress;

        RFAmpField field_rf_amp
        {
            {13 * 8, 0 * 16}
        };

        LNAGainField field_lna
        {
            {15 * 8, 0 * 16}
        };

        VGAGainField field_vga
        {
            {18 * 8, 0 * 16}
        };

        RSSI rssi
        {
            {21 * 8, 0, 6 * 8, 4}
        };

        Channel channel
        {
            {21 * 8, 5, 6 * 8, 4}
        };

        RxFrequencyField field_frequency
        {
            {0 * 8, 0 * 16},
            nav_
        };

        Console console
        {
            {0, 4 * 16, 240, 240}
        };

        Checkbox check_log
        {
            {0 * 8, 1 * 16},
            3,
            "LOG",
            false
        };

        Text text_debug
        {
            {0 * 8, 12 + 2 * 16, screen_width, 16},
            "BLE OUTPUT"
        };

        typedef enum
        {
            ADV_IND = 0,
            ADV_DIRECT_IND= 1,
            ADV_NONCONN_IND= 2,
            SCAN_REQ= 3,
            SCAN_RSP= 4,
            CONNECT_REQ= 5,
            ADV_SCAN_IND= 6,
            RESERVED0= 7,
            RESERVED1= 8,
            RESERVED2= 9,
            RESERVED3= 10,
            RESERVED4= 11,
            RESERVED5= 12,
            RESERVED6= 13,
            RESERVED7= 14,
            RESERVED8= 15
        } ADV_PDU_TYPE;

        std::string str_log{""};
        bool logging{false};

        std::unique_ptr<BLELogger> logger{};

        MessageHandlerRegistration message_handler_packet
        {
            Message::ID::AFSKData,
            [this](Message* const p) 
            {
                const auto message = static_cast<const AFSKDataMessage*>(p);
                this->on_data(message->value, message->is_data);
            }
        };
    };

} /* namespace ui */

#endif /*__UI_AFSK_RX_H__*/
