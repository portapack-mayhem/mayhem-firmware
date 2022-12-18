/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#include "capture_app.hpp"

#include "baseband_api.hpp"

#include "portapack.hpp"
using namespace portapack;

namespace ui {

CaptureAppView::CaptureAppView(NavigationView& nav) {
	baseband::run_image(portapack::spi_flash::image_tag_capture);

	add_children({
		&labels,
		&rssi,
		&channel,
		&field_frequency,
		&field_frequency_step,
		&field_rf_amp,
		&field_lna,
		&field_vga,
		&option_bandwidth,
		&record_view,
		&waterfall,
	});

	// Hack for initialization
	// TODO: This should be included in a more global section so apps dont need to do it 
	receiver_model.set_sampling_rate(3072000);
	receiver_model.set_baseband_bandwidth(1750000);
	//-------------------

	field_frequency.set_value(receiver_model.tuning_frequency());
	field_frequency.set_step(receiver_model.frequency_step());
	field_frequency.on_change = [this](rf::Frequency f) {
		this->on_tuning_frequency_changed(f);
	};
	field_frequency.on_edit = [this, &nav]() {
		// TODO: Provide separate modal method/scheme?
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			this->on_tuning_frequency_changed(f);
			this->field_frequency.set_value(f);
		};
	};
	
	field_frequency_step.set_by_value(receiver_model.frequency_step());
	field_frequency_step.on_change = [this](size_t, OptionsField::value_t v) {
		receiver_model.set_frequency_step(v);
		this->field_frequency.set_step(v);
	};
	
	option_bandwidth.on_change = [this](size_t, uint32_t base_rate) {
		sampling_rate = 8 * base_rate;	// Decimation by 8 done on baseband side
  	    /* base_rate  is used for FFT calculation and display LCD, and also in  recording writing SD Card  rate. */
		/* ex. sampling_rate values, 4Mhz, when recording 500 kHz (BW) and fs 8 Mhz , when selected 1 Mhz BW ...*/ 
	    /* ex. recording 500kHz BW  to .C16 file, base_rate clock 500kHz x2(I,Q) x 2 bytes (int signed) =2MB/sec rate SD Card  */

		waterfall.on_hide();
		record_view.set_sampling_rate(sampling_rate);
		receiver_model.set_sampling_rate(sampling_rate);
       /* Set up  proper anti aliasing BPF bandwith in MAX2837 before ADC sampling according to the new added BW Options . */ 
		
		switch(sampling_rate) {   // we use the var fs (sampling_rate) , to set up BPF aprox < fs_max/2 by Nyquist theorem. 
	  
	  			case 0 ... 2000000:  //  BW Captured range  (0 <= 250kHz max )  fs = 8 x 250 kHz 
					anti_alias_baseband_bandwidth_filter = 1750000;  // Minimum BPF MAX2837 for all those lower BW options.
        	 		break;

				case 4000000 ... 6000000:    // BW capture  range (500k ... 750kHz max )  fs_max = 8 x 750kHz = 6Mhz 
     				//  BW 500k ... 750kHz   ,  ex. 500kHz   (fs = 8*BW =  4Mhz) , BW 600kHz (fs = 4,8Mhz) , BW  750 kHz (fs = 6Mhz)  
					anti_alias_baseband_bandwidth_filter = 2500000;  // in some IC MAX2837 appear 2250000 , but both works similar. 
					break;	

				case 8800000:    // BW capture 1,1Mhz  fs = 8 x 1,1Mhz = 8,8Mhz . (1Mhz showed slightly higher noise background).
					anti_alias_baseband_bandwidth_filter = 3500000;   
        	 		break;

			    case 14000000:   // BW capture 1,75Mhz  , fs = 8 x 1,75Mhz = 14Mhz 
				    // good BPF ,good matching, but LCD making flicker , refresh rate should be < 20 Hz , but reasonable picture   
         			anti_alias_baseband_bandwidth_filter = 5000000; 
        			break;

    			case 16000000:   // BW capture 2Mhz  , fs = 8 x 2Mhz = 16Mhz
                     //  good BPF ,good matching, but LCD making flicker , refresh rate should be < 20 Hz , but reasonable picture   
         			anti_alias_baseband_bandwidth_filter = 6000000; 
        			break;
   
            	case 20000000:   // BW capture 2,5Mhz  , fs= 8 x 2,5 Mhz = 20Mhz
         			//  good BPF ,good matching, but LCD making flicker , refresh rate should be < 20 Hz , but reasonable picture 
					 anti_alias_baseband_bandwidth_filter = 7000000; 
	   	         	break;
		   	
      			default:         // BW capture 2,75Mhz, fs = 8 x 2,75Mhz= 22Mhz max ADC sampling)  and others.  
        			 //  We tested also 9Mhz FPB stightly too much noise floor , better 8Mhz 
					 anti_alias_baseband_bandwidth_filter = 8000000;
		}   
		receiver_model.set_baseband_bandwidth(anti_alias_baseband_bandwidth_filter);
							
		
		waterfall.on_show();
	};
	
	option_bandwidth.set_selected_index(7);		// 500k,  Preselected starting default option 500kHz 
	
	receiver_model.set_modulation(ReceiverModel::Mode::Capture);
	receiver_model.enable();

	record_view.on_error = [&nav](std::string message) {
		nav.display_modal("Error", message);
	};
}

CaptureAppView::~CaptureAppView() {

	// Hack for preventing halting other apps
	// TODO: This should be also part of something global
	receiver_model.set_sampling_rate(3072000);
	receiver_model.set_baseband_bandwidth(1750000);
	receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
	// ----------------------------

	receiver_model.disable();
	baseband::shutdown();
}

void CaptureAppView::on_hide() {
	// TODO: Terrible kludge because widget system doesn't notify Waterfall that
	// it's being shown or hidden.
	waterfall.on_hide();
	View::on_hide();
}

void CaptureAppView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);

	const ui::Rect waterfall_rect { 0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height };
	waterfall.set_parent_rect(waterfall_rect);
}

void CaptureAppView::focus() {
	record_view.focus();
}

void CaptureAppView::on_tuning_frequency_changed(rf::Frequency f) {
	receiver_model.set_tuning_frequency(f);
}

} /* namespace ui */
