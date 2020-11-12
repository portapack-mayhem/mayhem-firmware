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

#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;

namespace ui {

void SearchSetupView::focus() {
}

SearchSetupView::SearchSetupView(
	NavigationView& nav
)
{
	add_children({
		&labels,
		&button_load_freqs,
		&text_loaded_file,
		&button_save_freqs,
		&text_save_to_file,
		&checkbox_autosave_freqs,
		&checkbox_autorotate_file,
		&button_save
	});

	std::string input_freq_file { "SEARCH.TXT" };
	std::string output_freq_file { "SEARCHFINDS.TXT" };
	
	bool autosave_freqs = persistent_memory::search_autosave_freqs();
	bool autorotate_file = persistent_memory::search_autorotate_file();

	uint32_t nb_max_freqs = 500 ; /* hard coded, experience will show if max value is too high */
	uint32_t nb_freqs     = persistent_memory::search_nb_freqs();
	
//	checkbox_autosave_freqs =  persistent_memory::search_autosave_freqs();
//	checkbox_autorotate_file = persistent_memory::search_autorotate_file();
	
	button_save.on_select = [this,&nav](Button&) {
		persistent_memory::set_search_autosave_freqs(checkbox_autosave_freqs.value());
		persistent_memory::set_search_autorotate_file(checkbox_autorotate_file.value());
		//persistent_memory::set_search_nb_freqs(  );
		nav.pop();
	};
}

} /* namespace ui */
