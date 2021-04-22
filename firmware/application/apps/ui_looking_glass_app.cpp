/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2020 euquiq
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

#include "ui_looking_glass_app.hpp"


using namespace portapack;

namespace ui
{

void GlassView::focus() {
	field_marker.focus();
}

GlassView::~GlassView() {
    receiver_model.set_sampling_rate(3072000); // Just a hack to avoid hanging other apps
	receiver_model.disable();
	baseband::shutdown();
}

void GlassView::on_lna_changed(int32_t v_db) {
	receiver_model.set_lna(v_db);
}

void GlassView::on_vga_changed(int32_t v_db) {
	receiver_model.set_vga(v_db);
}

void GlassView::add_spectrum_pixel(Color color)
{
    spectrum_row[pixel_index++] = color;
    if (pixel_index == 240) //got an entire waterfall line
    { 
        const auto draw_y = display.scroll(1); //Scroll 1 pixel down
        display.draw_pixels( {{0, draw_y}, {240, 1}}, spectrum_row); //new line at top
        pixel_index = 0; //Start New cascade line
    }
}

//Apparently, the spectrum object returns an array of 256 bins
//Each having the radio signal power for it's corresponding frequency slot
void GlassView::on_channel_spectrum(const ChannelSpectrum &spectrum)
{
    baseband::spectrum_streaming_stop();

    // Convert bins of this spectrum slice into a representative max_power and when enough, into pixels
    // Spectrum.db has 256 bins. Center 12 bins are ignored (DC spike is blanked) Leftmost and rightmost 2 bins are ignored
    // All things said and done, we actually need 240 of those bins:
    for(uint8_t bin = 0; bin < 240; bin++) 
    {
        if (bin < 120) {
            if (spectrum.db[134 + bin] > max_power)
                max_power = spectrum.db[134 + bin];
        }         
        else
        {
            if (spectrum.db[bin - 118] > max_power)
                    max_power = spectrum.db[bin - 118];  
        }

        bins_Hz_size += each_bin_size;   //add this bin Hz count into the "pixel fulfilled bag of Hz"

        if (bins_Hz_size >= marker_pixel_step) //new pixel fullfilled
        {
            if (min_color_power < max_power)
                add_spectrum_pixel(spectrum_rgb3_lut[max_power]); //Pixel will represent max_power
            else
                add_spectrum_pixel(0);  //Filtered out, show black

            max_power = 0;

            if (!pixel_index) //Received indication that a waterfall line has been completed
            {
                bins_Hz_size = 0;  //Since this is an entire pixel line, we don't carry "Pixels into next bin"
                break;
            } else {
                bins_Hz_size -= marker_pixel_step; //reset bins size, but carrying the eventual excess Hz into next pixel
            }
        }
    }

    if (pixel_index) 
        f_center += LOOKING_GLASS_SLICE_WIDTH; //Move into the next bandwidth slice NOTE: spectrum.sampling_rate = LOOKING_GLASS_SLICE_WIDTH
    else
        f_center = f_center_ini; //Start a new sweep

    receiver_model.set_tuning_frequency(f_center); //tune rx for this slice
    baseband::spectrum_streaming_start();          //Do the RX
}

void GlassView::on_hide()
{
    baseband::spectrum_streaming_stop();
    display.scroll_disable();
}

void GlassView::on_show()
{
    display.scroll_set_area( 109, 319); //Restart scroll on the correct coordinates
    baseband::spectrum_streaming_start();
}

void GlassView::on_range_changed()
{
    f_min = field_frequency_min.value();
    f_max = field_frequency_max.value();
    search_span = f_max - f_min;

    field_marker.set_range(f_min, f_max);              //Move the marker between range
    field_marker.set_value(f_min + (search_span / 2)); //Put MARKER AT MIDDLE RANGE
    text_range.set(to_string_dec_uint(search_span));

    f_min = (f_min)*MHZ_DIV; //Transpose into full frequency realm
    f_max = (f_max)*MHZ_DIV;
    search_span = search_span * MHZ_DIV;

    marker_pixel_step = search_span / 240;                                        //Each pixel value in Hz
    text_marker_pm.set(to_string_dec_uint((marker_pixel_step / X2_MHZ_DIV) + 1)); // Give idea of +/- marker precision

    int32_t marker_step = marker_pixel_step / MHZ_DIV;
    if (!marker_step) 
        field_marker.set_step(1); //in case selected range is less than 240 (pixels)
    else
        field_marker.set_step(marker_step); //step needs to be a pixel wide.

    f_center_ini = f_min + (LOOKING_GLASS_SLICE_WIDTH / 2);    //Initial center frequency for sweep
    f_center_ini += LOOKING_GLASS_SLICE_WIDTH;                 //euquiq: Why do I need to move the center ???!!! (shift needed for marker accuracy)

    PlotMarker(field_marker.value()); //Refresh marker on screen

    f_center = f_center_ini;                        //Reset sweep into first slice
    pixel_index = 0;                                //reset pixel counter
    max_power = 0;
    bins_Hz_size = 0;                               //reset amount of Hz filled up by pixels
    
    baseband::set_spectrum(LOOKING_GLASS_SLICE_WIDTH, field_trigger.value());   
    receiver_model.set_tuning_frequency(f_center_ini); //tune rx for this slice
}

void GlassView::PlotMarker(rf::Frequency fpos)
{
    int64_t pos = fpos * MHZ_DIV;
    pos -= f_min;
    pos = pos / marker_pixel_step; //Real pixel 

    portapack::display.fill_rectangle({0, 100, 240, 8}, Color::black()); //Clear old marker and whole marker rectangle btw
    portapack::display.fill_rectangle({(int16_t)pos - 2, 100, 5, 3}, Color::red()); //Red marker middle
    portapack::display.fill_rectangle({(int16_t)pos - 1, 103, 3, 3}, Color::red()); //Red marker middle
    portapack::display.fill_rectangle({(int16_t)pos, 106, 1, 2}, Color::red()); //Red marker middle
}

GlassView::GlassView(
    NavigationView &nav) : nav_(nav)
{
    baseband::run_image(portapack::spi_flash::image_tag_wideband_spectrum);

    add_children({&labels,
                  &field_frequency_min,
                  &field_frequency_max,
                  &field_lna,
                  &field_vga,
                  &text_range,
                  &filter_config,
                  &field_rf_amp,
                  &range_presets,
                  &field_marker,
                  &text_marker_pm,
                  &field_trigger
                });

    load_Presets(); //Load available presets from TXT files (or default)
    
    field_frequency_min.set_value(presets_db[0].min);   //Defaults to first preset
    field_frequency_min.on_change = [this](int32_t v) {
        if (v >= field_frequency_max.value())
            field_frequency_max.set_value(v + 240);
        this->on_range_changed();
    };

    field_frequency_max.set_value(presets_db[0].max);   //Defaults to first preset
    field_frequency_max.on_change = [this](int32_t v) {
        if (v <= field_frequency_min.value())
            field_frequency_min.set_value(v - 240);
        this->on_range_changed();
    };

    field_lna.set_value(receiver_model.lna());
    field_lna.on_change = [this](int32_t v) {
        this->on_lna_changed(v);
    };

    field_vga.set_value(receiver_model.vga());
    field_vga.on_change = [this](int32_t v_db) {
        this->on_vga_changed(v_db);
    };

    filter_config.set_selected_index(0);
	filter_config.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		min_color_power = v;
	};

	range_presets.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		field_frequency_min.set_value(presets_db[v].min,false);
        field_frequency_max.set_value(presets_db[v].max,false);
        this->on_range_changed();
	};

    field_marker.on_change = [this](int32_t v) {
        PlotMarker(v); //Refresh marker on screen
    };

    field_marker.on_select = [this](NumberField&) {
        f_center = field_marker.value();
        f_center = f_center * MHZ_DIV;
        receiver_model.set_tuning_frequency(f_center); //Center tune rx in marker freq.
        receiver_model.set_frequency_step(MHZ_DIV); // Preset a 1 MHz frequency step into RX -> AUDIO
        nav_.pop();
		nav_.push<AnalogAudioView>(); //Jump into audio view
    };

    field_trigger.set_value(32);   //Defaults to 32, as normal triggering resolution
    field_trigger.on_change = [this](int32_t v) {
        baseband::set_spectrum(LOOKING_GLASS_SLICE_WIDTH, v);
    };

    display.scroll_set_area( 109, 319);
    baseband::set_spectrum(LOOKING_GLASS_SLICE_WIDTH, field_trigger.value());	//trigger:
    // Discord User jteich:  WidebandSpectrum::on_message to set the trigger value. In WidebandSpectrum::execute ,
    // it keeps adding the output of the fft to the buffer until "trigger" number of calls are made, 
    //at which time it pushes the buffer up with channel_spectrum.feed

    on_range_changed();

    receiver_model.set_modulation(ReceiverModel::Mode::SpectrumAnalysis);
	receiver_model.set_sampling_rate(LOOKING_GLASS_SLICE_WIDTH); //20mhz
    receiver_model.set_baseband_bandwidth(LOOKING_GLASS_SLICE_WIDTH); // possible values: 1.75/2.5/3.5/5/5.5/6/7/8/9/10/12/14/15/20/24/28MHz
    receiver_model.set_squelch_level(0);
	receiver_model.enable();
}

void GlassView::load_Presets() {
	File presets_file; 		//LOAD /WHIPCALC/ANTENNAS.TXT from microSD
	auto result = presets_file.open("LOOKINGGLASS/PRESETS.TXT");
	presets_db.clear();			//Start with fresh db
	if (result.is_valid()) {
		presets_Default(); 		//There is no txt, store a default range
	} else {

		std::string line;		//There is a txt file
		char one_char[1];		//Read it char by char
		for (size_t pointer=0; pointer < presets_file.size();pointer++) {
			presets_file.seek(pointer);
			presets_file.read(one_char, 1);
			if ((int)one_char[0] > 31) {			//ascii space upwards
				line += one_char[0];				//Add it to the textline
			}
			else if (one_char[0] == '\n') {			//New Line
				txtline_process(line);				//make sense of this textline
				line.clear();						//Ready for next textline
			} 
		}
		if (line.length() > 0) txtline_process(line);	//Last line had no newline at end ?
		if (!presets_db.size()) presets_Default();		//no antenna on txt, use default
    }

    populate_Presets();
}

void GlassView::txtline_process(std::string& line) 
{
	if (line.find("#") != std::string::npos) return;	//Line is just a comment

	size_t comma = line.find(",");          //Get first comma position
    if (comma == std::string::npos) return; //No comma at all

	size_t previous = 0;
	preset_entry new_preset;

    new_preset.min = std::stoi(line.substr(0,comma));
    if (!new_preset.min) return;    //No frequency!
    
    previous = comma + 1;
    comma = line.find(",",previous);		//Search for next delimiter
    if (comma == std::string::npos) return; //No comma at all

    new_preset.max = std::stoi(line.substr(previous,comma - previous));
    if (!new_preset.max) return;    //No frequency!

    new_preset.label = line.substr(comma + 1);
    if (new_preset.label.size() == 0) return; //No label ?
    
    presets_db.push_back(new_preset);   //Add this preset.
}

void GlassView::populate_Presets() 
{
    using option_t = std::pair<std::string, int32_t>;
	using options_t = std::vector<option_t>;
    options_t entries;

	for (preset_entry preset : presets_db) 
        {	//go thru all available presets
            entries.emplace_back(preset.label,entries.size());
        }
    range_presets.set_options(entries);
}

void GlassView::presets_Default() 
{
        presets_db.clear();
        presets_db.push_back({2320, 2560, "DEFAULT WIFI 2.4GHz"});
}

}
