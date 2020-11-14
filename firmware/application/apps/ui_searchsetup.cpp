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

#include "ui_searchsetup.hpp"
#include "ui_navigation.hpp"
#include "ui_fileman.hpp"

#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"

using namespace std;
using namespace portapack;

namespace ui {

	void SearchSetupView::focus() {
		button_load_freqs.focus();
	}

	SearchSetupView::SearchSetupView(
			NavigationView& nav
			) : nav_ { nav }

	{
		add_children({
				&labels,
				&button_load_freqs,
				&text_input_file,
				&button_save_freqs,
				&text_output_file,
				&checkbox_autosave_freqs,
				&checkbox_autorotate_file,
				&checkbox_autostart_search,
				&button_save
				});

		checkbox_autosave_freqs.set_value( persistent_memory::search_autosave_freqs() );
		checkbox_autorotate_file.set_value( persistent_memory::search_autorotate_file() );
		checkbox_autostart_search.set_value( persistent_memory::search_autostart_search() );

		text_input_file.set( "/FREQMAN/SEARCH.TXT" );	
		text_output_file.set( "/FREQMAN/SEARCHFINDS.TXT" );	

		button_load_freqs.on_select = [this, &nav](Button&) {
			auto open_view = nav.push<FileLoadView>(".TXT");
			open_view->on_changed = [this](std::filesystem::path new_file_path) {
				std::string dir_filter = "/FREQMAN/";
				std::string str_file_path = new_file_path.string();
				if (str_file_path.find(dir_filter) != string::npos) { // assert file from the FREQMAN folder
					text_input_file.set( str_file_path );
				} else {
					nav_.display_modal("LOAD ERROR", "A valid file from\nFREQMAN directory is\nrequired.");
				}
			};
		};

		button_save_freqs.on_select = [this, &nav](Button&) {
			auto open_view = nav.push<FileLoadView>(".TXT");
			open_view->on_changed = [this](std::filesystem::path new_file_path) {
				std::string dir_filter = "FREQMAN/";
				std::string str_file_path = new_file_path.string();
				if (str_file_path.find(dir_filter) != string::npos) { // assert file from the FREQMAN folder
					text_output_file.set( str_file_path );
				} else {
					nav_.display_modal("LOAD ERROR", "A valid file from\nFREQMAN directory is\nrequired.");
				}
			};
		};

		button_save.on_select = [this,&nav](Button&) {

			/* add save input_freq_file */
			/* add save output_freq_file */
			persistent_memory::set_search_autosave_freqs(checkbox_autosave_freqs.value());
			persistent_memory::set_search_autorotate_file(checkbox_autorotate_file.value());
			persistent_memory::set_search_autostart_search(checkbox_autostart_search.value()); 
			persistent_memory::set_search_nb_freqs( 500 );
			nav.pop();
		};
	}

} /* namespace ui */
