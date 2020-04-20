/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2020 Shao
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

#include "ui_tv.hpp"

#include "spectrum_color_lut.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "baseband_api.hpp"

#include "string_format.hpp"

#include <cmath>
#include <array>

namespace ui {
namespace tv {

/* TimeScopeView******************************************************/

TimeScopeView::TimeScopeView(
	const Rect parent_rect
) : View { parent_rect }
{
	set_focusable(true);
	
	add_children({
		//&labels,
		//&field_frequency,
		&waveform
	});
	
	/*field_frequency.on_change = [this](int32_t) {
		set_dirty();
	};
	field_frequency.set_value(10);*/
}

void TimeScopeView::paint(Painter& painter) {
	const auto r = screen_rect();

	painter.fill_rectangle(r, Color::black());
	
	// Cursor
	/*
	const Rect r_cursor {
		field_frequency.value() / (48000 / 240), r.bottom() - 32 - cursor_band_height,
		1, cursor_band_height
	};
	painter.fill_rectangle(
		r_cursor,
		Color::red()
	);*/
}

void TimeScopeView::on_audio_spectrum(const AudioSpectrum* spectrum) {
	for (size_t i = 0; i < spectrum->db.size(); i++)
		audio_spectrum[i] = ((int16_t)spectrum->db[i] - 127) * 256;
	waveform.set_dirty();
}

/* TVView *********************************************************/

void TVView::on_show() {
	clear();

	const auto screen_r = screen_rect();
	display.scroll_set_area(screen_r.top(), screen_r.bottom());
}

void TVView::on_hide() {
	/* TODO: Clear region to eliminate brief flash of content at un-shifted
	 * position?
	 */
	display.scroll_disable();
}

void TVView::paint(Painter& painter) {
	// Do nothing.
	(void)painter;
}

void TVView::on_adjust_xcorr(uint8_t xcorr){
	x_correction = xcorr;
}

void TVView::on_channel_spectrum(
	const ChannelSpectrum& spectrum
) {
	//portapack has limitations
        // 1.screen resolution (less than 240x320) 2.samples each call back (128 or 256)
	// 3.memory size (for ui::Color, the buffer size
	//spectrum.db[i] is 256 long
	//768x625 ->128x625 ->128x312 -> 128x104
	//originally @6MHz sample rate, the PAL should be 768x625
        //I reduced sample rate to 2MHz(3 times less samples), then calculate mag (effectively decimate by 2)
	//the resolution is now changed to 128x625. The total decimation factor is 6, which changes how many samples in a line
	//However 625 is too large for the screen, also interlaced scanning is harder to realize in portapack than normal computer.
        //So I decided to simply drop half of the lines, once y is larger than 625/2=312.5 or 312, I recognize it as a new frame.
        //then the resolution is changed to 128x312
        //128x312 is now able to put into a 240x320 screen, but the buffer for a whole frame is 128x312=39936, which is too large
        //according to my test, I can only make a buffer with a length of 13312 of type ui::Color. which is 1/3 of what I wanted.
        //So now the resolution is changed to 128x104, the height is shrinked to 1/3 of the original height.
        //I was expecting to see 1/3 height of original video.

        //Look how nice is that! I am now able to meet the requirements of 1 and 3 for portapack. Also the length of a line is 128
        //Each call back gives me 256 samples which is exactly 2 lines. What a coincidence!

	//After some experiment, I did some improvements.
	//1.I found that instead of 1/3 of the frame is shown, I got 3 whole frames shrinked into one window.
	//So I made the height twice simply by painting 2 identical lines in the place of original lines
	//2.I found sometimes there is an horizontal offset, so I added x_correction to move the frame back to center manually
	//3.I changed video_buffer's type, from ui::Color to uint_8, since I don't need 3 digit to represent a grey scale value.
	//I was hoping that by doing this, I can have a longer buffer like 39936, then the frame will looks better vertically
	//however this is useless until now.	

	for(size_t i=0; i<256; i++) 
	{
		//video_buffer[i+count*256] = spectrum_rgb4_lut[spectrum.db[i]];
		video_buffer_int[i+count*256] = 255 - spectrum.db[i];
	}
	count = count + 1;
	if (count == 52 -1)
	{
		ui::Color line_buffer[128];
		Coord line;
		uint32_t bmp_px;

		/*for (line = 0; line < 104; line++)
		{
			for (bmp_px = 0; bmp_px < 128; bmp_px++) 
			{
				//line_buffer[bmp_px] = video_buffer[bmp_px+line*128];
				line_buffer[bmp_px] = spectrum_rgb4_lut[video_buffer_int[bmp_px+line*128 + x_correction]];
			}

			display.render_line({ 0, line + 100 }, 128, line_buffer);
		}*/
		for (line = 0; line < 208; line=line+2)
		{
			for (bmp_px = 0; bmp_px < 128; bmp_px++) 
			{
				//line_buffer[bmp_px] = video_buffer[bmp_px+line*128];
				line_buffer[bmp_px] = spectrum_rgb4_lut[video_buffer_int[bmp_px+line/2*128 + x_correction]];
			}

			display.render_line({ 0, line + 100 }, 128, line_buffer);
			display.render_line({ 0, line + 101 }, 128, line_buffer);
		}
		count = 0;
	}

}

void TVView::clear() {
	display.fill_rectangle(
		screen_rect(),
		Color::black()
	);
}

/* TVWidget *******************************************************/

TVWidget::TVWidget(const bool cursor) {
	add_children({
		&tv_view,
		&field_xcorr
	});
	field_xcorr.set_value(10);
}

void TVWidget::on_show() {
	baseband::spectrum_streaming_start();
}

void TVWidget::on_hide() {
	baseband::spectrum_streaming_stop();
}

void TVWidget::show_audio_spectrum_view(const bool show) {
	if ((audio_spectrum_view && show) || (!audio_spectrum_view && !show)) return;
	
	if (show) {
		audio_spectrum_view = std::make_unique<TimeScopeView>(audio_spectrum_view_rect);
		add_child(audio_spectrum_view.get());
		update_widgets_rect();
	} else {
		audio_spectrum_update = false;
		remove_child(audio_spectrum_view.get());
		audio_spectrum_view.reset();
		update_widgets_rect();
	}
}

void TVWidget::update_widgets_rect() {
	if (audio_spectrum_view) {
		tv_view.set_parent_rect(tv_reduced_rect);
	} else {
		tv_view.set_parent_rect(tv_normal_rect);
	}
	tv_view.on_show();
}

void TVWidget::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);
	
	tv_normal_rect = { 0, scale_height, new_parent_rect.width(), new_parent_rect.height() - scale_height};
	tv_reduced_rect = { 0, audio_spectrum_height + scale_height, new_parent_rect.width(), new_parent_rect.height() - scale_height - audio_spectrum_height };
	
	update_widgets_rect();
}

void TVWidget::paint(Painter& painter) {
	// TODO:
	(void)painter;
}

void TVWidget::on_channel_spectrum(const ChannelSpectrum& spectrum) {
	tv_view.on_channel_spectrum(spectrum);
	tv_view.on_adjust_xcorr(field_xcorr.value());
	sampling_rate = spectrum.sampling_rate;
	
}

void TVWidget::on_audio_spectrum() {
	audio_spectrum_view->on_audio_spectrum(audio_spectrum_data);
}

} /* namespace tv */
} /* namespace ui */
