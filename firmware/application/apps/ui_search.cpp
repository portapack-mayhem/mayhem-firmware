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

#include "ui_search.hpp"
#include "ui_fileman.hpp"
#include "ui_searchsetup.hpp"

using namespace portapack;

namespace ui {

	SearchThread::SearchThread( std::vector<int64_t> frequency_list	) : frequency_list_ {  std::move(frequency_list) } {
		thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, SearchThread::static_fn, this);
	}

	SearchThread::~SearchThread() {
		stop();
	}

	void SearchThread::stop() {
		if( thread ) {
			chThdTerminate(thread);
			chThdWait(thread);
			thread = nullptr;
		}
	}

	void SearchThread::set_continuous(const bool v) {
		_continuous = v;
	}

	void SearchThread::set_searching(const bool v) {
		_searching = v;
	}

	void SearchThread::set_stepper( const int64_t v){
		_stepper = v;
	}

	bool SearchThread::is_searching() {
		return _searching;
	}

	void SearchThread::set_freq_lock(const uint32_t v) {
		_freq_lock = v;
	}
	uint32_t SearchThread::is_freq_lock() {
		return _freq_lock;
	}
	int64_t SearchThread::get_current_freq() {
		return freq ;
	}

	void SearchThread::change_searching_direction() {
		_fwd = !_fwd;
		chThdSleepMilliseconds(300);	//Give some pause after reversing searching direction
	}

	bool SearchThread::get_searching_direction() {
		return _fwd ;
	}
	void SearchThread::set_searching_direction( const bool v) {
		_fwd = v ;
	}

	msg_t SearchThread::static_fn(void* arg) {
		auto obj = static_cast<SearchThread*>(arg);
		obj->run();
		return 0;
	}

	void SearchThread::run() {
		if (frequency_list_.size() )	{					//IF THERE IS A FREQUENCY LIST ...	
			int64_t minfreq = 0 ;
			int64_t maxfreq = 0 ;
			bool has_looped = false ;
			RetuneMessage message { };
			int32_t frequency_index = 0 ;
			bool restart_search = false;					//Flag whenever searching is restarting after a pause

			if( frequency_list_[ 0 ] <  0 ) // first item to search is a range
			{
				freq = -frequency_list_[ 0 ] ;
				minfreq = -frequency_list_[ 0 ] ;
				maxfreq = -frequency_list_[ 1 ] ;
				step = -frequency_list_[ 2 ] ;
			}
			else    //first item to search is a single freq
			{
				freq = frequency_list_[ 0 ] ;
				minfreq = frequency_list_[ 0 ] ;
				maxfreq = frequency_list_[ 0 ] ;
				step = 0 ;
			}

			while( !chThdShouldTerminate() ) {
				has_looped = false ;
				if (_searching || _stepper != 0 ) {					//Searching
					//Inform freq (for coloring purposes also!) 
					message.freq = freq ;
					message.range = frequency_index ;
					EventDispatcher::send_message(message);
					receiver_model.set_tuning_frequency( freq );	// Retune
					if (_freq_lock == 0) {				//normal searching (not performing freq_lock)
						if (!restart_search) {			//looping at full speed
							/* we are doing a range */
							if( frequency_list_[ frequency_index ] <  0 )
							{

								if ( (_fwd&&_stepper==0) || _stepper > 0 ) {	
									//forward
									freq += step ;
									// if bigger than range max
									if (freq > maxfreq ) {
										// when going forward we already know that we can skip a whole range => two values in the list
										frequency_index += 3 ;
										// looping
										if( (uint32_t)frequency_index >= frequency_list_.size() )
										{
											has_looped = true ;
											frequency_index = 0  ;
										}
										if( frequency_list_[ frequency_index ] < 0 )
										{
											freq = -frequency_list_[ frequency_index ];
											minfreq = -frequency_list_[ frequency_index ];
											maxfreq = -frequency_list_[ frequency_index + 1 ];
											step = -frequency_list_[ frequency_index + 2 ];
										}
										else
										{
											freq = frequency_list_[ frequency_index ];
											minfreq = frequency_list_[ frequency_index ];
											maxfreq = frequency_list_[ frequency_index ];
											step = 0 ;
										}
									}
								}
								if( (!_fwd&&_stepper==0) || _stepper < 0 ) {	
									//reverse
									freq -= step ;
									// if lower than range min
									if (freq < minfreq ) {
										// when back we have to check one step at a time
										frequency_index -- ;
										// looping
										if( frequency_index < 0 )
										{
											has_looped = true ;
											frequency_index = frequency_list_.size() - 1 ;
										}
										if( frequency_list_[ frequency_index ] < 0 )
										{
											frequency_index -= 2 ;
											freq = -frequency_list_[ frequency_index + 1];
											minfreq = -frequency_list_[ frequency_index ];
											maxfreq = -frequency_list_[ frequency_index + 1 ];
											step = -frequency_list_[ frequency_index + 2 ];
										}
										else
										{
											freq = frequency_list_[ frequency_index ];
											minfreq = frequency_list_[ frequency_index ];
											maxfreq = frequency_list_[ frequency_index ];
											step = 0 ;
										}
									}
								}
							}
							else
							{
								freq = frequency_list_[ frequency_index ] ; 
								receiver_model.set_tuning_frequency( freq );	// Retune

								if ( (_fwd&&_stepper==0) || _stepper > 0 ) {					//forward
									frequency_index++;
									// looping
									if( (uint32_t)frequency_index >= frequency_list_.size() )
									{
										has_looped = true ;
										frequency_index = 0 ;
									}
									if( frequency_list_[ frequency_index ] < 0 )
									{
										freq = -frequency_list_[ frequency_index ];
										minfreq = -frequency_list_[ frequency_index ];
										maxfreq = -frequency_list_[ frequency_index + 1 ];
										step = -frequency_list_[ frequency_index + 2 ];
									}
									else
									{
										freq = frequency_list_[ frequency_index ];
										minfreq = frequency_list_[ frequency_index ];
										maxfreq = frequency_list_[ frequency_index ];
										step = 0 ;
									}
								}
								if( (!_fwd&&_stepper==0) || _stepper < 0 ) {		
									//reverse
									frequency_index--;
									// if previous if under the list => go back from end
									if( frequency_index < 0 )
									{
										has_looped = true ;
										frequency_index =  frequency_list_.size() - 1 ;
									}
									// if we are now on a negative value we are on a range max. Skipping 2 more to be positionned on range min
									if( frequency_list_[ frequency_index ] < 0 )
									{
										frequency_index -= 2 ;
										freq = -frequency_list_[ frequency_index + 1 ];
										minfreq = -frequency_list_[ frequency_index ];
										maxfreq = -frequency_list_[ frequency_index + 1 ];
										step = -frequency_list_[ frequency_index + 2 ];
									}
									else
									{
										freq = frequency_list_[ frequency_index ];
										step = 0 ;
										minfreq = frequency_list_[ frequency_index ];
										maxfreq = frequency_list_[ frequency_index ];
									}
								}
							}
						}
						else
							restart_search=false;			//Effectively skipping first retuning, giving system time
					} 
					if( has_looped && !_continuous )
					{
						/* prepare values for the next run, when user will resume */
						if( _fwd )
						{
							frequency_index = 0 ;
							if( frequency_list_[ frequency_index ] < 0 )
							{
								freq = -frequency_list_[ frequency_index ];
								minfreq = -frequency_list_[ frequency_index ];
								maxfreq = -frequency_list_[ frequency_index + 1 ];
								step = -frequency_list_[ frequency_index + 2 ];
							}
							else
							{
								freq = frequency_list_[ frequency_index ];
								minfreq = frequency_list_[ frequency_index ];
								maxfreq = frequency_list_[ frequency_index ];
								step = 0 ;
							}
						}
						else
						{
							frequency_index = frequency_list_.size() - 1 ;
							if( frequency_list_[ frequency_index ] < 0 )
							{
								frequency_index -= 2 ;
								freq = -frequency_list_[ frequency_index + 1];
								minfreq = -frequency_list_[ frequency_index ];
								maxfreq = -frequency_list_[ frequency_index + 1 ];
								step = -frequency_list_[ frequency_index + 2 ];
							}
							else
							{
								freq = frequency_list_[ frequency_index ];
								minfreq = frequency_list_[ frequency_index ];
								maxfreq = frequency_list_[ frequency_index ];
								step = 0 ;
							}

						}
						// signal pause to handle_retune 
						receiver_model.set_tuning_frequency( freq );	// Retune
						message.freq = freq ;
						message.range = 9999 ;
						EventDispatcher::send_message(message);
					}
					if( _stepper > 0 ) {
						_stepper -- ;
						if( _stepper == 0 ) {
							//Inform freq (for coloring purposes also!) 
							message.freq = freq ;
							message.range = frequency_index ;
							EventDispatcher::send_message(message);
							receiver_model.set_tuning_frequency( freq );	// Retune

						}
					}

					if( _stepper < 0 ) {
						_stepper ++ ;
						if( _stepper == 0 ) {
							//Inform freq (for coloring purposes also!) 
							message.freq = freq ;
							message.range = frequency_index ;
							EventDispatcher::send_message(message);
							receiver_model.set_tuning_frequency( freq );	// Retune
						}
					} 
				}
				chThdSleepMilliseconds(50);				//Needed to (eventually) stabilize the receiver into new freq
			}
		}
	}

	void SearchView::handle_retune( int64_t freq , uint32_t index ) {
		static int64_t last_freq = 0 ;
		static uint32_t last_index = 999999 ;
		// special non continuous => set userpause when reaching the end of a range 
		if( index == 9999 )
		{
			timer = 10 * wait ;
			search_pause();
			button_pause.set_text("RESTART");		//Show button for non continuous stop
			userpause=true;	
			return ;
		}
		current_index = index ;
		if( current_index != last_index )
		{
			if( frequency_list[ current_index ] < 0 )
			{
				last_index = current_index ;
				if( update_ranges )
				{
					button_manual_start.set_text( to_string_short_freq( (-frequency_list[ current_index ] ) ) );
					frequency_range.min = (-frequency_list[ current_index ] );
					button_manual_end.set_text( to_string_short_freq( ( -frequency_list[ current_index + 1 ] ) ) );
					frequency_range.max = (-frequency_list[ current_index + 1 ] );
				}
			}
		}
		switch (search_thread->is_freq_lock())
		{
			case 0:						//NO FREQ LOCK, ONGOING STANDARD SCANNING
				text_cycle.set( to_string_dec_uint( current_index + 1 , 3 ) );
				if(description_list[current_index].size() > 0) desc_cycle.set( description_list[current_index] );	//Show new description	
				break;
			case 1:						//STARTING LOCK FREQ
				big_display.set_style(&style_yellow);
				break;
			case MAX_FREQ_LOCK:				//FREQ IS STRONG: GREEN and search will pause when on_statistics_update()
				big_display.set_style(&style_green);
				if( autosave && last_freq != freq ) {
					File search_file;
					std::string freq_file_path = "/FREQMAN/"+output_file+".TXT" ;
					auto result = search_file.open(freq_file_path);	//First search if freq is already in txt
					std::string frequency_to_add = "f=" 
						+ to_string_dec_uint( freq / 1000) 
						+ to_string_dec_uint( freq % 1000UL, 3, '0');
					if (!result.is_valid()) {
						char one_char[1];		//Read it char by char
						std::string line;		//and put read line in here
						bool found=false;
						for (size_t pointer=0; pointer < search_file.size();pointer++) {

							search_file.seek(pointer);
							search_file.read(one_char, 1);
							if ((int)one_char[0] > 31) {			//ascii space upwards
								line += one_char[0];				//Add it to the textline
							}
							else if (one_char[0] == '\n') {			//New Line
								if (line.compare(0, frequency_to_add.size(),frequency_to_add) == 0) {
									found=true;
									break;
								}
								line.clear();						//Ready for next textline
							}
						}
						if( !found) {
							result = search_file.append(freq_file_path); //Second: append if it is not there
							if( !result.is_valid() )
							{
								// add current description if it available 
								if( description_list[current_index].size() > 0)
								{
									search_file.write_line(frequency_to_add + ",d=" + description_list[current_index] );
								}
								else
								{
									search_file.write_line(frequency_to_add + ",d=ADD FQ");
								}
							}
						}
					}
					else {
						result = search_file.create( freq_file_path );	//First freq if no output file
						if( !result.is_valid() )
						{
							// add current description if it available 
							if( description_list[current_index].size() > 0)
							{
								search_file.write_line(frequency_to_add + ",d=" + description_list[current_index] );
							}
							else
							{
								search_file.write_line(frequency_to_add + ",d=ADD FQ");
							}
						}
					}
				}
				last_freq = freq ;
				break;
			default:	//freq lock is checking the signal, do not update display
				return;
		}
		big_display.set( freq );	//UPDATE the big Freq after 0, 1 or MAX_FREQ_LOCK (at least, for color synching)
	}

	void SearchView::focus() {
		field_mode.focus();
	}

	SearchView::~SearchView() {
		audio::output::stop();
		receiver_model.disable();
		baseband::shutdown();
	}

	void SearchView::show_max() {		//show total number of freqs to search
		text_max.set_style(&style_white);
		text_max.set( "/ " + to_string_dec_uint(frequency_list.size()));
	}

	SearchView::SearchView( NavigationView& nav) : nav_ { nav } {

		add_children( {
				&labels,
				&field_lna,
				&field_vga,
				&field_rf_amp,
				&field_volume,
				&field_bw,
				&field_squelch,
				&field_wait,
				&rssi,
				&text_cycle,
				&text_max,
				&desc_cycle,
				&big_display,
				&button_search_setup,
				&button_manual_start,
				&button_manual_end,
				&button_manual_search,
				&field_mode,
				&step_mode,
				&button_pause,
				&button_audio_app,
				&button_add,
				&button_dir,
				&button_restart,
				&button_mic_app,
				&button_remove
		} );

		def_step = change_mode(SEARCH_AM);	//Start on AM
		field_mode.set_by_value(SEARCH_AM);	//Reflect the mode into the manual selector

		//HELPER: Pre-setting a manual range, based on stored frequency
		rf::Frequency stored_freq = persistent_memory::tuned_frequency();
		frequency_range.min = stored_freq - 1000000;
		button_manual_start.set_text(to_string_short_freq(frequency_range.min));
		frequency_range.max = stored_freq + 1000000;
		button_manual_end.set_text(to_string_short_freq(frequency_range.max));

		// Loading settings
		autostart = persistent_memory::search_autostart_search();
		autosave = persistent_memory::search_autosave_freqs();
		continuous = persistent_memory::search_continuous();
		filedelete = persistent_memory::search_clear_output();
		load_freqs = persistent_memory::search_load_freqs();
		load_ranges = persistent_memory::search_load_ranges();
		update_ranges = persistent_memory::search_update_ranges_when_searching();

		//Loading input and output file from settings
		SearchSetupLoadStrings( "SEARCH/SEARCH.CFG" , input_file , output_file );

		button_manual_start.on_select = [this, &nav](Button& button) {
			auto new_view = nav_.push<FrequencyKeypadView>(frequency_range.min);
			new_view->on_changed = [this, &button](rf::Frequency f) {
				frequency_range.min = f;
				button_manual_start.set_text(to_string_short_freq(f));
			};
		};

		button_manual_end.on_select = [this, &nav](Button& button) {
			auto new_view = nav.push<FrequencyKeypadView>(frequency_range.max);
			new_view->on_changed = [this, &button](rf::Frequency f) {
				frequency_range.max = f;
				button_manual_end.set_text(to_string_short_freq(f));
			};
		};

		button_pause.on_select = [this](ButtonWithEncoder&) {
			if( search_thread && frequency_list.size() > 0 )
			{
				if( userpause )
					user_resume();
				else {
					userpause=true;
					search_pause();
					button_pause.set_text("<RESUME>");	//PAUSED, show resume
					search_thread->set_freq_lock(0); 		//Reset the search lock, since it's a new signal
				}
			}
		};
		button_pause.on_change = [this]() {
			if( search_thread && frequency_list.size() > 0 )
			{
				if( userpause )
				{
					big_display.set_style(&style_white);		//Back to white color
					search_thread-> set_stepper( button_pause.get_encoder_delta() );
					button_pause.set_encoder_delta( 0 );
				}
			}
		}; 

		button_audio_app.on_select = [this](Button&) {
			search_thread->stop();
			nav_.pop();
			nav_.push<AnalogAudioView>();
		};

		button_mic_app.on_select = [this](Button&) {
			search_thread->stop();
			nav_.pop();
			nav_.push<MicTXView>();
		};

		button_remove.on_select = [this](Button&) {
			if( search_thread && search_thread->get_current_freq() > 0 )
			{
				File search_file;
				File tmpsearch_file;
				std::string freq_file_path = "FREQMAN/" + output_file + ".TXT";
				std::string tmp_freq_file_path = "FREQMAN/" + output_file + ".TXT.TMP";
				auto result = search_file.open( freq_file_path );	//First search if freq is already in txt
				if (!result.is_valid()) {
					std::string frequency_to_remove = "f=" 
						+ to_string_dec_uint(frequency_list[current_index] / 1000) 
						+ to_string_dec_uint(frequency_list[current_index] % 1000UL, 3, '0');
					char one_char[1];		//Read it char by char
					std::string line;		//and put read line in here
					bool found=false;
					for (size_t pointer=0; pointer < search_file.size();pointer++) {
						search_file.seek(pointer);
						search_file.read(one_char, 1);
						if ((int)one_char[0] > 31) {			//ascii space upwards
							line += one_char[0];				//Add it to the textline
						}
						else if (one_char[0] == '\n') {			//New Line
							if (line.compare(0, frequency_to_remove.size(),frequency_to_remove) == 0) {
								found=true;
								break;
							}
							line.clear();						//Ready for next textline
						}
					}
					if( found )
					{
						/* reread and write tmp file */
						delete_file( tmp_freq_file_path );
						result = tmpsearch_file.open( tmp_freq_file_path );
						if ( !result.is_valid() ) {
							/* re read file and write tmp out */
							for (size_t pointer=0; pointer < search_file.size();pointer++) {
								search_file.seek(pointer);
								search_file.read(one_char, 1);
								if ((int)one_char[0] > 31) {			//ascii space upwards
									line += one_char[0];				//Add it to the textline
								}
								else if (one_char[0] == '\n') {			//New Line
									if( line.compare(0, frequency_to_remove.size(),frequency_to_remove) != 0 ) {
										result = tmpsearch_file.write_line( line );
									}
									line.clear();					
								}
							}
							delete &search_file;
							delete &tmpsearch_file ;
							delete_file( freq_file_path );
							rename_file( tmp_freq_file_path , freq_file_path );
						}
					}
				}
			}
		};

		button_manual_search.on_select = [this](Button&) {
			if (!frequency_range.min || !frequency_range.max) {
				nav_.display_modal("Error", "Both START and END freqs\nneed a value");
			} else if (frequency_range.min > frequency_range.max) {
				nav_.display_modal("Error", "END freq\nis lower than START");
			} else {
				audio::output::stop();
				search_thread->stop();	//STOP SCANNER THREAD

				frequency_list.clear();
				description_list.clear();

				def_step = step_mode.selected_index_value();     //Use def_step from manual selector

				description_list.push_back(
						"R " + to_string_short_freq(frequency_range.min) + ">"
						+ to_string_short_freq(frequency_range.max) + " S" 	// current Manual range
						+ to_string_short_freq(def_step).erase(0,1) //euquiq: lame kludge to reduce spacing in step freq
						);
				frequency_list.push_back( -frequency_range.min ); // min range val
				description_list.push_back( "!range max val" );
				frequency_list.push_back( -frequency_range.max ); // max range val
				description_list.push_back( "!range step val" );
				frequency_list.push_back( -def_step ); 		  // custom range step

				show_max(); /* display step information */
				text_cycle.set( " " );
				timer = 10 * wait ;
				search_thread->set_freq_lock(0); 		//Reset the search lock

				if ( userpause ) 						//If user-paused, resume
					user_resume();
				big_display.set_style(&style_white);		//Back to white color
				start_search_thread(); //RESTART SCANNER THREAD
				if (!search_thread->is_searching())
					search_thread->set_searching(true);   // RESUME!
			}
		};

		field_mode.on_change = [this](size_t, OptionsField::value_t v) {
			receiver_model.disable();
			baseband::shutdown();
			change_mode(v);
			if ( !search_thread->is_searching() ) 						//for some motive, audio output gets stopped.
				audio::output::start();								//So if search was stopped we resume audio
			receiver_model.enable(); 
		};

		button_dir.on_select = [this](Button&) {
			search_thread->change_searching_direction();
			fwd = search_thread->get_searching_direction();
			if( fwd )
			{
				button_dir.set_text( "FW>" );
			}
			else
			{
				button_dir.set_text( "<RW" );
			}
			if ( userpause )				//If user-paused, resume
				user_resume();
			timer = 10 * wait ;
			search_resume();
			search_thread->set_freq_lock(0); 		//Reset the search lock
			big_display.set_style(&style_white);		//Back to white color
		};

		button_restart.on_select = [this](ImageButton&) {
			if( frequency_list.size() > 0 )
			{
				def_step = step_mode.selected_index_value();     //Use def_step from manual selector
				frequency_file_load( input_file , true );
				if( fwd )
				{
					button_dir.set_text( "FW>" );
				}
				else
				{
					button_dir.set_text( "<RW" );
				}
				timer = wait * 10;	 		//Will trigger a search_resume() on_statistics_update, also advancing to next freq.
				button_pause.set_text("PAUSE");		//Show button for pause
				userpause=false;	
			}
		};


		button_add.on_select = [this](Button&) {  //frequency_list[current_index]
			if( search_thread && search_thread->get_current_freq() > 0 )
			{
				File search_file;
				std::string freq_file_path = "FREQMAN/" + output_file + ".TXT";
				std::string frequency_to_add = "f=" 
					+ to_string_dec_uint(search_thread->get_current_freq() / 1000) 
					+ to_string_dec_uint(search_thread->get_current_freq() % 1000UL, 3, '0');
				auto result = search_file.open(freq_file_path);	//First search if freq is already in txt
				if (!result.is_valid()) {
					char one_char[1];		//Read it char by char
					std::string line;		//and put read line in here
					bool found=false;
					for (size_t pointer=0; pointer < search_file.size();pointer++) {
						search_file.seek(pointer);
						search_file.read(one_char, 1);
						if ((int)one_char[0] > 31) {			//ascii space upwards
							line += one_char[0];				//Add it to the textline
						}
						else if (one_char[0] == '\n') {			//New Line
							if (line.compare(0, frequency_to_add.size(),frequency_to_add) == 0) {
								found=true;
								break;
							}
							line.clear();						//Ready for next textline
						}
					}
					if (found) {
						nav_.display_modal("Error", "Frequency already exists");
						big_display.set( search_thread->get_current_freq() );		//After showing an error
					}
					else {
						result = search_file.append(freq_file_path); //Second: append if it is not there
						if( !result.is_valid() )
						{
							// add current description if it available 
							if( description_list[current_index].size() > 0)
							{
								search_file.write_line(frequency_to_add + ",d=" + description_list[current_index] );
							}
							else
							{
								search_file.write_line(frequency_to_add + ",d=ADD FQ");
							}
						}
					}
				}
				else
				{
					result = search_file.create(freq_file_path); //third: create if it is not there
					if( !result.is_valid() )
					{
						// add current description if it available 
						if( description_list[current_index].size() > 0)
						{
							search_file.write_line(frequency_to_add + ",d=" + description_list[current_index] );
						}
						else
						{
							search_file.write_line(frequency_to_add + ",d=ADD FQ");
						}
					}
				}
			}
		};
		button_search_setup.on_select = [this,&nav](Button&) {
			auto open_view = nav.push<SearchSetupView>(); 
			open_view -> on_changed = [this](std::vector<std::string> result) {
				input_file = result[0];
				output_file = result[1];
				autosave = persistent_memory::search_autosave_freqs();
				autostart = persistent_memory::search_autostart_search();
				continuous = persistent_memory::search_continuous();
				filedelete = persistent_memory::search_clear_output();
				load_freqs = persistent_memory::search_load_freqs();
				load_ranges = persistent_memory::search_load_ranges();
				update_ranges = persistent_memory::search_update_ranges_when_searching();
				frequency_file_load( input_file , true );
				//  User experience will tell how they need the output file to be cleared 
				// if( !filedelete )
				// {
				// delete_file( "FREQMAN/"+output_file+".TXT" );
				// }
				if( autostart )
				{
					timer = wait * 10;	 		//Will trigger a search_resume() on_statistics_update, also advancing to next freq.
					button_pause.set_text("PAUSE");		//Show button for pause
					userpause=false;	
					search_resume();
				}
				else
				{
					search_pause();
					button_pause.set_text("<RESUME>");		//Show button for pause
					userpause=true;	
				}
			};
		};

		//PRE-CONFIGURATION:
		field_wait.on_change = [this](int32_t v) {	wait = v;	}; 	field_wait.set_value(5);
		field_squelch.on_change = [this](int32_t v) {	squelch = v;	}; 	field_squelch.set_value(-10);
		field_volume.set_value((receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99);
		field_volume.on_change = [this](int32_t v) { this->on_headphone_volume_changed(v);	};

		if( !filedelete )
		{
			delete_file( "FREQMAN/"+output_file+".TXT" );
		}

		frequency_file_load( input_file );
		if( search_thread && frequency_list.size() > 0 )
		{
			if( autostart )
			{
				timer = wait * 10;	 		//Will trigger a search_resume() on_statistics_update, also advancing to next freq.
				button_pause.set_text("PAUSE");		//Show button for pause
				userpause=false;	
				search_resume();
			}
			else
			{
				userpause=true;
				search_pause();
				button_pause.set_text("<RESUME>");	//PAUSED, show resume
				search_thread->set_freq_lock(0); 		//Reset the search lock, since there is no signal
			}
		}	
	}


	void SearchView::frequency_file_load(std::string file_name, bool stop_all_before) {

		// stop everything running now if required
		if (stop_all_before) {
			search_thread->stop();
		}

		def_step = step_mode.selected_index_value();		//Use def_step from manual selector
		frequency_list.clear();                                 // clear the existing frequency list (expected behavior)
		description_list.clear();

		if ( load_freqman_file(file_name, database)  ) {
			for(auto& entry : database) {								// READ LINE PER LINE
				if (frequency_list.size() < MAX_DB_ENTRY) {					//We got space!
					if( entry.type == RANGE && load_ranges )  {						//RANGE	
						switch( entry.step ) {
							case AM_US:	def_step = 10000;  	break ;
							case AM_EUR:	def_step = 9000;  	break ;
							case NFM_1: 	def_step = 12500;  	break ;
							case NFM_2: 	def_step = 6250;	break ;	
							case FM_1:	def_step = 100000; 	break ;
							case FM_2:	def_step = 50000; 	break ;
							case N_1:	def_step = 25000;  	break ;
							case N_2:	def_step = 250000; 	break ;
							case AIRBAND:	def_step= 8330;  	break ;
						}
						frequency_list.push_back(-entry.frequency_a);		//Store starting freq and description
						description_list.push_back( entry.description );

						frequency_list.push_back(-entry.frequency_b);		//Store ending freq and description
						description_list.push_back( "" );

						frequency_list.push_back(-def_step);			//Store step 
						description_list.push_back( "" );

					} else if ( entry.type == SINGLE && load_freqs )  {
						frequency_list.push_back(entry.frequency_a);
						description_list.push_back("S: " + entry.description);
					}
				}
				else
				{
					break; //No more space: Stop reading the txt file !
				}		
			}
			show_max();
		} 
		else 
		{
			desc_cycle.set(" NO " + file_name + ".TXT FILE ..." );
		} 
		audio::output::stop();
		step_mode.set_by_value(def_step); //Impose the default step into the manual step selector
		start_search_thread(); 
	}

	void SearchView::on_statistics_update(const ChannelStatistics& statistics) {
		if ( !userpause ) 									//Searching not user-paused
		{
			if (timer >= (wait * 10) ) 
			{
				timer = 0;
				search_resume();
			} 
			else if (!timer) 
			{
				if (statistics.max_db > squelch ) {  		//There is something on the air...(statistics.max_db > -squelch) 
					if (search_thread->is_freq_lock() >= MAX_FREQ_LOCK) { //checking time reached
						search_pause();
						timer++;	
					} else {
						search_thread->set_freq_lock( search_thread->is_freq_lock() + 1 ); //in lock period, still analyzing the signal
					}
				} else {									//There is NOTHING on the air
					if (search_thread->is_freq_lock() > 0) {	//But are we already in freq_lock ?
						big_display.set_style(&style_white);	//Back to white color
						search_thread->set_freq_lock(0); 		//Reset the search lock, since there is no signal
					}				
				}
			} 
			else 	//Ongoing wait time
			{
				timer++;
			}
		}
	}

	void SearchView::search_pause() {
		if (search_thread ->is_searching()) {
			search_thread ->set_freq_lock(0); 		//Reset the search lock (because user paused, or MAX_FREQ_LOCK reached) for next freq search
			search_thread ->set_searching(false); // WE STOP SCANNING
			audio::output::start();
		}
	}


	void SearchView::search_resume() {
		audio::output::stop();
		big_display.set_style(&style_white);		//Back to white color
		if (!search_thread->is_searching())
			search_thread->set_searching(true);   // RESUME!
	}

	void SearchView::user_resume() {
		timer = wait * 10;			//Will trigger a search_resume() on_statistics_update, also advancing to next freq.
		button_pause.set_text("PAUSE");		//Show button for pause
		userpause=false;			//Resume searching
	}

	void SearchView::on_headphone_volume_changed(int32_t v) {
		const auto new_volume = volume_t::decibel(v - 99) + audio::headphone::volume_range().max;
		receiver_model.set_headphone_volume(new_volume);
	}

	size_t SearchView::change_mode(uint8_t new_mod) { //Before this, do a search_thread->stop();  After this do a start_search_thread()
		using option_t = std::pair<std::string, int32_t>;
		using options_t = std::vector<option_t>;
		options_t bw;
		field_bw.on_change = [this](size_t n, OptionsField::value_t) {	};

		switch (new_mod) {
			case SEARCH_NFM:	//bw 16k (2) default
				bw.emplace_back("8k5", 0);
				bw.emplace_back("11k", 0);
				bw.emplace_back("16k", 0);			
				field_bw.set_options(bw);

				baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
				receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
				field_bw.set_selected_index(2);
				receiver_model.set_nbfm_configuration(field_bw.selected_index());
				field_bw.on_change = [this](size_t n, OptionsField::value_t) { 	receiver_model.set_nbfm_configuration(n); };
				receiver_model.set_sampling_rate(3072000);	receiver_model.set_baseband_bandwidth(1750000);	
				break;
			case SEARCH_AM:
				bw.emplace_back("DSB", 0);
				bw.emplace_back("USB", 0);
				bw.emplace_back("LSB", 0);
				bw.emplace_back("CW ", 0);
				field_bw.set_options(bw);

				baseband::run_image(portapack::spi_flash::image_tag_am_audio);
				receiver_model.set_modulation(ReceiverModel::Mode::AMAudio);
				field_bw.set_selected_index(0);
				receiver_model.set_am_configuration(field_bw.selected_index());
				field_bw.on_change = [this](size_t n, OptionsField::value_t) { receiver_model.set_am_configuration(n);	};		
				receiver_model.set_sampling_rate(2000000);receiver_model.set_baseband_bandwidth(2000000); 
				break;
			case SEARCH_WFM:
				bw.emplace_back("16k", 0);
				field_bw.set_options(bw);

				baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);
				receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
				field_bw.set_selected_index(0);
				receiver_model.set_wfm_configuration(field_bw.selected_index());
				field_bw.on_change = [this](size_t n, OptionsField::value_t) {	receiver_model.set_wfm_configuration(n); };
				receiver_model.set_sampling_rate(3072000);	receiver_model.set_baseband_bandwidth(2000000);	
				break;
		}

		return search_mod_step[new_mod];
	}

	void SearchView::start_search_thread() {
		receiver_model.enable(); 
		receiver_model.set_squelch_level(0);
		search_thread = std::make_unique<SearchThread>(frequency_list);
		search_thread->set_continuous( continuous );
		search_thread->set_searching_direction( fwd );
	}


} /* namespace ui */
