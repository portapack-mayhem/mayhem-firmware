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


namespace ui {

	class ReconThread {
		public:
			ReconThread(freqman_db *database );
			~ReconThread();

			void set_recon(const bool v);
			void set_freq_delete(const bool v);
			bool is_recon();

			void set_lock_duration( const uint32_t v );
			uint32_t get_lock_duration();
			void set_lock_nb_match( const uint32_t v );
			void set_match_mode( const uint32_t v );
			uint32_t get_lock_nb_match();

			void set_freq_lock(const uint32_t v);
			uint32_t is_freq_lock();
			int64_t get_current_freq();

			void set_stepper(const int64_t v);
			void set_index_stepper(const int64_t v);

			void change_recon_direction();
			bool get_recon_direction();
			void set_recon_direction( const bool v);

			void set_continuous(const bool v);

			void set_default_modulation( freqman_index_t index );
			freqman_index_t get_current_modulation();
			void set_default_bandwidth( freqman_index_t index );
			freqman_index_t get_current_bandwidth();
			void set_default_step( freqman_index_t index );
			void set_freq_index( int16_t index );
			int16_t get_freq_index();

			void run();
			void stop();

			ReconThread(const ReconThread&) = delete;
			ReconThread(ReconThread&&) = delete;
			ReconThread& operator=(const ReconThread&) = delete;
			ReconThread& operator=(ReconThread&&) = delete;

		private:
			freqman_db &frequency_list_ ;
			Thread* thread { nullptr };
			int64_t freq = 0 ;
			uint32_t step = 0 ;
			freqman_index_t def_modulation  = 0 ;
			freqman_index_t def_bandwidth = 0 ;
			freqman_index_t def_step = 0 ;
			tone_index tone = 0 ;
			freqman_entry last_entry = { } ;
			int16_t frequency_index = 0 ;

			bool _recon { true };
			bool _freq_delete { false };
			bool _fwd { true };
			bool _continuous { true };
			bool entry_has_changed = false ;
			int64_t _stepper { 0 };
			int64_t _index_stepper { 0 };
			int32_t _freq_lock { 0 };
			uint32_t _lock_duration { 50 };
			uint32_t _lock_nb_match { 10 };
			static msg_t static_fn(void* arg);
	};

	class ReconView : public View {
		public:
			ReconView(NavigationView& nav);
			~ReconView();

			void focus() override;

			void big_display_freq( int64_t f );

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

			void start_recon_thread();
			size_t change_mode( freqman_index_t mod_type);
			void show_max( bool refresh_display = false );
			void recon_pause();
			void recon_resume();
			void user_pause();
			void user_resume();
			void frequency_file_load( bool stop_all_before = false);
			void on_statistics_update(const ChannelStatistics& statistics);
			void on_headphone_volume_changed(int32_t v);
			void set_display_freq( int64_t freq );
			void handle_retune( int64_t freq , uint32_t index );
			bool check_sd_card();
			void handle_coded_squelch(const uint32_t value);

			jammer::jammer_range_t frequency_range { false, 0, 0 };  //perfect for manual recon task too...
			int32_t squelch { 0 };
			int32_t db { 0 };
			int32_t timer { 0 };
			int32_t wait { 5000 };   // in msec. if > 0 wait duration after a lock, if < 0 duration is set to 'wait' unless there is no more activity
			uint32_t lock_wait { 500 }; // in msec. Represent the maximum amount of time we will wait for a lock to complete before switching to next
			int32_t def_step { 0 };
			freqman_db frequency_list  = { };
			uint32_t current_index { 0 };
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
			// maximum usable freq
			long long int MAX_UFREQ = { 7200000000 };
			uint32_t recon_lock_nb_match = { 10 };
			uint32_t recon_lock_duration = { 50 };
			uint32_t recon_match_mode = { 0 };
			bool scanner_mode { false };
			bool manual_mode { false };
			bool sd_card_mounted = false ;
			int32_t volume = 40 ;

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
					{ -9000, 9000 },
					100,
					' ',
			};

			NumberField field_lock_wait {
				{ 26 * 8, 1 * 16 },
					4,
					{ 100 , 9000 },
					100,
					' ',
			};

			RSSI rssi {
				{ 0 * 16, 2 * 16, 240 - 5 * 8 - 1 , 16 },
			}; 

			ButtonWithEncoder text_cycle {
				{ 0, 3 * 16, 3 * 8, 16 },  
					""
			};

			Text text_max {
				{ 3 * 8, 3 * 16, 20 * 8 , 16 },  
			};

			Text desc_cycle {
				{0, 4 * 16, 240 , 16 },	   
			};

			/* BigFrequency big_display {		//Show frequency in glamour
			   { 4, 7 * 16 - 8 , 28 * 8, 52 },
			   0
			   }; */

			Text big_display {		//Show frequency in text mode
				{ 0, 5 * 16 , 21 * 8, 16 },
			};

			Text freq_stats {		//Show frequency stats in text mode
				{ 0, 6 * 16 , 21 * 8, 16 },
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
				{ 240 - 5 * 8 , 2 * 16 , 5 * 8, 28 },
					"OPT"
			};

			Button button_looking_glass {
				{ 240 - 5 * 8 , 5 * 16 , 5 * 8, 28 },
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

			std::unique_ptr<ReconThread> recon_thread { };

			MessageHandlerRegistration message_handler_coded_squelch {
				Message::ID::CodedSquelch,
					[this](const Message* const p) {
						const auto message = *reinterpret_cast<const CodedSquelchMessage*>(p);
						this->handle_coded_squelch(message.value);
					}
			};

			MessageHandlerRegistration message_handler_retune {
				Message::ID::Retune,
					[this](const Message* const p) {
						const auto message = *reinterpret_cast<const RetuneMessage*>(p);
						this->handle_retune(message.freq,message.range);
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
