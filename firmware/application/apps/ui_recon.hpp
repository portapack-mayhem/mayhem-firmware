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

#ifndef _UI_RECON
#define _UI_RECON

#include "ui.hpp"
#include "receiver_model.hpp"
#include "ui_receiver.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "freqman.hpp"
#include "analog_audio_app.hpp"
#include "audio.hpp"
#include "ui_mictx.hpp"
#include "ui_level.hpp"
#include "ui_looking_glass_app.hpp"
#include "portapack_persistent_memory.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "file.hpp"
#include "app_settings.hpp"
#include "ui_recon_settings.hpp"

namespace ui {

	class ReconView : public View {
		public:
			ReconView(NavigationView& nav);
			~ReconView();

			void focus() override;

			const Style style_grey {		// recon
				.font = font::fixed_8x16,
					.background = Color::black(),
					.foreground = Color::grey(),
			};

			const Style style_white {		// recon
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

			const Style style_blue {		// quick recon, wait == 0
				.font = font::fixed_8x16,
					.background = Color::black(),
					.foreground = Color::blue(),
			};

			std::string title() const override { return "Recon"; };

			//void set_parent_rect(const Rect new_parent_rect) override;

		private:
			NavigationView& nav_;

            void audio_output_start();
			bool check_sd_card();
			size_t change_mode( freqman_index_t mod_type);
			void show_max( bool refresh_display = false );
			void recon_pause();
			void recon_resume();
			void frequency_file_load( bool stop_all_before = false);
			void on_statistics_update(const ChannelStatistics& statistics);
			void on_headphone_volume_changed(int32_t v);
			void on_index_delta(int32_t v);
			void on_stepper_delta(int32_t v);
			void recon_redraw();
			void handle_retune();
			void handle_coded_squelch(const uint32_t value);

			jammer::jammer_range_t frequency_range { false, 0, MAX_UFREQ };  //perfect for manual recon task too...
			int32_t squelch { 0 };
			int32_t db { 0 };
			int32_t timer { 0 };
			int32_t wait { RECON_DEF_WAIT_DURATION };   // in msec. if > 0 wait duration after a lock, if < 0 duration is set to 'wait' unless there is no more activity
			freqman_db frequency_list  = { };
			int32_t current_index { 0 };
			bool userpause { false };
			bool continuous_lock { false };
			std::string input_file = { "RECON" };
			std::string output_file = { "RECON_RESULTS" };
			bool autosave    = { true };
			bool autostart   = { true };
			bool continuous  = { true }; 
			bool filedelete  = { true };
			bool load_freqs  = { true };
			bool load_ranges = { true };
			bool load_hamradios = { true };
			bool update_ranges = { true };
			bool fwd = { true };
            bool recon = true ;
			uint32_t recon_lock_nb_match = { 3 };
			uint32_t recon_lock_duration = { RECON_DEF_LOCK_DURATION };
			uint32_t recon_match_mode = { RECON_MATCH_CONTINUOUS };
			bool scanner_mode { false };
			bool manual_mode { false };
			bool sd_card_mounted = false ;
			int32_t volume = 40 ;
            int32_t stepper = 0 ;
            int32_t index_stepper = 0 ;
			int64_t freq = 0 ;
			uint32_t step = 0 ;
			freqman_index_t def_modulation  = 0 ;
			freqman_index_t def_bandwidth = 0 ;
			freqman_index_t def_step = 0 ;
			tone_index tone = 0 ;
			freqman_entry last_entry = { } ;
			bool entry_has_changed = false ;
			uint32_t freq_lock { 0 };
            int64_t minfreq = 0 ;
			int64_t maxfreq = 0 ;
			bool has_looped = false ;

			Labels labels 
			{ 
				{ { 0  * 8 , 0  * 16      }, "LNA:   VGA:   AMP:  VOL:     ", Color::light_grey() },
					{ { 0  * 8 , 1  * 16      }, "BW:      SQ:    W,L:     ,     ", Color::light_grey() },
					{ { 3  * 8 , 10 * 16      }, "START       END     MANUAL", Color::light_grey() },
					{ { 0  * 8 , (26 * 8) + 4 }, "MODE:", Color::light_grey() },
					{ { 11 * 8 , (26 * 8) + 4 }, "STEP:", Color::light_grey() },
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

			NumberField field_squelch {
				{ 12 * 8, 1 * 16 },
					3,
					{ -90, 20 },
					1,
					' ',
			};

			NumberField field_wait {
				{ 20 * 8, 1 * 16 },
					5,
					{ -(10000-STATS_UPDATE_INTERVAL) , (10000-STATS_UPDATE_INTERVAL) },
					STATS_UPDATE_INTERVAL,
					' ',
			};

			NumberField field_lock_wait {
				{ 26 * 8, 1 * 16 },
					4,
					{ RECON_DEF_LOCK_DURATION , (10000-RECON_DEF_LOCK_DURATION) },
					RECON_DEF_LOCK_DURATION,
					' ',
			};

			RSSI rssi {
				{ 0 * 16, 2 * 16, 240 - 8 * 8 + 4 , 16 },
			}; 

			ButtonWithEncoder text_cycle {
				{ 0, 3 * 16, 4 * 8, 16 },  
					""
			};

			Text text_max {
				{ 4 * 8, 3 * 16, 240 - 7 * 8 - 4 * 8 , 16 },  
			};

			Text desc_cycle {
				{0, 4 * 16, 240 , 16 },	   
			};

			/* BigFrequency big_display {		//Show frequency in glamour
			   { 4, 7 * 16 - 8 , 28 * 8, 52 },
			   0
			   }; */

			Text big_display {		//Show frequency in text mode
				{ 0, 5 * 16 , 23 * 8, 16 },
			};

			Text freq_stats {		//Show frequency stats in text mode
				{ 0, 6 * 16 , 23 * 8, 16 },
			};

			// TIMER: 9999
			Text text_timer {		//Show frequency stats in text mode
				{ 0, 7 * 16 , 11 * 8, 16 },
			};

			// T: Senn. 32.000k
			Text text_ctcss {
				{ 12 * 8 + 4, 7 * 16 , 14 * 8, 1 * 8 },
					""
			};

			Button button_recon_setup {
				{ 240 - 7 * 8 , 2 * 16 , 7 * 8, 28 },
					"CONFIG"
			};

			Button button_looking_glass {
				{ 240 - 7 * 8 , 5 * 16 , 7 * 8, 28 },
					"GLASS"
			};

			// Button can be RECON or SCANNER
			Button button_scanner_mode {
				{ 240 - 7 * 8 , 8 * 16 , 7 * 8, 28 },
					"RECON"
			};

			Text file_name {		//Show file used
				{ 0 , 8 * 16 + 6 , 240 - 7 * 8, 16 },
			};

			ButtonWithEncoder button_manual_start {
				{ 0 * 8, 11 * 16, 11 * 8, 28 },
					""
			};

			ButtonWithEncoder button_manual_end {
				{ 12 * 8 - 6, 11 * 16, 11 * 8, 28 },
					""
			};

			Button button_manual_recon {
				{ 23 * 8 - 3, 11 * 16, 7 * 8 , 28 },
					"SEARCH"
			};

			OptionsField field_mode {
				{ 5 * 8, (26 * 8) + 4 },
					6,
					{
					}
			};

			OptionsField step_mode {
				{ 17 * 8, (26 * 8) + 4 },
					12,
					{
					}
			};

			ButtonWithEncoder button_pause {
				{ 0, (15 * 16) - 4, 72, 28 },
					"PAUSE"
			}; 


			Button button_audio_app {
				{ 84, (15 * 16) - 4, 72, 28 },
					"AUDIO"
			};

			ButtonWithEncoder button_add {
				{ 168, (15 * 16) - 4, 72, 28 },
					"<STORE>"
			};

			Button button_dir {
				{ 0,  (35 * 8) - 4, 34, 28 },
					"FW>"
			};

			Button button_restart {
				{ 38, (35 * 8) - 4, 34, 28 },
					"RST"
			};


			Button button_mic_app {
				{ 84,  (35 * 8) - 4, 72, 28 },
					"MIC TX"
			};

			ButtonWithEncoder button_remove {
				{ 168, (35 * 8) - 4, 72, 28 },
					"<REMOVE>"
			};

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
