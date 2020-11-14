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

#define SETTINGS_NAME_MAX_LEN

using namespace std;
using namespace portapack;

namespace ui {

	bool SearchSetupLoadStrings( std::string source, std::string &input_file , std::string &output_file )
	{
		File settings_file;
		size_t length, n = 0, file_position = 0;
		char * pos;
		char * line_start;
		char * line_end;
		char file_data[257];

		uint32_t it = 0 ;
		uint32_t nb_params = 2 ;
		std::string params[ 2 ];

		auto result = settings_file.open( source );
		if( !result.is_valid() )
		{
			while( it < nb_params )
			{
				// Read a 256 bytes block from file
				settings_file.seek(file_position);
				memset(file_data, 0, 257);
				auto read_size = settings_file.read(file_data, 256);
				if (read_size.is_error())
					break ;
				file_position += 256;
				// Reset line_start to beginning of buffer
				line_start = file_data;
				pos=line_start;
				while ((line_end = strstr(line_start, "\x0A"))) {
					length = line_end - line_start - 1 ;
					params[ it ]  = string( pos , length );
					it ++ ;	
					line_start = line_end + 1;
					pos=line_start ;
					if (line_start - file_data >= 256) 
						break;
					if( it >= nb_params )
						break ;
				}			
				if (read_size.value() != 256)
					break;	// End of file

				// Restart at beginning of last incomplete line
				file_position -= (file_data + 256 - line_start);
			}
		}
		if( it < nb_params )
		{
			/* bad number of params, setting default */
			input_file  = "SEARCH.TXT" ;
			output_file = "SEARCHRESULT" ;	
			return false ;
		}
		input_file = params[ 0 ];
		output_file= params[ 1 ];
		return true ;
	}

	bool SearchSetupSaveStrings( std::string dest, std::string input_file , std::string output_file )
	{
		File settings_file;

		auto result = settings_file.create( dest );
		if( result.is_valid() )
			return false ;
		settings_file.write_line( input_file );
		settings_file.write_line( output_file );
		return true ;
	}

	void SearchSetupView::focus() {
		button_load_freqs.focus();
	}

	SearchSetupView::SearchSetupView(
			NavigationView& nav
			) : nav_ { nav }

	{
		add_children({
				&button_load_freqs,
				&text_input_file,
				&button_save_freqs,
				&text_output_file,
				&checkbox_autosave_freqs,
				&checkbox_autostart_search,
				&checkbox_powersave,
				&checkbox_filemode,
				&button_save
				});

		checkbox_autosave_freqs.set_value( persistent_memory::search_autosave_freqs() );
		checkbox_autostart_search.set_value( persistent_memory::search_autostart_search() );
		checkbox_powersave.set_value( persistent_memory::search_powersave() );
		checkbox_filemode.set_value( persistent_memory::search_filemode() );

		SearchSetupLoadStrings( "SEARCH/SEARCH.CFG" , input_file , output_file );

		text_input_file.set( input_file );	
		text_output_file.set( output_file );	

		button_load_freqs.on_select = [this, &nav](Button&) {
			auto open_view = nav.push<FileLoadView>(".TXT");
			open_view->on_changed = [this](std::filesystem::path new_file_path) {
				std::string dir_filter = "FREQMAN/";
				std::string str_file_path = new_file_path.string();
				if (str_file_path.find(dir_filter) != string::npos) { // assert file from the FREQMAN folder
					// get the filename without txt extension so we can use load_freqman_file fcn
					input_file = new_file_path.stem().string();
					text_input_file.set( input_file );
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
					output_file = new_file_path.stem().string();
					text_output_file.set( output_file );
				} else {
					nav_.display_modal("LOAD ERROR", "A valid file from\nFREQMAN directory is\nrequired.");
				}
			};
		};

		button_save.on_select = [this,&nav](Button&) {
			persistent_memory::set_search_autosave_freqs(checkbox_autosave_freqs.value());
			persistent_memory::set_search_autostart_search(checkbox_autostart_search.value());
			persistent_memory::set_search_powersave(checkbox_powersave.value()); 
			persistent_memory::set_search_filemode(checkbox_filemode.value()); 
			SearchSetupSaveStrings( "SEARCH/SEARCH.CFG" , input_file , output_file );
			nav.pop();
		};
	}

} /* namespace ui */
