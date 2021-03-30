/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "ui_freqman_settings.hpp"
#include "ui_navigation.hpp"
#include "ui_fileman.hpp"
#include "ui_textentry.hpp"

#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"

using namespace std;
using namespace portapack;

namespace ui {

	FreqmanSetupViewFreq::FreqmanSetupViewFreq(
			NavigationView &nav , Rect parent_rect , frewman_entry &rf_entry
			) : View( parent_rect ) , _rf_entry { rf_entry }  
	{
		hidden(true);
		add_children({
				&text_entry_type,
				&button_freq,
				&field_mode,
				&field_bw,
				});
		/* set default values */



		/* set actions */
		
	};

	void FreqmanSetupViewFreq::Save( std::string &input_file , std::string &output_file ) {
		rf_freq =_rf_freq ;
	};

	void FreqmanSetupViewFreq::focus() {
		button_load_freqs.focus();
	}

	FreqmanSetupViewRange::FreqmanSetupViewRange(
			Rect parent_rect
			) : View( parent_rect ) {

		hidden(true);

		add_children({
				&checkbox_load_freqs,
				&checkbox_load_ranges,
				&checkbox_load_hamradios,
				&checkbox_update_ranges_when_searching
				});

		checkbox_load_freqs.set_value( persistent_memory::search_app_load_freqs() );
		checkbox_load_ranges.set_value( persistent_memory::search_app_load_ranges() );
		checkbox_load_hamradios.set_value( persistent_memory::search_app_load_hamradios() );
		checkbox_update_ranges_when_searching.set_value( persistent_memory::search_app_update_ranges_when_searching() );
	};

	void FreqmanSetupViewRange::focus() {
		checkbox_load_freqs.focus();
	}

	void FreqmanSetupViewRange::Save() {
		persistent_memory::set_search_app_load_freqs(checkbox_load_freqs.value()); 
		persistent_memory::set_search_app_load_ranges(checkbox_load_ranges.value()); 
		persistent_memory::set_search_app_load_hamradios(checkbox_load_hamradios.value()); 
		persistent_memory::set_search_app_update_ranges_when_searching(checkbox_update_ranges_when_searching.value()); 
	};

	FreqmanSetupViewHamRadio::FreqmanSetupViewHamRadio(
			Rect parent_rect
			) : View( parent_rect ) {

		hidden(true);

		add_children({
				&checkbox_load_freqs,
				&checkbox_load_ranges,
				&checkbox_load_hamradios,
				&checkbox_update_ranges_when_searching
				});

		checkbox_load_freqs.set_value( persistent_memory::search_app_load_freqs() );
		checkbox_load_ranges.set_value( persistent_memory::search_app_load_ranges() );
		checkbox_load_hamradios.set_value( persistent_memory::search_app_load_hamradios() );
		checkbox_update_ranges_when_searching.set_value( persistent_memory::search_app_update_ranges_when_searching() );
	};

	void FreqmanSetupViewHamRadio::focus() {
		checkbox_load_freqs.focus();
	}

	void FreqmanSetupViewHamRadio::Save() {
		persistent_memory::set_search_app_load_freqs(checkbox_load_freqs.value()); 
		persistent_memory::set_search_app_load_ranges(checkbox_load_ranges.value()); 
		persistent_memory::set_search_app_load_hamradios(checkbox_load_hamradios.value()); 
		persistent_memory::set_search_app_update_ranges_when_searching(checkbox_update_ranges_when_searching.value()); 
	};


	FreqmanSetupView::FreqmanSetupView(
			NavigationView& nav , std::string _input_file , std::string _output_file 
			) : nav_ { nav } , input_file { _input_file } , output_file { _output_file }

	{
		add_children({
				&tab_view,
				&viewFreq,
				&viewRange,
				&viewHamRadio,
				&button_save
				});

		button_save.on_select = [this,&nav](Button&) {
			viewFreq.Save(input_file,output_file);
			viewRange.Save();
			viewHamRadio.Save();
			//SearchAppSetupLoadStrings( "SEARCH/SEARCH.CFG" , input_file , output_file );
			std::vector<std::string> messages ;
			messages.push_back( input_file );
			messages.push_back( output_file );
			on_changed( messages );
			nav.pop();
		};
	}

	void FreqmanSetupView::focus() {
		viewFreq.focus();
	}
} /* namespace ui */
