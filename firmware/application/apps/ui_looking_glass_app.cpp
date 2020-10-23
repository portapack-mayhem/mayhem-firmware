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
    uint8_t max_power = 0;
    baseband::spectrum_streaming_stop();

    // Convert bins of this spectrum slice into a representative max_power and when enough, into pixels
    for (uint16_t bin = 0; bin < 256; bin++) //Spectrum.db has 256 bins
    {     // Center 12 bins are ignored (DC spike is blanked) Leftmost and rightmost 2 bins are ignored
        if (bin > 1 && bin < 122) //> 1
        {
            if (spectrum.db[128 + bin] > max_power)
                max_power = spectrum.db[128 + bin];
        }
        else if (bin > 133 && bin < 254) // < 254
        {
            if (spectrum.db[bin - 128] > max_power)
                max_power = spectrum.db[bin - 128];
        }

        bins_Hz_size += each_bin_size;   //add this bin Hz count into the "pixel fulfilled bag of Hz"

        if (bins_Hz_size >= marker_pixel_step) //new pixel fullfilled
        {
            if (min_color_power < max_power)
                add_spectrum_pixel(spectrum_rgb3_lut[max_power]); //Pixel will represent max_power
            else
                add_spectrum_pixel(0);  //Filtered out, show black

            max_power = 0;
            bins_Hz_size = 0;
            if (!pixel_index) //a waterfall line has been completed
                break;
        }
    }

    if (pixel_index) 
        f_center += SEARCH_SLICE_WIDTH; //Move into the next bandwidth slice
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
    display.scroll_set_area( 88, 319); //Restart scrolling on the correct coordinates
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
    field_marker.set_step(marker_pixel_step / MHZ_DIV); //step needs to be a pixel wide.
    f_center_ini = f_min + (SEARCH_SLICE_WIDTH / 2);    //Initial center frequency for sweep
    f_center_ini += SEARCH_SLICE_WIDTH;                 //euquiq: Why do I need to move the center ???!!! (shift needed for marker accuracy)

    PlotMarker(field_marker.value()); //Refresh marker on screen

    f_center = f_center_ini;                        //Reset sweep into first slice
    pixel_index = 0;                                //reset pixel counter
    bins_Hz_size = 0;                               //reset amount of Hz filled up by pixels
    receiver_model.set_tuning_frequency(f_center_ini); //tune rx for this slice
}

void GlassView::PlotMarker(double pos)
{
    pos = pos * MHZ_DIV;
    pos -= f_min;
    pos = pos / marker_pixel_step; //Real pixel 

    portapack::display.fill_rectangle({0, 82, 240, 8}, Color::black()); //Clear old marker and whole marker rectangle btw
    portapack::display.fill_rectangle({pos - 2, 82, 5, 3}, Color::red()); //Red marker middle
    portapack::display.fill_rectangle({pos - 1, 84, 3, 3}, Color::red()); //Red marker middle
    portapack::display.fill_rectangle({pos, 86, 1, 2}, Color::red()); //Red marker middle
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
                  &field_marker,
                  &text_marker_pm
                });
    
    field_frequency_min.set_value(2400);
    field_frequency_min.on_change = [this](int32_t v) {
        if (v >= field_frequency_max.value())
            field_frequency_max.set_value(v + 240);
        this->on_range_changed();
    };

    field_frequency_max.set_value(2640); 
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
		min_color_power = v;
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

    display.scroll_set_area( 88, 319);
    baseband::set_spectrum(SEARCH_SLICE_WIDTH, 16);	// Trigger was 31. Need to understand this parameter.

    on_range_changed();

    receiver_model.set_modulation(ReceiverModel::Mode::SpectrumAnalysis);
	receiver_model.set_sampling_rate(SEARCH_SLICE_WIDTH); //20mhz
    receiver_model.set_baseband_bandwidth(SEARCH_SLICE_WIDTH); // possible values: 1.75/2.5/3.5/5/5.5/6/7/8/9/10/12/14/15/20/24/28MHz
    receiver_model.set_squelch_level(0);
	receiver_model.enable();
}

}
