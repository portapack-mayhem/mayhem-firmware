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

#include "ui_scanner.hpp"
#include "ui_fileman.hpp"

using namespace portapack;

namespace ui {

ScannerThread::ScannerThread(
	std::vector<rf::Frequency> frequency_list
) : frequency_list_ {  std::move(frequency_list) }
{
	thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, ScannerThread::static_fn, this);
}

ScannerThread::~ScannerThread() {
	stop();
}

void ScannerThread::stop() {
	if( thread ) {
		chThdTerminate(thread);
		chThdWait(thread);
		thread = nullptr;
	}
}

void ScannerThread::set_scanning(const bool v) {
	_scanning = v;
}

bool ScannerThread::is_scanning() {
	return _scanning;
}

void ScannerThread::set_freq_lock(const uint32_t v) {
	_freq_lock = v;
}

uint32_t ScannerThread::is_freq_lock() {
	return _freq_lock;
}

void ScannerThread::set_freq_del(const uint32_t v) {
	_freq_del = v;
}

void ScannerThread::change_scanning_direction() {
	_fwd = !_fwd;
	chThdSleepMilliseconds(300);	//Give some pause after reversing scanning direction

}

msg_t ScannerThread::static_fn(void* arg) {
	auto obj = static_cast<ScannerThread*>(arg);
	obj->run();
	return 0;
}

void ScannerThread::run() {
	if (frequency_list_.size())	{					//IF THERE IS A FREQUENCY LIST ...	
		RetuneMessage message { };
		uint32_t frequency_index = frequency_list_.size();
		bool restart_scan = false;					//Flag whenever scanning is restarting after a pause
		while( !chThdShouldTerminate() ) {
			if (_scanning) {						//Scanning
				if (_freq_lock == 0) {				//normal scanning (not performing freq_lock)
					if (!restart_scan) {			//looping at full speed
						if (_fwd) {					//forward
							frequency_index++;
							if (frequency_index >= frequency_list_.size())
								frequency_index = 0;	

						} else {					//reverse
							if (frequency_index < 1)
								frequency_index = frequency_list_.size();	
							frequency_index--;
						}
						receiver_model.set_tuning_frequency(frequency_list_[frequency_index]);	// Retune
					}
					else
						restart_scan=false;			//Effectively skipping first retuning, giving system time
				} 
				message.range = frequency_index;	//Inform freq (for coloring purposes also!)
				EventDispatcher::send_message(message);
			} 
			else {									//NOT scanning 									
				if (_freq_del != 0) {				//There is a frequency to delete
					for (uint16_t i = 0; i < frequency_list_.size(); i++) {	//Search for the freq to delete
						if (frequency_list_[i] == _freq_del) 
						{							//found: Erase it
							frequency_list_.erase(frequency_list_.begin() + i);
							if (i==0)				//set scan index one place back to compensate
								i=frequency_list_.size();
							else
								i--;
							break;
						}
					}
					_freq_del = 0;					//deleted.
				}
				else {
					restart_scan=true;					//Flag the need for skipping a cycle when restarting scan
				}
			}
			chThdSleepMilliseconds(50);				//Needed to (eventually) stabilize the receiver into new freq
		}
	}
}

void ScannerView::handle_retune(uint32_t i) {
	switch (scan_thread->is_freq_lock())
	{
	case 0:										//NO FREQ LOCK, ONGOING STANDARD SCANNING
		text_cycle.set( to_string_dec_uint(i + 1,3) );
		current_index = i;		//since it is an ongoing scan, this is a new index
		if (description_list[current_index].size() > 0) desc_cycle.set( description_list[current_index] );	//Show new description	
		break;
	case 1:										//STARTING LOCK FREQ
		big_display.set_style(&style_yellow);
		break;
	case MAX_FREQ_LOCK:							//FREQ IS STRONG: GREEN and scanner will pause when on_statistics_update()
		big_display.set_style(&style_green);
		break;
	default:	//freq lock is checking the signal, do not update display
		return;
	}
	big_display.set(frequency_list[current_index]);	//UPDATE the big Freq after 0, 1 or MAX_FREQ_LOCK (at least, for color synching)
}

void ScannerView::focus() {
	field_mode.focus();
}

ScannerView::~ScannerView() {
	audio::output::stop();
	receiver_model.disable();
	baseband::shutdown();
}

void ScannerView::show_max() {		//show total number of freqs to scan
	if (frequency_list.size() == MAX_DB_ENTRY) {
		text_max.set_style(&style_red);
		text_max.set( "/ " + to_string_dec_uint(MAX_DB_ENTRY) + " (DB MAX!)");
	}
	else {
		text_max.set_style(&style_grey);
		text_max.set( "/ " + to_string_dec_uint(frequency_list.size()));
	}
}

ScannerView::ScannerView(
	NavigationView& nav
	) : nav_ { nav } , loaded_file_name { "SCANNER" }
{
	add_children({
		&labels,
		&field_lna,
		&field_vga,
		&field_rf_amp,
		&field_volume,
		&field_bw,
		&field_squelch,
		&field_wait,
		&button_load,
		&rssi,
		&text_cycle,
		&text_max,
		&desc_cycle,
		&big_display,
		&button_manual_start,
		&button_manual_end,
		&field_mode,
		&step_mode,
		&button_manual_scan,
		&button_pause,
		&button_dir,
		&button_audio_app,
		&button_mic_app,
		&button_add,
		&button_remove

	});

	def_step = change_mode(AM);	//Start on AM
	field_mode.set_by_value(AM);	//Reflect the mode into the manual selector

	//HELPER: Pre-setting a manual range, based on stored frequency
	rf::Frequency stored_freq = persistent_memory::tuned_frequency();
	frequency_range.min = stored_freq - 1000000;
	button_manual_start.set_text(to_string_short_freq(frequency_range.min));
	frequency_range.max = stored_freq + 1000000;
	button_manual_end.set_text(to_string_short_freq(frequency_range.max));

	button_load.on_select = [this, &nav](Button&) {
		// load txt files from the FREQMAN folder
		auto open_view = nav.push<FileLoadView>(".TXT");
		open_view->on_changed = [this](std::filesystem::path new_file_path) {

			std::string dir_filter = "FREQMAN/";
			std::string str_file_path = new_file_path.string();

			if (str_file_path.find(dir_filter) != string::npos) { // assert file from the FREQMAN folder
				scan_pause();
				// get the filename without txt extension so we can use load_freqman_file fcn
				std::string str_file_name = new_file_path.stem().string();
				frequency_file_load(str_file_name, true);
			} else {
				nav_.display_modal("LOAD ERROR", "A valid file from\nFREQMAN directory is\nrequired.");
			}
		};
	};

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

	button_pause.on_select = [this](Button&) {
		if ( userpause )
			user_resume();
		else {
			scan_pause();
			button_pause.set_text("RESUME");	//PAUSED, show resume
			userpause=true;
		}
	};

	button_audio_app.on_select = [this](Button&) {
		scan_thread->stop();
		nav_.pop();
		nav_.push<AnalogAudioView>();
	};

	button_mic_app.on_select = [this](Button&) {
		scan_thread->stop();
		nav_.pop();
		nav_.push<MicTXView>();
	};

	button_remove.on_select = [this](Button&) {
		if (frequency_list.size() > current_index) {
			if (scan_thread->is_scanning())			//STOP Scanning if necessary
				scan_thread->set_scanning(false);
			scan_thread->set_freq_del(frequency_list[current_index]);
			description_list.erase(description_list.begin() + current_index);
			frequency_list.erase(frequency_list.begin() + current_index);
			show_max();								//UPDATE new list size on screen
			desc_cycle.set(" ");					//Clean up description (cosmetic detail)
			scan_thread->set_freq_lock(0); 			//Reset the scanner lock
			if ( userpause ) 						//If user-paused, resume
				user_resume();
		}
	};

	button_manual_scan.on_select = [this](Button&) {
		if (!frequency_range.min || !frequency_range.max) {
			nav_.display_modal("Error", "Both START and END freqs\nneed a value");
		} else if (frequency_range.min > frequency_range.max) {
			nav_.display_modal("Error", "END freq\nis lower than START");
		} else {
		audio::output::stop();
		scan_thread->stop();	//STOP SCANNER THREAD
		frequency_list.clear();
		description_list.clear();
		def_step = step_mode.selected_index_value();		//Use def_step from manual selector

		description_list.push_back(
			"M" + to_string_short_freq(frequency_range.min) + ">"
	 		+ to_string_short_freq(frequency_range.max) + "S" 
	 		+ to_string_short_freq(def_step).erase(0,1) //euquiq: lame kludge to reduce spacing in step freq
		);

		rf::Frequency frequency = frequency_range.min;
		while (frequency_list.size() < MAX_DB_ENTRY &&  frequency <= frequency_range.max) { //add manual range				
			frequency_list.push_back(frequency);
			description_list.push_back("");				//If empty, will keep showing the last description
			frequency+=def_step;
		}
		show_max();
		if ( userpause ) 						//If user-paused, resume
			user_resume();
		big_display.set_style(&style_grey);		//Back to grey color
		start_scan_thread(); //RESTART SCANNER THREAD
		}
	};

	field_mode.on_change = [this](size_t, OptionsField::value_t v) {
		receiver_model.disable();
		baseband::shutdown();
		change_mode(v);
		if ( !scan_thread->is_scanning() ) 						//for some motive, audio output gets stopped.
			audio::output::start();								//So if scan was stopped we resume audio
		receiver_model.enable(); 
	};

	button_dir.on_select = [this](Button&) {
		scan_thread->change_scanning_direction();
		if ( userpause ) 						//If user-paused, resume
			user_resume();
		big_display.set_style(&style_grey);		//Back to grey color
	};

	button_add.on_select = [this](Button&) {  //frequency_list[current_index]
		File scanner_file;
		std::string freq_file_path = "FREQMAN/" + loaded_file_name + ".TXT";
		auto result = scanner_file.open(freq_file_path);	//First search if freq is already in txt
		if (!result.is_valid()) {
			std::string frequency_to_add = "f=" 
				+ to_string_dec_uint(frequency_list[current_index] / 1000) 
				+ to_string_dec_uint(frequency_list[current_index] % 1000UL, 3, '0');
			char one_char[1];		//Read it char by char
			std::string line;		//and put read line in here
			bool found=false;
			for (size_t pointer=0; pointer < scanner_file.size();pointer++) {

				scanner_file.seek(pointer);
				scanner_file.read(one_char, 1);
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
				big_display.set(frequency_list[current_index]);		//After showing an error
			}
			else {
				scanner_file.append(freq_file_path); //Second: append if it is not there
				scanner_file.write_line(frequency_to_add + ",d=ADD FQ");
			}
		} else
		{
			nav_.display_modal("Error", "Cannot open " + loaded_file_name + ".TXT\nfor appending freq.");
			big_display.set(frequency_list[current_index]);		//After showing an error
		}
	};

	//PRE-CONFIGURATION:
	field_wait.on_change = [this](int32_t v) {	wait = v;	}; 	field_wait.set_value(5);
	field_squelch.on_change = [this](int32_t v) {	squelch = v;	}; 	field_squelch.set_value(-10);
	field_volume.set_value((receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99);
	field_volume.on_change = [this](int32_t v) { this->on_headphone_volume_changed(v);	};

	// LEARN FREQUENCIES
	std::string scanner_txt = "SCANNER";
	frequency_file_load(scanner_txt);
}

void ScannerView::frequency_file_load(std::string file_name, bool stop_all_before) {

	// stop everything running now if required
	if (stop_all_before) {
		scan_thread->stop();
		frequency_list.clear(); // clear the existing frequency list (expected behavior)
		description_list.clear();
		def_step = step_mode.selected_index_value();		//Use def_step from manual selector
	}

	if ( load_freqman_file(file_name, database)  ) {
		loaded_file_name = file_name; // keep loaded filename in memory
		for(auto& entry : database) {									// READ LINE PER LINE
			if (frequency_list.size() < MAX_DB_ENTRY) {					//We got space!
				if (entry.type == RANGE)  {								//RANGE	
					switch (entry.step) {
						case AM_US:	def_step = 10000;  	break ;
						case AM_EUR:def_step = 9000;  	break ;
						case NFM_1: def_step = 12500;  	break ;
						case NFM_2: def_step = 6250;	break ;	
						case FM_1:	def_step = 100000; 	break ;
						case FM_2:	def_step = 50000; 	break ;
						case N_1:	def_step = 25000;  	break ;
						case N_2:	def_step = 250000; 	break ;
						case AIRBAND:   def_step= 8330;  	break ;
						case ERROR_STEP:
						case STEP_DEF:
						default:
							def_step = step_mode.selected_index_value(); //Use def_step from manual selector
								 			break ;		
					}
					frequency_list.push_back(entry.frequency_a);		//Store starting freq and description
					description_list.push_back("R" + to_string_short_freq(entry.frequency_a)
						+ ">" + to_string_short_freq(entry.frequency_b)
						+ " S" + to_string_short_freq(def_step).erase(0,1) //euquiq: lame kludge to reduce spacing in step freq
						);
					while (frequency_list.size() < MAX_DB_ENTRY && entry.frequency_a <= entry.frequency_b) { //add the rest of the range
						entry.frequency_a+=def_step;
						frequency_list.push_back(entry.frequency_a);
						description_list.push_back("");				//Token (keep showing the last description)
					}
				} else if ( entry.type == SINGLE)  {
					frequency_list.push_back(entry.frequency_a);
					description_list.push_back("S: " + entry.description);
				}
				show_max();
			}
			else
			{
				break; //No more space: Stop reading the txt file !
			}		
		}
	} 
	else 
	{
		loaded_file_name = "SCANNER"; // back to the default frequency file
		desc_cycle.set(" NO " + file_name + ".TXT FILE ..." );
	}
	audio::output::stop();
	step_mode.set_by_value(def_step); //Impose the default step into the manual step selector
	start_scan_thread();
}

void ScannerView::on_statistics_update(const ChannelStatistics& statistics) {
	if ( !userpause ) 									//Scanning not user-paused
	{
		if (timer >= (wait * 10) ) 
		{
			timer = 0;
			scan_resume();
		} 
		else if (!timer) 
		{
			if (statistics.max_db > squelch ) {  		//There is something on the air...(statistics.max_db > -squelch) 
				if (scan_thread->is_freq_lock() >= MAX_FREQ_LOCK) { //checking time reached
					scan_pause();
					timer++;	
				} else {
					scan_thread->set_freq_lock( scan_thread->is_freq_lock() + 1 ); //in lock period, still analyzing the signal
				}
			} else {									//There is NOTHING on the air
				if (scan_thread->is_freq_lock() > 0) {	//But are we already in freq_lock ?
					big_display.set_style(&style_grey);	//Back to grey color
					scan_thread->set_freq_lock(0); 		//Reset the scanner lock, since there is no signal
				}				
			}
		} 
		else 	//Ongoing wait time
		{
				timer++;
		}
	}
}

void ScannerView::scan_pause() {
	if (scan_thread->is_scanning()) {
		scan_thread->set_freq_lock(0); 		//Reset the scanner lock (because user paused, or MAX_FREQ_LOCK reached) for next freq scan	
		scan_thread->set_scanning(false); // WE STOP SCANNING
		audio::output::start();
		on_headphone_volume_changed(field_volume.value()); // quick fix to make sure WM8731S chips don't stay silent after pause
	}
}

void ScannerView::scan_resume() {
	audio::output::stop();
	big_display.set_style(&style_grey);		//Back to grey color
	if (!scan_thread->is_scanning())
		scan_thread->set_scanning(true);   // RESUME!
}

void ScannerView::user_resume() {
	timer = wait * 10;					//Will trigger a scan_resume() on_statistics_update, also advancing to next freq.
	button_pause.set_text("PAUSE");		//Show button for pause
	userpause=false;					//Resume scanning
}

void ScannerView::on_headphone_volume_changed(int32_t v) {
	const auto new_volume = volume_t::decibel(v - 99) + audio::headphone::volume_range().max;
	receiver_model.set_headphone_volume(new_volume);
}

size_t ScannerView::change_mode(uint8_t new_mod) { //Before this, do a scan_thread->stop();  After this do a start_scan_thread()
	using option_t = std::pair<std::string, int32_t>;
	using options_t = std::vector<option_t>;
	options_t bw;
	field_bw.on_change = [this](size_t n, OptionsField::value_t) { 
		(void)n;  //avoid unused warning 
	};

	switch (new_mod) {
	case NFM:	//bw 16k (2) default
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
	case AM:
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
		receiver_model.set_sampling_rate(3072000);	receiver_model.set_baseband_bandwidth(1750000); 
		break;
	case WFM:
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

	return mod_step[new_mod];
}

void ScannerView::start_scan_thread() {
	receiver_model.enable(); 
	receiver_model.set_squelch_level(0);
	scan_thread = std::make_unique<ScannerThread>(frequency_list);
}

} /* namespace ui */
