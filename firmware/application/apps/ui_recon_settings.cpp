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

#include "ui_recon_settings.hpp"
#include "ui_navigation.hpp"
#include "ui_fileman.hpp"
#include "ui_textentry.hpp"

#include "file.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"

using namespace std;
using namespace portapack;

namespace ui {

	bool ReconSetupLoadStrings( std::string source, std::string &input_file , std::string &output_file , uint32_t &recon_lock_duration , uint32_t &recon_lock_nb_match , int32_t &recon_squelch_level , uint32_t &recon_match_mode , int32_t &wait , uint32_t &lock_wait , int32_t &volume )
	{
		File settings_file;
		size_t length, file_position = 0;
		char * pos;
		char * line_start;
		char * line_end;
		char file_data[257];

		uint32_t it = 0 ;
		uint32_t nb_params = 9 ;
		std::string params[ 9 ];

		bool check_sd_card = (sd_card::status() == sd_card::Status::Mounted) ? true : false ;

		if( check_sd_card )
		{
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

		}

		if( it > 0 )
			input_file = params[ 0 ];
		else
			input_file  = "RECON" ;

		if( it > 1 )
			output_file= params[ 1 ];
		else
			output_file = "RECON_RESULTS" ;	

		if( it > 2 )
			recon_lock_duration = strtoll( params[ 2 ].c_str() , nullptr , 10 );
		else
			recon_lock_duration = 50 ;

		if( it > 3 )
			recon_lock_nb_match = strtoll( params[ 3 ].c_str() , nullptr , 10 );
		else
			recon_lock_nb_match = 10 ;

		if( it > 4 )
			recon_squelch_level = strtoll( params[ 4 ].c_str() , nullptr , 10 );
		else
			recon_squelch_level = -14 ;

		if( it > 5 )
			recon_match_mode    = strtoll( params[ 5 ].c_str() , nullptr , 10 );
		else
			recon_match_mode = 0 ;

		if( it > 6 )
			wait = strtoll( params[ 6 ].c_str() , nullptr , 10 );
		else
			wait = 5000 ;

		if( it > 7 )
			lock_wait = strtoll( params[ 7 ].c_str() , nullptr , 10 );
		else
			lock_wait = 1000 ;

		if( it > 8 )
			volume = strtoll( params[ 8 ].c_str() , nullptr , 10 );
		else
			volume = 40 ;

		if( it < nb_params )
		{
			/* bad number of params, signal defaults */
			return false ;
		}
		return true ;
	}

	bool ReconSetupSaveStrings( std::string dest, std::string input_file , std::string output_file , uint32_t recon_lock_duration , uint32_t recon_lock_nb_match , int32_t recon_squelch_level , uint32_t recon_match_mode , int32_t wait , uint32_t lock_wait , int32_t volume )
	{
		File settings_file;

		auto result = settings_file.create( dest );
		if( result.is_valid() )
			return false ;
		settings_file.write_line( input_file );
		settings_file.write_line( output_file );
		settings_file.write_line( to_string_dec_uint( recon_lock_duration ) );
		settings_file.write_line( to_string_dec_uint( recon_lock_nb_match ) );
		settings_file.write_line( to_string_dec_int( recon_squelch_level ) );
		settings_file.write_line( to_string_dec_uint( recon_match_mode ) );
		settings_file.write_line( to_string_dec_int( wait ) );
		settings_file.write_line( to_string_dec_uint( lock_wait ) );
		settings_file.write_line( to_string_dec_int( volume ) );
		return true ;
	}

	ReconSetupViewMain::ReconSetupViewMain( NavigationView &nav , Rect parent_rect , std::string input_file , std::string output_file ) : View( parent_rect ) , _input_file { input_file } , _output_file { output_file } 
	{
		hidden(true);
		add_children({
				&button_load_freqs,
				&text_input_file,
				&button_save_freqs,
				&button_output_file,
				&checkbox_autosave_freqs,
				&checkbox_autostart_recon,
				&checkbox_continuous,
				&checkbox_clear_output
				});

		checkbox_autosave_freqs.set_value( persistent_memory::recon_autosave_freqs() );
		checkbox_autostart_recon.set_value( persistent_memory::recon_autostart_recon() );
		checkbox_continuous.set_value( persistent_memory::recon_continuous() );
		checkbox_clear_output.set_value( persistent_memory::recon_clear_output() );

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

	void ReconSetupViewMain::Save( std::string &input_file , std::string &output_file ) {
		persistent_memory::set_recon_autosave_freqs(checkbox_autosave_freqs.value());
		persistent_memory::set_recon_autostart_recon(checkbox_autostart_recon.value());
		persistent_memory::set_recon_continuous(checkbox_continuous.value()); 
		persistent_memory::set_recon_clear_output(checkbox_clear_output.value()); 
		input_file=_input_file ;
		output_file=_output_file ;
	};
	void ReconSetupViewMore::Save( uint32_t &recon_lock_duration , uint32_t &recon_lock_nb_match , uint32_t &recon_match_mode ) {
		persistent_memory::set_recon_load_freqs(checkbox_load_freqs.value()); 
		persistent_memory::set_recon_load_ranges(checkbox_load_ranges.value()); 
		persistent_memory::set_recon_load_hamradios(checkbox_load_hamradios.value()); 
		persistent_memory::set_recon_update_ranges_when_recon(checkbox_update_ranges_when_recon.value()); 
		recon_lock_duration = field_recon_lock_duration.value();
		recon_lock_nb_match = field_recon_lock_nb_match.value();
		recon_match_mode    = field_recon_match_mode . selected_index_value() ;
	};

	void ReconSetupViewMain::focus() {
		button_load_freqs.focus();
	}

	ReconSetupViewMore::ReconSetupViewMore( NavigationView &nav , Rect parent_rect , uint32_t recon_lock_duration , uint32_t recon_lock_nb_match , uint32_t recon_match_mode ) : View( parent_rect ), _recon_lock_duration { recon_lock_duration } , _recon_lock_nb_match { recon_lock_nb_match } , _recon_match_mode { recon_match_mode } 
	{
		(void)nav;
		hidden(true);

		add_children({
				&checkbox_load_freqs,
				&checkbox_load_ranges,
				&checkbox_load_hamradios,
				&checkbox_update_ranges_when_recon,
				&text_recon_lock_duration,
				&field_recon_lock_duration,
				&text_recon_lock_nb,
				&field_recon_lock_nb_match,
				&field_recon_match_mode,
				});

		checkbox_load_freqs.set_value( persistent_memory::recon_load_freqs() );
		checkbox_load_ranges.set_value( persistent_memory::recon_load_ranges() );
		checkbox_load_hamradios.set_value( persistent_memory::recon_load_hamradios() );
		checkbox_update_ranges_when_recon.set_value( persistent_memory::recon_update_ranges_when_recon() );
		field_recon_lock_duration.set_value( _recon_lock_duration ); 
		field_recon_lock_nb_match.set_value( _recon_lock_nb_match ); 
		field_recon_match_mode.set_by_value( _recon_match_mode ); 
	};

	void ReconSetupViewMore::focus() {
		checkbox_load_freqs.focus();
	}

	void ReconSetupView::focus() {
		viewMain.focus();
	}

	ReconSetupView::ReconSetupView(
			NavigationView& nav , std::string _input_file , std::string _output_file , uint32_t _recon_lock_duration , uint32_t _recon_lock_nb_match , uint32_t _recon_match_mode ) : nav_ { nav } , input_file { _input_file } , output_file { _output_file } , recon_lock_duration { _recon_lock_duration } , recon_lock_nb_match { _recon_lock_nb_match } , recon_match_mode { _recon_match_mode } 
	{
		add_children({
				&tab_view,
				&viewMain,
				&viewMore,
				&button_save
				});

		button_save.on_select = [this,&nav](Button&) {
			viewMain.Save( input_file , output_file );
			viewMore.Save( recon_lock_duration , recon_lock_nb_match , recon_match_mode );
			std::vector<std::string> messages ;
			messages.push_back( input_file );
			messages.push_back( output_file );
			messages.push_back( to_string_dec_uint( recon_lock_duration ) );
			messages.push_back( to_string_dec_uint( recon_lock_nb_match ) );
			messages.push_back( to_string_dec_uint( recon_match_mode ) );
			on_changed( messages );
			nav.pop();
		};
	}
} /* namespace ui */
