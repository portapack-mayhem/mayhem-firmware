/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#ifndef _UI_LEVEL
#define _UI_LEVEL

#include "ui.hpp"
#include "receiver_model.hpp"
#include "ui_receiver.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "freqman.hpp"
#include "analog_audio_app.hpp"
#include "audio.hpp"
#include "ui_mictx.hpp"
#include "portapack_persistent_memory.hpp"
#include "baseband_api.hpp"
#include "ui_spectrum.hpp"
#include "string_format.hpp"
#include "file.hpp"
#include "app_settings.hpp"


namespace ui {

    class LevelView : public View {
        public:
            LevelView(NavigationView& nav);
            ~LevelView();

            void focus() override;

            void big_display_freq( int64_t f );

            const Style style_grey {		// level
                .font = font::fixed_8x16,
                    .background = Color::black(),
                    .foreground = Color::grey(),
            };

            const Style style_white {		// level
                .font = font::fixed_8x16,
                    .background = Color::black(),
                    .foreground = Color::white(),
            };

            const Style style_yellow {		//Found signal
                .font = font::fixed_8x16,
                    .background = Color::black(),
                    .foreground = Color::yellow(),
            };

            const Style style_green {		//Found signal
                .font = font::fixed_8x16,
                    .background = Color::black(),
                    .foreground = Color::green(),
            };

            const Style style_red {		//erasing freq
                .font = font::fixed_8x16,
                    .background = Color::black(),
                    .foreground = Color::red(),
            };

            const Style style_blue {		// quick level, wait == 0
                .font = font::fixed_8x16,
                    .background = Color::black(),
                    .foreground = Color::blue(),
            };

            std::string title() const override { return "Level"; };

        private:
            NavigationView& nav_;

            size_t change_mode( freqman_index_t mod_type);
            void on_statistics_update(const ChannelStatistics& statistics);
            void set_display_freq( int64_t freq );
            bool check_sd_card();

            int32_t db { 0 };
            long long int MAX_UFREQ = { 7200000000 }; // maximum usable freq
            bool sd_card_mounted = false ;
            rf::Frequency freq = { 0 } ;

            Labels labels 
            { 
                { { 0  * 8 , 0  * 16      }, "LNA:   VGA:   AMP:  VOL:     ", Color::light_grey() },
                    { { 0  * 8 , 1  * 16      }, "BW:       MODE:    S:   ", Color::light_grey() },
            };

            LNAGainField field_lna {
                { 4 * 8, 0 * 16 }
            };

            VGAGainField field_vga {
                { 11 * 8, 0 * 16 }
            };

            RFAmpField field_rf_amp {
                { 18 * 8, 0 * 16 }
            };

            NumberField field_volume {
                { 24 * 8, 0 * 16 },
                    2,
                    { 0, 99 },
                    1,
                    ' ',
            };

            OptionsField field_bw {
                { 3 * 8, 1 * 16 },
                    6,
                    { }
            };		

            OptionsField field_mode {
                { 15 * 8, 1 * 16 },
                    3,
                    {
                    }
            };

            OptionsField step_mode {
                { 16 * 8, 2 * 16 + 4 },
                    12,
                    {
                    }
            };

            RSSIGraph rssi_graph { // 240x320  => 
                { 0 , 5 * 16 + 4 , 240 - 5 * 8 , 216 },
            }; 

            RSSI rssi { // 240x320  =>  
                { 240 - 5 * 8 , 5 * 16 + 4 , 5 * 8 , 216 },
            }; 


            ButtonWithEncoder button_frequency {
                { 0 * 8 , 2 * 16 + 8 , 15 * 8 , 1 * 8 },
                    ""
            };

            OptionsField audio_mode {
                { 21 * 8, 1 * 16 },
                    9,
                    {
                        {"audio off", 0},
                        {"audio on",1}
                        //{"tone on", 2},
                        //{"tone off", 2},
                    }
            };		


            Text text_ctcss {
                { 20 * 8, 3 * 16 + 4 , 14 * 8, 1 * 8 },
                    ""
            };

            Text freq_stats_rssi {		
                { 0 * 8 , 3 * 16 + 4 , 20 * 8, 16 },
            };

            Text freq_stats_db {		
                { 0 * 8 , 4 * 16 + 4 , 15 * 8, 16 },
            };


            OptionsField peak_mode {
                { 44 + 10 * 8, 4 * 16 + 4 },
                    10,
                    {
                        {"peak:none", 0},
                        {"peak:0.25s",250},
                        {"peak:0.5s",500},
                        {"peak:1s",1000},
                        {"peak:3s",3000},
                        {"peak:5s",5000},
                        {"peak:10s",10000},
                    }
            };		

            void handle_coded_squelch(const uint32_t value);
            void on_headphone_volume_changed(int32_t v);

            MessageHandlerRegistration message_handler_coded_squelch {
                Message::ID::CodedSquelch,
                    [this](const Message* const p) {
                        const auto message = *reinterpret_cast<const CodedSquelchMessage*>(p);
                        this->handle_coded_squelch(message.value);
                    }
            };

            MessageHandlerRegistration message_handler_stats {
                Message::ID::ChannelStatistics,
                    [this](const Message* const p) {
                        this->on_statistics_update(static_cast<const ChannelStatisticsMessage*>(p)->statistics);
                    }
            };
            // app save settings
            std::app_settings settings { }; 		
            std::app_settings::AppSettings app_settings { };
    };

} /* namespace ui */

#endif
