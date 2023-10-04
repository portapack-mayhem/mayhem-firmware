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
#include "ui_rssi.hpp"
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
    // class BaudIndicator : public Widget 
    // {
    // public:
    //     BaudIndicator(Point position)
    //         : Widget{{position, {5, height}}} {}

    //     void paint(Painter& painter) override;
    //     void set_rate(uint16_t rate) 
    //     {
    //         if (rate != rate_) 
    //         {
    //             rate_ = rate;
    //             set_dirty();
    //         }
    //     }

    // private:
    //     static constexpr uint8_t height = 16;
    //     uint16_t rate_ = 0;
    // };

    // class BitsIndicator : public Widget 
    // {
    // public:
    //     BitsIndicator(Point position)
    //         : Widget{{position, {2, height}}
    //         } {}

    //     void paint(Painter& painter) override;

    //     void set_bits(uint32_t bits) 
    //     {
    //         if (bits != bits_) 
    //         {
    //             bits_ = bits;
    //             set_dirty();
    //         }
    //     }

    // private:
    //     static constexpr uint8_t height = 16;
    //     uint32_t bits_ = 0;
    // };

    // class FrameIndicator : public Widget 
    // {
    // public:
    //     FrameIndicator(Point position)
    //         : Widget{{position, {4, height}}} {}

    //     void paint(Painter& painter) override;

    //     void set_frames(uint8_t frame_count)
    //     {
    //         if (frame_count != frame_count_) 
    //         {
    //             frame_count_ = frame_count;
    //             set_dirty();
    //         }
    //     }

    //     void set_sync(bool has_sync) 
    //     {
    //         if (has_sync != has_sync_) 
    //         {
    //             has_sync_ = has_sync;
    //             set_dirty();
    //         }
    //     }

    // private:
    //     static constexpr uint8_t height = 16;
    //     uint8_t frame_count_ = 0;
    //     bool has_sync_ = false;
    // };

    class FskRxAppView : public View 
    {
        public:
        FskRxAppView(NavigationView& nav);
        ~FskRxAppView();

        std::string title() const override { return "FSK RX"; };
        void focus() override;

        private:
        static constexpr uint32_t initial_target_frequency = 902'075'000;
        bool logging() const { return false; };
        bool logging_raw() const { return false; };

        NavigationView& nav_;
        RxRadioState radio_state_{};

        void refresh_ui();
        void on_packet(uint32_t value, bool is_data);
        void handle_decoded(Timestamp timestamp, const std::string& prefix);

        uint32_t last_address = 0;
        FskRxLogger logger{};
        uint16_t packet_count = 0;

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

        Audio audio
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

        Image image_status
        {
            {0 * 8 + 4, 1 * 16 + 2, 16, 16},
            &bitmap_icon_pocsag,
            Color::white(),
            Color::black()
        };

        Text text_packet_count
        {
            {3 * 8, 1 * 16 + 2, 5 * 8, 16},
            "0"
        };

        BitsIndicator widget_bits
        {
            {8 * 8 + 1, 1 * 16 + 2}
        };

        FrameIndicator widget_frames
        {
            {8 * 8 + 4, 1 * 16 + 2}
        };

        BaudIndicator widget_baud
        {
            {8 * 9 + 1, 1 * 16 + 2}
        };

        // 54 == status bar (16) + top controls (2 * 16 + 6).
        Console console
        {
            {0, 2 * 16 + 6, screen_width, screen_height - 54}
        };

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