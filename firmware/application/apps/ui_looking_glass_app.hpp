/*
  * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
  * Copyright (C) 2020 euquiq
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
 #include "portapack.hpp"
 #include "baseband_api.hpp"
 #include "receiver_model.hpp"
 #include "ui_widget.hpp"
 #include "ui_navigation.hpp"
 #include "ui_receiver.hpp"
 #include "string_format.hpp"
 #include "analog_audio_app.hpp"
 #include "spectrum_color_lut.hpp"

 namespace ui
 {
     #define LOOKING_GLASS_SLICE_WIDTH	20000000 // Each slice bandwidth 20 MHz
     #define MHZ_DIV	            1000000
     #define X2_MHZ_DIV	        2000000

     class GlassView : public View
     {
     public:
         GlassView(NavigationView &nav);

	 GlassView( const GlassView &);
	 GlassView& operator=(const GlassView &nav);

         ~GlassView();
         std::string title() const override { return "Looking Glass"; };

 	    void on_show() override;
 	    void on_hide() override;
         void focus() override;

     private:
     	NavigationView& nav_;

        struct preset_entry
        {
            rf::Frequency min{};
            rf::Frequency max{};
            std::string label{};
        };

        std::vector<preset_entry> presets_db{};

         void on_channel_spectrum(const ChannelSpectrum& spectrum);
         void do_timers();
         void on_range_changed();
         void on_lna_changed(int32_t v_db);
 	    void on_vga_changed(int32_t v_db);
         void add_spectrum_pixel(Color color);
        void PlotMarker(rf::Frequency pos);
        void load_Presets();
        void txtline_process(std::string& line);
        void populate_Presets();
        void presets_Default();

         rf::Frequency f_min { 0 }, f_max { 0 };
         rf::Frequency search_span { 0 };
         rf::Frequency f_center { 0 };
         rf::Frequency f_center_ini { 0 };
         rf::Frequency marker_pixel_step { 0 };
         rf::Frequency each_bin_size { LOOKING_GLASS_SLICE_WIDTH  / 240 };
         rf::Frequency bins_Hz_size { 0 };
         uint8_t min_color_power { 0 };
         uint32_t pixel_index { 0 };
         std::array<Color, 240> spectrum_row = { 0 };
         ChannelSpectrumFIFO* fifo { nullptr }; 
         uint8_t max_power = 0;

        Labels labels{
            {{0, 0}, "MIN:     MAX:     LNA   VGA  ", Color::light_grey()},
            {{0, 1 * 16}, " RANGE:     FILTER:      AMP:", Color::light_grey()},
            {{0, 2 * 16}, "PRESET:", Color::light_grey()},
            {{0, 3 * 16}, "MARKER:     MHz +/-    MHz", Color::light_grey()},
             {{0, 4 * 16}, "RESOLUTION:    (fft trigger)", Color::light_grey()}
        };

         NumberField field_frequency_min {
             { 4 * 8, 0 * 16 },
             4,
             { 0, 6960 },
             240,
             ' '
         };

         NumberField field_frequency_max {
             { 13 * 8, 0 * 16 },
             4,
             { 240, 7200 },
             240,
             ' '
         };

         LNAGainField field_lna {
             { 21 * 8, 0 * 16 }
         };

         VGAGainField field_vga {
             { 27 * 8, 0 * 16 }
         };

         Text text_range{
            {7 * 8, 1 * 16, 4 * 8, 16},
            ""};

        OptionsField filter_config{
            {19 * 8, 1 * 16},
            4,
            {
                {"OFF ", 0},
                {"MID ", 118}, //85 +25 (110) + a bit more to kill all blue
                {"HIGH", 202}, //168 + 25 (193)
            }};

        RFAmpField field_rf_amp{
            {29 * 8, 1 * 16}};

        OptionsField range_presets{
            {7 * 8, 2 * 16},
            20,
            {
                {" NONE (WIFI 2.4GHz)", 0},
            }};

        NumberField field_marker{
            {7 * 8, 3 * 16},
            4,
            {0, 7200},
            25,
            ' '};

        Text text_marker_pm{
            {20 * 8, 3 * 16, 2 * 8, 16},
            ""};

        NumberField field_trigger{
             {11 * 8, 4 * 16},
             3,
             {2, 128},
             2,
             ' '};


     MessageHandlerRegistration message_handler_spectrum_config {
 		Message::ID::ChannelSpectrumConfig,
 		[this](const Message* const p) {
 			const auto message = *reinterpret_cast<const ChannelSpectrumConfigMessage*>(p);
 			this->fifo = message.fifo;
 		}
 	};
 	MessageHandlerRegistration message_handler_frame_sync {
 		Message::ID::DisplayFrameSync,
 		[this](const Message* const) {
 			if( this->fifo ) {
 				ChannelSpectrum channel_spectrum;
 				while( fifo->out(channel_spectrum) ) {
 					this->on_channel_spectrum(channel_spectrum);
 				}
 			}
 		}
 	};

     };
 } 
 
