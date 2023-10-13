/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
 * Copyright (C) 2023 gullradriel, Nilorea Studio Inc.
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

#ifndef __UI_FSK_RX_H__
#define __UI_FSK_RX_H__

#include "ui_widget.hpp"
#include "ui_freq_field.hpp"
#include "ui_receiver.hpp"
#include "ui_record_view.hpp"
#include "ui_rssi.hpp"
#include "ui_spectrum.hpp"
#include "ui_styles.hpp"

#include "app_settings.hpp"
#include "log_file.hpp"
#include "radio_state.hpp"
#include "pocsag_app.hpp"

#include <functional>

class FskRxLogger 
{
   public:
    Optional<File::Error> append(const std::string& filename) 
    {
        return log_file.append(filename);
    }

    void log_raw_data(const std::string& data, const uint32_t frequency);
    void log_decoded(Timestamp timestamp, const std::string& text);

   private:
    LogFile log_file{};
};

namespace ui 
{
    class FskRxAppView : public View 
    {
        public:
        FskRxAppView(NavigationView& nav);
        ~FskRxAppView();

        std::string title() const override { return "FSK RX"; };
        void focus() override;
        void set_parent_rect(const Rect new_parent_rect) override;

        private:
        static constexpr uint32_t initial_target_frequency = 902'075'000;
        static constexpr ui::Dim header_height = (4 * 16) + 120;
        uint32_t previous_bandwidth{32000};
        bool logging() const { return false; };
        bool logging_raw() const { return false; };

        NavigationView& nav_;
        RxRadioState radio_state_{};

        void refresh_ui(uint32_t bandwidth);
        void on_packet(uint32_t value, bool is_data);
        void handle_decoded(Timestamp timestamp, const std::string& prefix);

        uint32_t last_address = 0;
        FskRxLogger logger{};
        uint16_t packet_count = 0;

        Labels labels
        {
            {{0 * 8, 1 * 16}, "Rate:", Color::light_grey()},
        };

        RxFrequencyField field_frequency
        {
            {0 * 8, 0 * 8},
            nav_
        };

        RFAmpField field_rf_amp
        {
            {11 * 8, 0 * 16}
        };

        LNAGainField field_lna
        {
            {13 * 8, 0 * 16}
        };

        VGAGainField field_vga
        {
            {16 * 8, 0 * 16}
        };

        RSSI rssi
        {
            {19 * 8 - 4, 3, 6 * 8, 4}
        };

        Channel channel
        {
            {19 * 8 - 4, 8, 6 * 8, 4}
        };

        NumberField field_squelch
        {
            {25 * 8, 0 * 16},
            2,
            {0, 99},
            1,
            ' ',
            true /*wrap*/
        };

        AudioVolumeField field_volume
        {
            {28 * 8, 0 * 16}
        };

        OptionsField option_bandwidth
        {
            {5 * 8, 1 * 16},
            5,
            {}
        };

        // DEBUG
        RecordView record_view
        {
            {0 * 8, 2 * 16, 30 * 8, 1 * 16},
            u"FSKRX_????.C16",
            u"FSKRX",
            RecordView::FileType::RawS16,
            16384,
            3
        };

        Console console
        {
            {0, 3 * 16, 240, 120}
        };

        spectrum::WaterfallView waterfall{};

        MessageHandlerRegistration message_handler_packet
        {
            Message::ID::AFSKData,
            [this](Message* const p) 
            {
                const auto message = static_cast<const AFSKDataMessage*>(p);
                this->on_packet(message->value, message->is_data);
            }
        };
    };

} /* namespace ui */

#endif /*__POCSAG_APP_H__*/