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

#include "recent_entries.hpp"

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
    struct BleRecentEntry {
        using Key = uint64_t;

        static constexpr Key invalid_key = 0xffffffff;

        uint64_t macAddress;
        int dbValue;
        BlePacketData packetData;

        BleRecentEntry()
            : BleRecentEntry{0} {
        }

        BleRecentEntry(
            const uint64_t macAddress)
            : macAddress{macAddress},
              dbValue{},
              packetData{} {
        }

        Key key() const {
            return macAddress;
        }

        //void update(const BlePacketData * packet);
    };

    using BleRecentEntries = RecentEntries<BleRecentEntry>;
    using BleRecentEntriesView = RecentEntriesView<BleRecentEntries>;

    class BLERxView : public View 
    {
        public:
        BLERxView(NavigationView& nav);
        ~BLERxView();

        void set_parent_rect(const Rect new_parent_rect) override;
        void paint(Painter&) override{};

        void focus() override;

        std::string title() const override { return "BLE RX"; };

        private:
        void on_data(BlePacketData * packetData);
        
        NavigationView& nav_;
        RxRadioState radio_state_
        {
            2402000000 /* frequency */,
            4000000 /* bandwidth */,
            4000000 /* sampling rate */,
            ReceiverModel::Mode::WidebandFMAudio};
        app_settings::SettingsManager settings_{
            "BLE Rx", app_settings::Mode::RX};

        uint8_t console_color{0};
        uint32_t prev_value{0};
        uint8_t channel_number = 37;

        static constexpr auto header_height = 12 + 2 * 16;

        RFAmpField field_rf_amp
        {
            {16 * 8, 0 * 16}
        };

        LNAGainField field_lna
        {
            {18 * 8, 0 * 16}
        };

        VGAGainField field_vga
        {
            {21 * 8, 0 * 16}
        };

        RSSI rssi
        {
            {24 * 8, 0, 6 * 8, 4}
        };

        Channel channel
        {
            {24 * 8, 5, 6 * 8, 4}
        };

        RxFrequencyField field_frequency
        {
            {6 * 8, 0 * 16},
            nav_
        };

        OptionsField options_region{
        {0 * 8, 0 * 8},
        5,
        {{"Ch.37 ", 0},
         {"Ch.38", 1},
         {"Ch.39", 2}}};

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

        // const RecentEntriesColumns columns{{{"Source", 9},
        //                             {"Loc", 6},
        //                             {"Hits", 4},
        //                             {"Time", 8}}};

        BleRecentEntries recent{};

        const RecentEntriesColumns columns{{
            {"Mac Address", 20},
            {"dB", 20},
        }};
        
        BleRecentEntriesView recent_entries_view{columns, recent};

        //RecentEntriesView<RecentEntries<APRSRecentEntry>> recent_entries_view{columns, recent};
        //APRSDetailsView details_view{nav_};
        //uint32_t detailed_entry_key{0};
        //bool send_updates{false};

        MessageHandlerRegistration message_handler_packet
        {
            Message::ID::BlePacket,
            [this](Message* const p) 
            {
                const auto message = static_cast<const BLEPacketMessage*>(p);
                this->on_data(message->packet);
            }
        };
    };

} /* namespace ui */

#endif /*__UI_AFSK_RX_H__*/
