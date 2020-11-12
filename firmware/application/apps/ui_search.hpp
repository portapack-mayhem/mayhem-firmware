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


#define MAX_DB_ENTRY 500
#define MAX_FREQ_LOCK 10 		//50ms cycles search locks into freq when signal detected, to verify signal is not spureous

/* indexes used in frequency_list in case of a manual search */
#define MS_FLAG_IDX  0       // position of flag in list
#define MS_FLAG_VAL  -666    // value of flag to tell it's a manual search ordered frequency_list
#define MS_MIN_FREQ_IDX  1   // position of minimum frequency
#define MS_MAX_FREQ_IDX  2   // position of maximum frequency
#define MS_FREQ_STEP_IDX 3   // position or step 
#define MS_CUR_FREQ_IDX  4   // position of currently used frequency

namespace ui {

enum search_modulation_type { SEARCH_AM = 0,SEARCH_WFM,SEARCH_NFM };
string const search_mod_name[3] = {"AM", "WFM", "NFM"};
size_t const search_mod_step[3] = {9000, 100000, 12500 };

class SearchThread {
public:
	SearchThread(std::vector<rf::Frequency> frequency_list);
	~SearchThread();

	void set_searching(const bool v);
	bool is_searching();

	void set_freq_lock(const uint32_t v);
	uint32_t is_freq_lock();

	void set_freq_del(const uint32_t v);

	void change_searching_direction();

	void stop();

	SearchThread(const SearchThread&) = delete;
	SearchThread(SearchThread&&) = delete;
	SearchThread& operator=(const SearchThread&) = delete;
	SearchThread& operator=(SearchThread&&) = delete;

private:
	std::vector<rf::Frequency> frequency_list_ { };
	Thread* thread { nullptr };
	
	bool _searching { false };
	bool _fwd { true };
	uint32_t _freq_lock { 0 };
	uint32_t _freq_del { 0 };
	static msg_t static_fn(void* arg);
	void run();
};

class SearchView : public View {
public:
	SearchView(NavigationView& nav);
	~SearchView();
	
	void focus() override;

	void big_display_freq(rf::Frequency f);

	const Style style_grey {		// searching
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::grey(),
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
	std::vector<rf::Frequency> frequency_list{ };
	std::vector<string> description_list { };

//void set_parent_rect(const Rect new_parent_rect) override;

private:
	NavigationView& nav_;

	void start_search_thread();
	size_t change_mode(uint8_t mod_type);
	void show_max();
	void search_pause();
	void search_resume();
	void user_resume();
	void frequency_file_load(std::string file_name, bool stop_all_before = false);

	void on_statistics_update(const ChannelStatistics& statistics);
	void on_headphone_volume_changed(int32_t v);
	void handle_retune( long i );

	jammer::jammer_range_t frequency_range { false, 0, 0 };  //perfect for manual search task too...
	int32_t squelch { 0 };
	uint32_t timer { 0 };
	uint32_t wait { 0 };
	size_t	def_step { 0 };
	freqman_db database { };
	std::string loaded_file_name;
	uint32_t current_index { 0 };
	bool userpause { false };
	bool manual_search { false }; 
	
	Labels labels {
		{ { 0 * 8, 0 * 16 }, "LNA:   VGA:   AMP:  VOL:", Color::light_grey() },
		{ { 0 * 8, 1* 16 }, "BW:    SQUELCH:   db WAIT:", Color::light_grey() },
		{ { 3 * 8, 10 * 16 }, "START        END     MANUAL", Color::light_grey() },
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

	Button button_manual_start {
		{ 0 * 8, 11 * 16, 11 * 8, 28 },
		""
	};

	Button button_manual_end {
		{ 12 * 8, 11 * 16, 11 * 8, 28 },
		""
	};

	Button button_manual_search {
		{ 24 * 8, 11 * 16, 6 * 8, 28 },
		"SEARCH"
	};

	OptionsField field_mode {
		{ 5 * 8, (26 * 8) + 4 },
		6,
		{
			{ " AM  ", 0 },
			{ " WFM ", 1 },
			{ " NFM ", 2 },
		}
	};

	OptionsField step_mode {
		{ 17 * 8, (26 * 8) + 4 },
		12,
		{
			{ "5Khz (SA AM)", 	5000 },
			{ "9Khz (EU AM)", 	9000 },
			{ "10Khz(US AM)", 	10000 },
			{ "50Khz (FM1)", 	50000 },
			{ "100Khz(FM2)", 	100000 },
			{ "6.25khz(NFM)",	6250 },
			{ "12.5khz(NFM)",	12500 },
			{ "25khz (N1)",		25000 },
			{ "250khz (N2)",	250000 },
			{ "8.33khz(AIR)",	8330 }
		}
	};

	Button button_pause {
		{ 0, (15 * 16) - 4, 72, 28 },
		"PAUSE"
	};

	Button button_dir {
		{ 0,  (35 * 8) - 4, 72, 28 },
		"FW><RV"
	};

	Button button_audio_app {
		{ 84, (15 * 16) - 4, 72, 28 },
		"AUDIO"
	};

	Button button_mic_app {
		{ 84,  (35 * 8) - 4, 72, 28 },
		"MIC TX"
	};

	Button button_add {
		{ 168, (15 * 16) - 4, 72, 28 },
		"ADD FQ"
	};


	Button button_remove {
		{ 168, (35 * 8) - 4, 72, 28 },
		"DEL FQ"
	};
	Button button_search_setup {
		{ 12 * 8, 1 * 16, 96, 24 },
		"Settings"
	};
	Button button_load {
		{ 24 * 8, 3 * 16 - 8, 6 * 8, 22 },
		"Load"
	};


	
	std::unique_ptr<SearchThread> search_thread { };
	
	MessageHandlerRegistration message_handler_retune {
		Message::ID::Retune,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const RetuneMessage*>(p);
			this->handle_retune(message.range);
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
