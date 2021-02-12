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

#ifndef _UI_SEARCH_APP
#define _UI_SEARCH_APP

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
#include "string_format.hpp"
#include "file.hpp"


namespace ui {

	class SearchAppThread {
		public:
			SearchAppThread(freqman_db *database );
			~SearchAppThread();

			void set_searching(const bool v);
			bool is_searching();

			void set_freq_lock(const uint32_t v);
			uint32_t is_freq_lock();
			int64_t get_current_freq();

			void set_stepper(const int64_t v);

			void change_searching_direction();
			bool get_searching_direction();
			void set_searching_direction( const bool v);

			void set_continuous(const bool v);
			uint8_t get_current_modulation();

			void run();
			void stop();

			SearchAppThread(const SearchAppThread&) = delete;
			SearchAppThread(SearchAppThread&&) = delete;
			SearchAppThread& operator=(const SearchAppThread&) = delete;
			SearchAppThread& operator=(SearchAppThread&&) = delete;

		private:
			freqman_db &frequency_list_ ;
			Thread* thread { nullptr };
			int64_t freq = 0 ;
			uint32_t step = 0 ;
			uint8_t modulation  = 0 ;
			uint8_t bandwidth = 0 ;
			uint16_t tone = 0 ;
			freqman_entry last_entry = { } ;

			bool _searching { true };
			bool _fwd { true };
			bool _continuous { true };
			int64_t _stepper { 0 };
			int32_t _freq_lock { 0 };
			static msg_t static_fn(void* arg);
	};

	class SearchAppView : public View {
		public:
			SearchAppView(NavigationView& nav);
			~SearchAppView();

			void focus() override;

			void big_display_freq( int64_t f );

			const Style style_grey {		// searching
				.font = font::fixed_8x16,
					.background = Color::black(),
					.foreground = Color::grey(),
			};

			const Style style_white {		// searching
				.font = font::fixed_8x16,
					.background = Color::black(),
					.foreground = Color::white(),
			};

			const Style style_yellow {		//Found signal
				.font = font::fixed_8x16,
					.background = Color::black(),
					.foreground = Color::dark_yellow(),
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

			std::string title() const override { return "Search"; };


			//void set_parent_rect(const Rect new_parent_rect) override;

		private:
			NavigationView& nav_;

			void start_search_thread();
			size_t change_mode( int8_t mod_type);
			void show_max();
			void search_pause();
			void search_resume();
			void user_resume();
			void frequency_file_load(std::string file_name, bool stop_all_before = false);
			void on_statistics_update(const ChannelStatistics& statistics);
			void on_headphone_volume_changed(int32_t v);
			void handle_retune( int64_t freq , uint32_t index );

			jammer::jammer_range_t frequency_range { false, 0, 0 };  //perfect for manual search task too...
			int32_t squelch { 0 };
			uint32_t timer { 0 };
			uint32_t wait { 0 };
			int32_t def_step { 0 };
			freqman_db frequency_list  = { };
			uint32_t current_index { 0 };
			bool userpause { false };
			std::string input_file = { "SEARCH" };
			std::string output_file = { "SEARCHRESULTS" };
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
			long long int MAX_UFREQ = { 7000000000 };
			//50ms cycles search locks into freq when signal detected, to verify signal is not spureous
			uint8_t SEARCH_APP_MAX_FREQ_LOCK = { 10 };

			Labels labels {
				{ { 0 * 8, 0 * 16 }, "LNA:   VGA:   AMP:  VOL:", Color::light_grey() },
					{ { 0 * 8, 1* 16 }, "BW:    SQUELCH:   db WAIT:", Color::light_grey() },
					{ { 3 * 8, 10 * 16 }, "START       END     MANUAL", Color::light_grey() },
					{ { 0 * 8, (26 * 8) + 4 }, "MODE:", Color::light_grey() },
					{ { 11 * 8, (26 * 8) + 4 }, "STEP:", Color::light_grey() },
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
					4,
					{ }
			};		

			NumberField field_squelch {
				{ 15 * 8, 1 * 16 },
					3,
					{ -90, 20 },
					1,
					' ',
			};

			NumberField field_wait {
				{ 26 * 8, 1 * 16 },
					2,
					{ 0, 99 },
					1,
					' ',
			};

			RSSI rssi {
				{ 0 * 16, 2 * 16, 15 * 16, 8 },
			}; 

			Text text_cycle {
				{ 0, 3 * 16, 3 * 8, 16 },  
			};

			Text text_max {
				{ 4 * 8, 3 * 16, 18 * 8, 16 },  
			};

			Text desc_cycle {
				{0, 4 * 16, 240, 16 },	   
			};

			BigFrequency big_display {		//Show frequency in glamour
				{ 4, 6 * 16, 28 * 8, 52 },
					0
			};

			Button button_search_setup {
				{ 22 * 8 + 6, 3 * 16 - 8, 7 * 8, 22 },
					"PARAMS"
			};

			ButtonWithEncoder button_manual_start {
				{ 0 * 8, 11 * 16, 11 * 8, 28 },
					""
			};

			ButtonWithEncoder button_manual_end {
				{ 12 * 8 - 6, 11 * 16, 11 * 8, 28 },
					""
			};

			Button button_manual_search {
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

			Button button_add {
				{ 168, (15 * 16) - 4, 72, 28 },
					"STORE"
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

			Button button_remove {
				{ 168, (35 * 8) - 4, 72, 28 },
					"REMOVE"
			};

			std::unique_ptr<SearchAppThread> search_thread { };

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
	};

} /* namespace ui */

#endif
