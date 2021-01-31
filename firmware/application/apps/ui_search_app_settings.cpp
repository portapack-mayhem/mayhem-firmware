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

#include "ui_search_app_settings.hpp"
#include "ui_navigation.hpp"
#include "ui_fileman.hpp"
#include "ui_textentry.hpp"

#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"

using namespace std;
using namespace portapack;

namespace ui {

	bool SearchAppSetupLoadStrings( std::string source, std::string &input_file , std::string &output_file )
	{
		File settings_file;
		size_t length, file_position = 0;
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
			input_file  = "SEARCH" ;
			output_file = "SEARCHRESULT" ;	
			return false ;
		}
		input_file = params[ 0 ];
		output_file= params[ 1 ];
		return true ;
	}

	bool SearchAppSetupSaveStrings( std::string dest, std::string input_file , std::string output_file )
	{
		File settings_file;

		auto result = settings_file.create( dest );
		if( result.is_valid() )
			return false ;
		settings_file.write_line( input_file );
		settings_file.write_line( output_file );

		return true ;
	}



	SearchAppSetupViewMain::SearchAppSetupViewMain(
			NavigationView &nav , Rect parent_rect , std::string input_file , std::string output_file 
			) : View( parent_rect ) , _input_file { input_file } , _output_file { output_file } 
	{
		hidden(true);
		add_children({
				&button_load_freqs,
				&text_input_file,
				&button_save_freqs,
				&button_output_file,
				&checkbox_autosave_freqs,
				&checkbox_autostart_search,
				&checkbox_continuous,
				&checkbox_clear_output,
				});

		checkbox_autosave_freqs.set_value( persistent_memory::search_app_autosave_freqs() );
		checkbox_autostart_search.set_value( persistent_memory::search_app_autostart_search() );
		checkbox_continuous.set_value( persistent_memory::search_app_continuous() );
		checkbox_clear_output.set_value( persistent_memory::search_app_clear_output() );

		text_input_file.set( _input_file );	
		button_output_file.set_text( _output_file );	

		button_load_freqs.on_select = [this, &nav](Button&) {
			auto open_view = nav.push<FileLoadView>(".TXT");
			open_view->on_changed = [this,&nav](std::filesystem::path new_file_path) {
				std::string dir_filter = "FREQMAN/";
				std::string str_file_path = new_file_path.string();
				if (str_file_path.find(dir_filter) != string::npos) { // assert file from the FREQMAN folder
					// get the filename without txt extension so we can use load_freqman_file fcn
					_input_file = new_file_path.stem().string();
					text_input_file.set( _input_file );
				} else {
					nav.display_modal("LOAD ERROR", "A valid file from\nFREQMAN directory is\nrequired.");
				}
			};
		};

		button_save_freqs.on_select = [this, &nav](Button&) {
			auto open_view = nav.push<FileLoadView>(".TXT");
			open_view->on_changed = [this,&nav](std::filesystem::path new_file_path) {
				std::string dir_filter = "FREQMAN/";
				std::string str_file_path = new_file_path.string();
				if (str_file_path.find(dir_filter) != string::npos) { // assert file from the FREQMAN folder
					_output_file = new_file_path.stem().string();
					button_output_file.set_text( _output_file );	
				} else {
					nav.display_modal("LOAD ERROR", "A valid file from\nFREQMAN directory is\nrequired.");
				}
			};
		};

		button_output_file.on_select =[this, &nav](Button&) {
			text_prompt( nav, _output_file , 28 ,
					[this](std::string& buffer) {
					_output_file = buffer ;
					button_output_file.set_text( _output_file );
					}   );
		};
	};

	void SearchAppSetupViewMain::Save( std::string &input_file , std::string &output_file ) {
		persistent_memory::set_search_app_autosave_freqs(checkbox_autosave_freqs.value());
		persistent_memory::set_search_app_autostart_search(checkbox_autostart_search.value());
		persistent_memory::set_search_app_continuous(checkbox_continuous.value()); 
		persistent_memory::set_search_app_clear_output(checkbox_clear_output.value()); 
		SearchAppSetupSaveStrings( "SEARCH/SEARCH.CFG" , _input_file , _output_file );
		input_file=_input_file ;
		output_file=_output_file ;
	};
	void SearchAppSetupViewMore::Save() {
		persistent_memory::set_search_app_load_freqs(checkbox_load_freqs.value()); 
		persistent_memory::set_search_app_load_ranges(checkbox_load_ranges.value()); 
		persistent_memory::set_search_app_update_ranges_when_searching(checkbox_update_ranges_when_searching.value()); 
	};

	void SearchAppSetupViewMain::focus() {
		button_load_freqs.focus();
	}

	SearchAppSetupViewMore::SearchAppSetupViewMore(
			Rect parent_rect
			) : View( parent_rect ) {

		hidden(true);

		add_children({
				&checkbox_load_freqs,
				&checkbox_load_ranges,
				&checkbox_update_ranges_when_searching
				});

		checkbox_load_freqs.set_value( persistent_memory::search_app_load_freqs() );
		checkbox_load_ranges.set_value( persistent_memory::search_app_load_ranges() );
		checkbox_update_ranges_when_searching.set_value( persistent_memory::search_app_update_ranges_when_searching() );
	};

	void SearchAppSetupViewMore::focus() {
		checkbox_load_freqs.focus();
	}

	void SearchAppSetupView::focus() {
		viewMain.focus();
	}

	SearchAppSetupView::SearchAppSetupView(
			NavigationView& nav , std::string _input_file , std::string _output_file 
			) : nav_ { nav } , input_file { _input_file } , output_file { _output_file }

	{
		add_children({
				&tab_view,
				&viewMain,
				&viewMore,
				&button_save
				});

		button_save.on_select = [this,&nav](Button&) {
			viewMain.Save(input_file,output_file);
			viewMore.Save();
			//SearchAppSetupLoadStrings( "SEARCH/SEARCH.CFG" , input_file , output_file );
			std::vector<std::string> messages ;
			messages.push_back( input_file );
			messages.push_back( output_file );
			on_changed( messages );
			nav.pop();
		};
	}
} /* namespace ui */
