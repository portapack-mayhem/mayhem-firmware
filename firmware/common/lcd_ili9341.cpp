/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "lcd_ili9341.hpp"
#include "bmp.hpp"

#include "portapack_io.hpp"
using namespace portapack;

#include "utility.hpp"

#include "ch.h"

#include <complex>

namespace lcd {

namespace {

void lcd_reset() {
	io.lcd_reset_state(false);
	chThdSleepMilliseconds(1);
	io.lcd_reset_state(true);
	chThdSleepMilliseconds(10);
	io.lcd_reset_state(false);
	chThdSleepMilliseconds(120);
}

void lcd_sleep_in() {
	io.lcd_data_write_command_and_data(0x10, {});
	// "It will be necessary to wait 5msec before sending next command,
	// this is to allow time for the supply voltages and clock circuits
	// to stabilize."
	chThdSleepMilliseconds(5);
}

void lcd_sleep_out() {
	io.lcd_data_write_command_and_data(0x11, {});
	// "It will be necessary to wait 120msec after sending Sleep Out
	// command (when in Sleep In Mode) before Sleep In command can be
	// sent."
	chThdSleepMilliseconds(120);
}

void lcd_display_on() {
	io.lcd_data_write_command_and_data(0x29, {});
}

void lcd_display_off() {
	io.lcd_data_write_command_and_data(0x28, {});
}

void lcd_sleep() {
	lcd_display_off();
	lcd_sleep_in();
}

void lcd_wake() {
	lcd_sleep_out();
	lcd_display_on();
}

void lcd_init() {
	// LCDs are configured for IM[2:0] = 001
	// 8080-I system, 16-bit parallel bus

	//
	// 0x3a: DBI[2:0] = 101
	// MDT[1:0] = XX (if not in 18-bit mode, right?)

	// Power control B
	// 0
	// PCEQ=1, DRV_ena=0, Power control=3
	io.lcd_data_write_command_and_data(0xCF, { 0x00, 0xD9, 0x30 });

	// Power on sequence control
	io.lcd_data_write_command_and_data(0xED, { 0x64, 0x03, 0x12, 0x81 });

	// Driver timing control A
	io.lcd_data_write_command_and_data(0xE8, { 0x85, 0x10, 0x78 });

	// Power control A
	io.lcd_data_write_command_and_data(0xCB, { 0x39, 0x2C, 0x00, 0x34, 0x02 });

	// Pump ratio control
	io.lcd_data_write_command_and_data(0xF7, { 0x20 });

	// Driver timing control B
	io.lcd_data_write_command_and_data(0xEA, { 0x00, 0x00 });

	io.lcd_data_write_command_and_data(0xB1, { 0x00, 0x1B });

	// Blanking Porch Control
	// VFP = 0b0000010 = 2 (number of HSYNC of vertical front porch)
	// VBP = 0b0000010 = 2 (number of HSYNC of vertical back porch)
	// HFP = 0b0001010 = 10 (number of DOTCLOCK of horizontal front porch)
	// HBP = 0b0010100 = 20 (number of DOTCLOCK of horizontal back porch)
	io.lcd_data_write_command_and_data(0xB5, { 0x02, 0x02, 0x0a, 0x14 });

	// Display Function Control
	// PT[1:0] = 0b10
	// PTG[1:0] = 0b10
	// ISC[3:0] = 0b0010 (scan cycle interval of gate driver: 5 frames)
	// SM = 0 (gate driver pin arrangement in combination with GS)
	// SS = 1 (source output scan direction S720 -> S1)
	// GS = 0 (gate output scan direction G1 -> G320)
	// REV = 1 (normally white)
	// NL = 0b100111 (default)
	// PCDIV = 0b000000 (default?)
	io.lcd_data_write_command_and_data(0xB6, { 0x0A, 0xA2, 0x27, 0x00 });

	// Power Control 1
	//VRH[5:0]
	io.lcd_data_write_command_and_data(0xC0, { 0x1B });

	// Power Control 2
	//SAP[2:0];BT[3:0]
	io.lcd_data_write_command_and_data(0xC1, { 0x12 });

	// VCOM Control 1
	io.lcd_data_write_command_and_data(0xC5, { 0x32, 0x3C });

	// VCOM Control 2
	io.lcd_data_write_command_and_data(0xC7, { 0x9B });

	// Memory Access Control
	// Invert X and Y memory access order, so upper-left of
	// screen is (0,0) when writing to display.
	io.lcd_data_write_command_and_data(0x36, {
		(1 << 7) |	// MY=1
		(1 << 6) |	// MX=1
		(0 << 5) |	// MV=0
		(1 << 4) |	// ML=1: reverse vertical refresh to simplify scrolling logic
		(1 << 3)	// BGR=1: For Kingtech LCD, BGR filter.
	});

	// COLMOD: Pixel Format Set
	// DPI=101 (16 bits/pixel), DBI=101 (16 bits/pixel)
	io.lcd_data_write_command_and_data(0x3A, { 0x55 });

	//io.lcd_data_write_command_and_data(0xF6, { 0x01, 0x30 });
	// WEMODE=1 (reset column and page number on overflow)
	// MDT[1:0]
	// EPF[1:0]=00 (use channel MSB for LSB)
	// RIM=0 (If COLMOD[6:4]=101 (65k color), 16-bit RGB interface (1 transfer/pixel))
	// RM=0 (system interface/VSYNC interface)
	// DM[1:0]=00 (internal clock operation)
	// ENDIAN=0 (doesn't matter with 16-bit interface)
	io.lcd_data_write_command_and_data(0xF6, { 0x01, 0x30, 0x00 });

	// 3Gamma Function Disable
	io.lcd_data_write_command_and_data(0xF2, { 0x00 });

	// Gamma curve selected
	io.lcd_data_write_command_and_data(0x26, { 0x01 });

	// Set Gamma
	io.lcd_data_write_command_and_data(0xE0, {
		0x0F, 0x1D, 0x19, 0x0E, 0x10, 0x07, 0x4C, 0x63,
		0x3F, 0x03, 0x0D, 0x00, 0x26, 0x24, 0x04
	});

	// Set Gamma
	io.lcd_data_write_command_and_data(0xE1, {
		0x00, 0x1C, 0x1F, 0x02, 0x0F, 0x03, 0x35, 0x25,
		0x47, 0x04, 0x0C, 0x0B, 0x29, 0x2F, 0x05
	});

	lcd_wake();

	// Turn on Tearing Effect Line (TE) output signal.
	io.lcd_data_write_command_and_data(0x35, { 0b00000000 });
}

void lcd_set(const uint_fast8_t command, const uint_fast16_t start, const uint_fast16_t end) {
	io.lcd_data_write_command_and_data(command, {
		static_cast<uint8_t>(start >> 8), static_cast<uint8_t>(start & 0xff),
		static_cast<uint8_t>(end   >> 8), static_cast<uint8_t>(end   & 0xff)
	});
}

void lcd_ramwr_start() {
	io.lcd_data_write_command_and_data(0x2c, {});
}

void lcd_ramrd_start() {
	io.lcd_data_write_command_and_data(0x2e, {});
	io.lcd_read_word();
}

void lcd_caset(const uint_fast16_t start_column, uint_fast16_t end_column) {
	lcd_set(0x2a, start_column, end_column);
}

void lcd_paset(const uint_fast16_t start_page, const uint_fast16_t end_page) {
	lcd_set(0x2b, start_page, end_page);
}

void lcd_start_ram_write(
	const ui::Point p,
	const ui::Size s
) {
	lcd_caset(p.x(), p.x() + s.width()  - 1);
	lcd_paset(p.y(), p.y() + s.height() - 1);
	lcd_ramwr_start();
}

void lcd_start_ram_read(
	const ui::Point p,
	const ui::Size s
) {
	lcd_caset(p.x(), p.x() + s.width()  - 1);
	lcd_paset(p.y(), p.y() + s.height() - 1);
	lcd_ramrd_start();
}

void lcd_start_ram_write(
	const ui::Rect& r
) {
	lcd_start_ram_write(r.location(), r.size());
}

void lcd_start_ram_read(
	const ui::Rect& r
) {
	lcd_start_ram_read(r.location(), r.size());
}

void lcd_vertical_scrolling_definition(
	const uint_fast16_t top_fixed_area,
	const uint_fast16_t vertical_scrolling_area,
	const uint_fast16_t bottom_fixed_area
) {
	io.lcd_data_write_command_and_data(0x33, {
		static_cast<uint8_t>(top_fixed_area            >> 8),
		static_cast<uint8_t>(top_fixed_area          & 0xff),
		static_cast<uint8_t>(vertical_scrolling_area   >> 8),
		static_cast<uint8_t>(vertical_scrolling_area & 0xff),
		static_cast<uint8_t>(bottom_fixed_area         >> 8),
		static_cast<uint8_t>(bottom_fixed_area       & 0xff)
	});
}

void lcd_vertical_scrolling_start_address(
	const uint_fast16_t vertical_scrolling_pointer
) {
	io.lcd_data_write_command_and_data(0x37, {
		static_cast<uint8_t>(vertical_scrolling_pointer >> 8),
		static_cast<uint8_t>(vertical_scrolling_pointer & 0xff)
	});
}

}

void ILI9341::init() {
	lcd_reset();
	lcd_init();
}

void ILI9341::shutdown() {
	lcd_reset();
}

void ILI9341::sleep() {
	lcd_sleep();
}

void ILI9341::wake() {
	lcd_wake();
}

void ILI9341::fill_rectangle(ui::Rect r, const ui::Color c) {
	const auto r_clipped = r.intersect(screen_rect());
	if( !r_clipped.is_empty() ) {
		lcd_start_ram_write(r_clipped);
		size_t count = r_clipped.width() * r_clipped.height();
		io.lcd_write_pixels(c, count);
	}
}

void ILI9341::fill_rectangle_unrolled8(ui::Rect r, const ui::Color c) {
	const auto r_clipped = r.intersect(screen_rect());
	if( !r_clipped.is_empty() ) {
		lcd_start_ram_write(r_clipped);
		size_t count = r_clipped.width() * r_clipped.height();
		io.lcd_write_pixels_unrolled8(c, count);
	}
}

void ILI9341::render_line(const ui::Point p, const uint8_t count, const ui::Color* line_buffer) {
	lcd_start_ram_write(p, { count, 1 });
	io.lcd_write_pixels(line_buffer, count);
}

void ILI9341::render_box(const ui::Point p, const ui::Size s, const ui::Color* line_buffer) {
	lcd_start_ram_write(p, s);
	io.lcd_write_pixels(line_buffer, s.width() * s.height());
}

// RLE_4 BMP loader (delta not implemented)
void ILI9341::drawBMP(const ui::Point p, const uint8_t * bitmap, const bool transparency) {
	const bmp_header_t * bmp_header = (const bmp_header_t *)bitmap;
	uint32_t data_idx;
	uint8_t by, c, count, transp_idx = 0;
	ui::Color line_buffer[240];
	uint16_t px = 0, py;
	ui::Color palette[16];
	
	// Abort if bad depth or no RLE
	if ((bmp_header->bpp != 4) ||
		(bmp_header->compression != 2)) return;

	data_idx = bmp_header->image_data;
	const bmp_palette_t * bmp_palette = (const bmp_palette_t *)&bitmap[bmp_header->BIH_size + 14];
	
	// Convert palette and find pure magenta index (alpha color key)
	for (c = 0; c < 16; c++) {
		palette[c] = ui::Color(bmp_palette->color[c].R, bmp_palette->color[c].G, bmp_palette->color[c].B);
		if ((bmp_palette->color[c].R == 0xFF) &&
			(bmp_palette->color[c].G == 0x00) &&
			(bmp_palette->color[c].B == 0xFF)) transp_idx = c;
	}

	if (!transparency) {
		py = bmp_header->height + 16;
		do {
			by = bitmap[data_idx++];
			if (by) {
				count = by >> 1;
				by = bitmap[data_idx++];
				for (c = 0; c < count; c++) {
					line_buffer[px++] = palette[by >> 4];
					if (px < bmp_header->width) line_buffer[px++] = palette[by & 15];
				}
				if (data_idx & 1) data_idx++;
			} else {
				by = bitmap[data_idx++];
				if (by == 0) {
					render_line({p.x(), p.y() + py}, bmp_header->width, line_buffer);
					py--;
					px = 0;
				} else if (by == 1) {
					break;
				} else if (by == 2) {
					// Delta
				} else {
					count = by >> 1;
					for (c = 0; c < count; c++) {
						by = bitmap[data_idx++];
						line_buffer[px++] = palette[by >> 4];
						if (px < bmp_header->width) line_buffer[px++] = palette[by & 15];
					}
					if (data_idx & 1) data_idx++;
				}
			}
		} while (1);
	} else {
		py = bmp_header->height;
		do {
			by = bitmap[data_idx++];
			if (by) {
				count = by >> 1;
				by = bitmap[data_idx++];
				for (c = 0; c < count; c++) {
					if ((by >> 4) != transp_idx) draw_pixel({static_cast<ui::Coord>(p.x() + px), static_cast<ui::Coord>(p.y() + py)}, palette[by >> 4]);
					px++;
					if (px < bmp_header->width) {
						if ((by & 15) != transp_idx) draw_pixel({static_cast<ui::Coord>(p.x() + px), static_cast<ui::Coord>(p.y() + py)}, palette[by & 15]);
					}
					px++;
				}
				if (data_idx & 1) data_idx++;
			} else {
				by = bitmap[data_idx++];
				if (by == 0) {
					py--;
					px = 0;
				} else if (by == 1) {
					break;
				} else if (by == 2) {
					// Delta
				} else {
					count = by >> 1;
					for (c = 0; c < count; c++) {
						by = bitmap[data_idx++];
						if ((by >> 4) != transp_idx) draw_pixel({static_cast<ui::Coord>(p.x() + px), static_cast<ui::Coord>(p.y() + py)}, palette[by >> 4]);
						px++;
						if (px < bmp_header->width) {
							if ((by & 15) != transp_idx) draw_pixel({static_cast<ui::Coord>(p.x() + px), static_cast<ui::Coord>(p.y() + py)}, palette[by & 15]);
						}
						px++;
					}
					if (data_idx & 1) data_idx++;
				}
			}
		} while (1);
	}
}

void ILI9341::draw_line(const ui::Point start, const ui::Point end, const ui::Color color) {
	int x0 = start.x();
	int y0 = start.y();
	int x1 = end.x();
	int y1 = end.y();
	
	int dx = std::abs(x1-x0), sx = x0<x1 ? 1 : -1;
	int dy = std::abs(y1-y0), sy = y0<y1 ? 1 : -1; 
	int err = (dx>dy ? dx : -dy)/2, e2;
 
	for(;;){
		draw_pixel({static_cast<ui::Coord>(x0), static_cast<ui::Coord>(y0)}, color);
		if (x0==x1 && y0==y1) break;
		e2 = err;
		if (e2 >-dx) { err -= dy; x0 += sx; }
		if (e2 < dy) { err += dx; y0 += sy; }
	}
}

void ILI9341::fill_circle(
	const ui::Point center,
	const ui::Dim radius,
	const ui::Color foreground,
	const ui::Color background
) {
	const uint32_t radius2 = radius * radius;
	for(int32_t y=-radius; y<radius; y++) {
		const int32_t y2 = y * y;
		for(int32_t x=-radius; x<radius; x++) {
			const int32_t x2 = x * x;
			const uint32_t d2 = x2 + y2;
			const bool inside = d2 < radius2;
			const auto color = inside ? foreground : background;
			draw_pixel({ x + center.x(), y + center.y() }, color);
		}
	}
}

void ILI9341::draw_pixel(
	const ui::Point p,
	const ui::Color color
) {
	if( screen_rect().contains(p) ) {
		lcd_start_ram_write(p, { 1, 1 });
		io.lcd_write_pixel(color);
	}
}

void ILI9341::draw_pixels(
	const ui::Rect r,
	const ui::Color* const colors,
	const size_t count
) {
	/* TODO: Assert that rectangle width x height < count */
	lcd_start_ram_write(r);
	io.lcd_write_pixels(colors, count);
}

void ILI9341::read_pixels(
	const ui::Rect r,
	ui::ColorRGB888* const colors,
	const size_t count
) {
	/* TODO: Assert that rectangle width x height < count */
	lcd_start_ram_read(r);
	io.lcd_read_bytes(
		reinterpret_cast<uint8_t*>(colors),
		count * sizeof(ui::ColorRGB888)
	);
}

void ILI9341::draw_bitmap(
	const ui::Point p,
	const ui::Size size,
	const uint8_t* const pixels,
	const ui::Color foreground,
	const ui::Color background
) {
	lcd_start_ram_write(p, size);

	const size_t count = size.width() * size.height();
	for(size_t i=0; i<count; i++) {
		const auto pixel = pixels[i >> 3] & (1U << (i & 0x7));
		io.lcd_write_pixel(pixel ? foreground : background);
	}
}

void ILI9341::draw_glyph(
	const ui::Point p,
	const ui::Glyph& glyph,
	const ui::Color foreground,
	const ui::Color background
) {
	draw_bitmap(p, glyph.size(), glyph.pixels(), foreground, background);
}

void ILI9341::scroll_set_area(
	const ui::Coord top_y,
	const ui::Coord bottom_y
) {
	scroll_state.top_area = top_y;
	scroll_state.bottom_area = height() - bottom_y;
	scroll_state.height = bottom_y - top_y;
	lcd_vertical_scrolling_definition(scroll_state.top_area, scroll_state.height, scroll_state.bottom_area);
}

ui::Coord ILI9341::scroll_set_position(
	const ui::Coord position
) {
	scroll_state.current_position = position % scroll_state.height;
	const uint_fast16_t address = scroll_state.top_area + scroll_state.current_position;
	lcd_vertical_scrolling_start_address(address);
	return address;
}

ui::Coord ILI9341::scroll(const int32_t delta) {
	return scroll_set_position(scroll_state.current_position + scroll_state.height - delta);
}

ui::Coord ILI9341::scroll_area_y(const ui::Coord y) const {
	const auto wrapped_y = (scroll_state.current_position + y) % scroll_state.height;
	return wrapped_y + scroll_state.top_area;
}

void ILI9341::scroll_disable() {
	lcd_vertical_scrolling_definition(0, height(), 0);
	lcd_vertical_scrolling_start_address(0);
}

} /* namespace lcd */
