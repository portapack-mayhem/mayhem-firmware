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
 // AD V 1.1 12/6/2020

#include "ui_scanner.hpp"

#include "baseband_api.hpp"
#include "string_format.hpp"
#include "audio.hpp"

using namespace portapack;

namespace ui {

ScannerThread::ScannerThread(
	std::vector<rf::Frequency> frequency_list
) : frequency_list_ {  std::move(frequency_list) }
{
	thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, ScannerThread::static_fn, this);
}

ScannerThread::~ScannerThread() {
	if( thread ) {
		chThdTerminate(thread);
		chThdWait(thread);
		thread = nullptr;
	}
}

void ScannerThread::set_scanning(const bool v) {
	_scanning = v;
}

msg_t ScannerThread::static_fn(void* arg) {
	auto obj = static_cast<ScannerThread*>(arg);
	obj->run();
	return 0;
}

void ScannerThread::run() {
	RetuneMessage message { };
	uint32_t frequency_index = 0;
	
	while( !chThdShouldTerminate() ) {
		if (_scanning) {
			// Retune
			receiver_model.set_tuning_frequency(frequency_list_[frequency_index]);
			
			message.range = frequency_index;  
			EventDispatcher::send_message(message);
		
			frequency_index++;
			if (frequency_index >= frequency_list_.size())
				frequency_index = 0;
		}
		
		chThdSleepMilliseconds(50);  
	}
}

void ScannerView_FM::handle_retune(uint32_t i) {
	text_cycle.set(	to_string_dec_uint(i + 1) + "/" +
					to_string_dec_uint(frequency_list.size()) + ": " +
					to_string_dec_uint(frequency_list[i])  );
	desc_cycle.set( description_list[i] );
}

void ScannerView_AM::handle_retune(uint32_t i) {
	text_cycle.set(	to_string_dec_uint(i + 1) + "/" +
					to_string_dec_uint(frequency_list.size()) + ": " +
					to_string_dec_uint(frequency_list[i])  );
	desc_cycle.set( description_list[i] );
}

void ScannerView_NFM::handle_retune(uint32_t i) {
	text_cycle.set(	to_string_dec_uint(i + 1) + "/" +
					to_string_dec_uint(frequency_list.size()) + ": " +
					to_string_dec_uint(frequency_list[i]) );
	desc_cycle.set( description_list[i] );
}

void ScannerView_AM::focus() {
	field_lna.focus();
}

void ScannerView_FM::focus() {
	field_lna.focus();
}

void ScannerView_NFM::focus() {
	field_lna.focus();
}

ScannerView_AM::~ScannerView_AM() {
	audio::output::stop();
	receiver_model.disable();
	baseband::shutdown();
}

ScannerView_FM::~ScannerView_FM() {
	audio::output::stop();
	receiver_model.disable();
	baseband::shutdown();
}

ScannerView_NFM::~ScannerView_NFM() {
	audio::output::stop();
	receiver_model.disable();
	baseband::shutdown();
}

// NFM  same as before

ScannerView_NFM::ScannerView_NFM(
	NavigationView&
)
{
	add_children({
		&rssi,
		&labels,
		&field_lna,
		&field_vga,
		&field_rf_amp,
		&field_volume,
		&field_bw,
		&field_squelch,
		&field_wait,
		//&record_view,
		&text_cycle,
		&desc_cycle,
		&text_cycle1,
		&text_modulation,
		//&waterfall,
	});

	size_t step_nfm = 0;
	size_t	max_entry_nfm = 0 ;

	std::string scanner_file = "SCANNER_NFM";
	if (load_freqman_file(scanner_file, database)) {
		for(auto& entry : database) {
			// FIXME
			if (entry.type == RANGE) {
				
				switch (entry.step) {
				case STEP_DEF:	step_nfm = 12500  	;  	break ;				// default
				case AM_US:		step_nfm = 10000  	;  	break ;
				case AM_EUR:	step_nfm = 9000  	;  	break ;
				case NFM_1: 	step_nfm = 12500  	;  	break ;
				case NFM_2:    	step_nfm = 6250 	;	break ;	
				case FM_1:		step_nfm = 100000  	;  	break ;
				case FM_2:		step_nfm = 50000  	; 	break ;
				case N_1:		step_nfm = 25000  	;  	break ;
				case N_2:		step_nfm = 250000  	;  	break ;
				default:		step_nfm = 12500 	;							//error
						}
				
				for (uint32_t i=entry.frequency_a; i < entry.frequency_b; i+= step_nfm) 	{
					
					max_entry_nfm = max_entry_nfm  + 1 ;

					
				if ( max_entry_nfm >= MAX_DB_ENTRY) { 
					text_modulation.set( "Many entries:R: " + to_string_dec_uint(entry.frequency_a)  );break;}
					frequency_list.push_back(i);
					description_list.push_back("R: " + to_string_dec_uint(entry.frequency_a) + ">" + to_string_dec_uint(entry.frequency_b));
																						}
			} else {
				if (entry.type == SINGLE) {
					max_entry_nfm =max_entry_nfm + 1;
				if ( max_entry_nfm >= MAX_DB_ENTRY) {
					text_modulation.set( "Many entries:S: " + to_string_dec_uint(entry.frequency_a) );break; }
					frequency_list.push_back(entry.frequency_a);}
					description_list.push_back("S: " + entry.description);
				}
		}
	} else {
		// DEBUG
	frequency_list.push_back(44600625);
	description_list.push_back("PMR-446-1");
    frequency_list.push_back(44601875);
	description_list.push_back("PMR-446-2");
    frequency_list.push_back(44603125); 	
	description_list.push_back("PMR-446-3");
    frequency_list.push_back(44604375);
	description_list.push_back("PMR-446-4");
    frequency_list.push_back(44605625); 	
	description_list.push_back("PMR-446-5");
    frequency_list.push_back(44606875); 	
	description_list.push_back("PMR-446-6");
    frequency_list.push_back(44608125); 
	description_list.push_back("PMR-446-7");
    frequency_list.push_back(44609375);	
	description_list.push_back("PMR-446-8");
	 
	
	frequency_list.push_back(466025000);
	description_list.push_back("POCSAG-France-1");
	frequency_list.push_back(466050000);
	description_list.push_back("POCSAG-France-2");
	frequency_list.push_back(466075000);
	description_list.push_back("POCSAG-France-3");
	frequency_list.push_back(466175000);
	description_list.push_back("POCSAG-France-4");
	frequency_list.push_back(466206250);
	description_list.push_back("POCSAG-France-5");
	frequency_list.push_back(466231250);		 
	description_list.push_back("POCSAG-France-6");
	
	text_cycle1.set(" NO SCANNER_NFM.TXT FILE ..." );
	}

	field_bw.set_selected_index(2);
	field_bw.on_change = [this](size_t n, OptionsField::value_t) {
		receiver_model.set_nbfm_configuration(n);
	};

	field_wait.on_change = [this](int32_t v) {
		wait = v;
	};
	field_wait.set_value(5);

	field_squelch.on_change = [this](int32_t v) {
		squelch = v;
	};
	field_squelch.set_value(30);

	field_volume.set_value((receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99);
	field_volume.on_change = [this](int32_t v) {
		this->on_headphone_volume_changed(v);
	};
	
	audio::output::start();
	
	audio::output::mute();
	baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
	receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
	receiver_model.set_sampling_rate(3072000);
	receiver_model.set_baseband_bandwidth(1750000);
	receiver_model.enable();
	receiver_model.set_squelch_level(0);
	receiver_model.set_nbfm_configuration(field_bw.selected_index());
	audio::output::unmute();
	
	// TODO: Scanning thread here
	scan_thread = std::make_unique<ScannerThread>(frequency_list);
}

// FIN NFM

// **********************************************************************************************

//FM

ScannerView_FM::ScannerView_FM(
	NavigationView&
)
{
	add_children({
		&rssi,
		&labels,
		&field_lna,
		&field_vga,
		&field_rf_amp,
		&field_volume,
		&field_bw,
		&field_squelch,
		&field_wait,
		//&record_view,
		&text_cycle,
		&desc_cycle,
		&text_cycle1,
		&text_modulation,

		//&waterfall,
	});
	
	size_t step_fm = 0;
	size_t max_entry_fm = 0;

	std::string scanner_file = "SCANNER_FM";
	if (load_freqman_file(scanner_file, database)) {
		for(auto& entry : database) {
			// FIXME
			if (entry.type == RANGE) {
	
				switch (entry.step) {
				case STEP_DEF:		step_fm = 100000  	;  break ;				// default
				case AM_US:			step_fm = 10000  	;  	break ;
				case AM_EUR:		step_fm = 9000  	;  	break ;
				case NFM_1: 		step_fm = 12500  	;  	break ;
				case NFM_2:    		step_fm = 6250 		;	break ;
				case FM_1: 			step_fm = 100000  	;  break ;
				case FM_2:    		step_fm = 50000 	;	break ;	
				case N_1:			step_fm = 25000  	;  	break ;
				case N_2:			step_fm = 250000  	;  	break ;				
				default:			step_fm = 100000 	;							//error
						}
						
				for (uint32_t i=entry.frequency_a; i < entry.frequency_b; i+= step_fm) {
					max_entry_fm = max_entry_fm  + 1 ; 
				if ( max_entry_fm >= MAX_DB_ENTRY) { 
						text_modulation.set( "Many entries:R: " + to_string_dec_uint(entry.frequency_a) );break;}
					frequency_list.push_back(i);
					description_list.push_back("R: " + to_string_dec_uint(entry.frequency_a) + ">" + to_string_dec_uint(entry.frequency_b));
				}
			} else {
				if (entry.type == SINGLE) {
				max_entry_fm =max_entry_fm + 1;
				if ( max_entry_fm >= MAX_DB_ENTRY) { 
						text_modulation.set( "Many entries:S: " + to_string_dec_uint(entry.frequency_a)  );break;}
				frequency_list.push_back(entry.frequency_a);
				description_list.push_back("S: " + entry.description);
				}
			}
		}
	} else {
		// DEBUG
		frequency_list.push_back(100000000);
		description_list.push_back("-");
		frequency_list.push_back(102400000);
		description_list.push_back("-");
		frequency_list.push_back(107700000);
		description_list.push_back("-");
		frequency_list.push_back(105500000);
		description_list.push_back("-");
		frequency_list.push_back(103200000);
		description_list.push_back("-");
		
		text_cycle1.set(" NO SCANNER_FM.TXT FILE ..." );

	}

	field_bw.set_selected_index(0);
	field_bw.on_change = [this](size_t n, OptionsField::value_t) {
		receiver_model.set_wfm_configuration(n);
	};

	field_wait.on_change = [this](int32_t v) {
		wait = v;
	};
	field_wait.set_value(5);

	field_squelch.on_change = [this](int32_t v) {
		squelch = v;
	};
	field_squelch.set_value(30);

	field_volume.set_value((receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99);
	field_volume.on_change = [this](int32_t v) {
		this->on_headphone_volume_changed(v);
	};
	
	audio::output::start();
	audio::output::mute();
	
	baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);
	receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);

	receiver_model.set_sampling_rate(3072000);
	receiver_model.set_baseband_bandwidth(1750000);
	receiver_model.enable();
	receiver_model.set_squelch_level(0);
	receiver_model.set_wfm_configuration(field_bw.selected_index());
	
	audio::output::unmute();
	
	// TODO: Scanning thread here
	scan_thread = std::make_unique<ScannerThread>(frequency_list);
}

//FIN FM

//******************************************************************************************************

// AM

ScannerView_AM::ScannerView_AM(

	NavigationView&
	
)
{
	add_children({
		&rssi,
		&labels,
		&field_lna,
		&field_vga,
		&field_rf_amp,
		&field_volume,
		&field_bw,
		&field_squelch,
		&field_wait,
		//&record_view,
		&text_cycle,
		&desc_cycle,
		&text_cycle1,
		&text_modulation,
		
		//&waterfall,
	});
	
	size_t step_am = 0;
	
	size_t max_entry_am = 0;

	std::string scanner_file = "SCANNER_AM";
	if (load_freqman_file(scanner_file, database)) {
		for(auto& entry : database) {
			// FIXME
			if (entry.type == RANGE) {

				switch (entry.step) {				
					
				case STEP_DEF:		step_am = 9000  ;  	break ;				// default
				case AM_US: 		step_am = 10000 ;  	break ;
				case AM_EUR:    	step_am = 9000 	;	break ;	
				case NFM_1: 		step_am = 12500  	;  	break ;
				case NFM_2:    		step_am = 6250 		;	break ;
				case FM_1: 			step_am = 100000  	;  break ;
				case FM_2:    		step_am = 50000 	;	break ;	
				case N_1:			step_am = 25000  	;  	break ;
				case N_2:			step_am = 250000  	;  	break ;					
				default:			step_am = 9000 	;							//error
						}						
				
				for (uint32_t i=entry.frequency_a; i < entry.frequency_b; i+= step_am) 
				
				{  
				max_entry_am = max_entry_am  + 1 ;
				if ( max_entry_am >= MAX_DB_ENTRY) { 
				text_modulation.set( "Many entries:R: " + to_string_dec_uint(entry.frequency_a) );break;}
				frequency_list.push_back(i);	
				description_list.push_back("R: " + to_string_dec_uint(entry.frequency_a) + ">" + to_string_dec_uint(entry.frequency_b));				
				}
			} else {
				if (entry.type == SINGLE) {
				max_entry_am =max_entry_am + 1;
				if ( max_entry_am >= MAX_DB_ENTRY) { 
				text_modulation.set( "Many entries:S: " + to_string_dec_uint(entry.frequency_a)  );break;}
				frequency_list.push_back(entry.frequency_a);
				description_list.push_back("S: " + entry.description);
				}
			}
		}
	} else {
		// DEBUG
		frequency_list.push_back(118900000);
		description_list.push_back("-");
		frequency_list.push_back(234000);
		description_list.push_back("RTL");
		frequency_list.push_back(27185000);
		description_list.push_back("-");
		frequency_list.push_back(27155000);
		description_list.push_back("-");
		
		text_cycle1.set(" NO SCANNER_AM.TXT FILE ..." );
	}

	field_bw.set_selected_index(0);    // DSB by default
	field_bw.on_change = [this](size_t n, OptionsField::value_t) {
		receiver_model.set_am_configuration(n);
	};

	field_wait.on_change = [this](int32_t v) {
		wait = v;
	};
	field_wait.set_value(5);

	field_squelch.on_change = [this](int32_t v) {
		squelch = v;
	};
	field_squelch.set_value(30);

	field_volume.set_value((receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99);
	field_volume.on_change = [this](int32_t v) {
		this->on_headphone_volume_changed(v);
	};
	
	audio::output::start();
	
	audio::output::mute();

	baseband::run_image(portapack::spi_flash::image_tag_am_audio);
	receiver_model.set_modulation(ReceiverModel::Mode::AMAudio);
	
	receiver_model.set_sampling_rate(2000000);
	receiver_model.set_baseband_bandwidth(2000000);
	receiver_model.enable();
	receiver_model.set_squelch_level(0);
	receiver_model.set_am_configuration(field_bw.selected_index());
	
	audio::output::unmute();
	
	// TODO: Scanning thread here
	scan_thread = std::make_unique<ScannerThread>(frequency_list);
}

// FIN AM


void ScannerView_AM::on_statistics_update(const ChannelStatistics& statistics) {
	int32_t max_db = statistics.max_db;
	
	if (max_db < -squelch) 	{    // BAD SIGNAL
					//audio::output::stop();
			scan_thread->set_scanning(true);   // WE RESCAN
			timer = 0;
							} 
	else {							// GOOD SIGNAL
		//audio::output::start();
		if ( wait == 99) { timer = 0; }  //we stop scanning
		if ( timer  >= (wait * 10 ) ) 	{    // WAIT in SECOND 10 * 50ms = 1 Ssc
				scan_thread->set_scanning(true);  // WE RESCAN
				timer = 0;
										}
			else
				{
				scan_thread->set_scanning(false); // WE STOP SCANNING
				timer++;
				}
		}
}

void ScannerView_AM::on_headphone_volume_changed(int32_t v) {
	const auto new_volume = volume_t::decibel(v - 99) + audio::headphone::volume_range().max;
	receiver_model.set_headphone_volume(new_volume);
}


void ScannerView_FM::on_statistics_update(const ChannelStatistics& statistics) {
	int32_t max_db = statistics.max_db;
		
	if (max_db < -squelch) 	{    // BAD SIGNAL
					//audio::output::stop();
			scan_thread->set_scanning(true);   // WE RESCAN
			timer = 0;
							} 
	else {							// GOOD SIGNAL
		//audio::output::start();
		if ( wait == 99) { timer = 0; }  //we stop scanning
		if ( timer  >= (wait * 10 ) ) 	{    // WAIT in SECOND 10 * 50ms = 1 Ssc
				scan_thread->set_scanning(true);  // WE RESCAN
				timer = 0;
										}
			else
				{
				scan_thread->set_scanning(false); // WE STOP SCANNING
				timer++;
				}
		}
}

void ScannerView_FM::on_headphone_volume_changed(int32_t v) {
	const auto new_volume = volume_t::decibel(v - 99) + audio::headphone::volume_range().max;
	receiver_model.set_headphone_volume(new_volume);
}

void ScannerView_NFM::on_statistics_update(const ChannelStatistics& statistics) {
	int32_t max_db = statistics.max_db;
	
if (max_db < -squelch) 	{    // BAD SIGNAL
					//audio::output::stop();
			scan_thread->set_scanning(true);   // WE RESCAN
			timer = 0;
							} 
	else {							// GOOD SIGNAL
		//audio::output::start();
		if ( wait == 99) { timer = 0; }  //we stop scanning
		if ( timer  >= (wait * 10 ) ) 	{    // WAIT in SECOND 10 * 50ms = 1 Ssc
				scan_thread->set_scanning(true);  // WE RESCAN
				timer = 0;
										}
			else
				{
				scan_thread->set_scanning(false); // WE STOP SCANNING
				timer++;
				}
		}
}

void ScannerView_NFM::on_headphone_volume_changed(int32_t v) {
	const auto new_volume = volume_t::decibel(v - 99) + audio::headphone::volume_range().max;
	receiver_model.set_headphone_volume(new_volume);
}

} /* namespace ui */
