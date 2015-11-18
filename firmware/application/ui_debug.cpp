/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_debug.hpp"

#include "ch.h"

#include "ff.h"
#include "led.hpp"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"
#include "portapack_shared_memory.hpp"

#include "radio.hpp"

namespace ui {
	
FRESULT fr;         /* FatFs function common result code */

DebugMemoryView::DebugMemoryView(NavigationView& nav) {
	add_children({ {
		&text_title,
		&text_label_m0_free,
		&text_label_m0_free_value,
		&text_label_m0_heap_fragmented_free,
		&text_label_m0_heap_fragmented_free_value,
		&text_label_m0_heap_fragments,
		&text_label_m0_heap_fragments_value,
		&button_done
	} });

	const auto m0_free = chCoreStatus();
	text_label_m0_free_value.set(to_string_dec_uint(m0_free, 5));

	size_t m0_fragmented_free_space = 0;
	const auto m0_fragments = chHeapStatus(NULL, &m0_fragmented_free_space);
	text_label_m0_heap_fragmented_free_value.set(to_string_dec_uint(m0_fragmented_free_space, 5));
	text_label_m0_heap_fragments_value.set(to_string_dec_uint(m0_fragments, 5));

	button_done.on_select = [&nav](Button&){ nav.pop(); };
}

void DebugMemoryView::focus() {
	button_done.focus();
}

void DebugRFFC5072RegistersWidget::update() {
	set_dirty();
}

void DebugRFFC5072RegistersWidget::paint(Painter& painter) {
	draw_legend(painter);

	const auto registers = radio::first_if.registers();
	draw_values(painter, registers);
}

void DebugRFFC5072RegistersWidget::draw_legend(Painter& painter) {
	for(size_t i=0; i<registers_count; i+=registers_per_row) {
		const Point offset {
			0, static_cast<Coord>((i / registers_per_row) * row_height)
		};

		const auto text = to_string_hex(i, legend_length);
		painter.draw_string(
			screen_pos() + offset,
			style(),
			text
		);
	}
}

void DebugRFFC5072RegistersWidget::draw_values(
	Painter& painter,
	const rffc507x::RegisterMap registers
) {
	for(size_t i=0; i<registers_count; i++) {
		const Point offset = {
			static_cast<Coord>(legend_width + 8 + (i % registers_per_row) * (value_width + 8)),
			static_cast<Coord>((i / registers_per_row) * row_height)
		};

		const uint16_t value = registers.w[i];

		const auto text = to_string_hex(value, value_length);
		painter.draw_string(
			screen_pos() + offset,
			style(),
			text
		);
	}
}

DebugRFFC5072View::DebugRFFC5072View(NavigationView& nav) {
	add_children({ {
		&text_title,
		&widget_registers,
		&button_update,
		&button_done,
	} });

	button_update.on_select = [this](Button&){
		this->widget_registers.update();
	};
	button_done.on_select = [&nav](Button&){ nav.pop(); };
}

void DebugRFFC5072View::focus() {
	button_done.focus();
}

void DebugSDView::paint(Painter& painter) {
	const Point offset = {
		static_cast<Coord>(32),
		static_cast<Coord>(32)
	};
	
	const auto text = to_string_hex(fr, 2);
	painter.draw_string(
		screen_pos() + offset,
		style(),
		text
	);
}

DebugSDView::DebugSDView(NavigationView& nav) {
	add_children({ {
		&text_title,
		&button_makefile,
		&button_done
	} });
	
	button_makefile.on_select = [this](Button&){
 		FATFS fs;         	/* Work area (file system object) for logical drives */
		FIL fdst;      		/* File objects */
		int16_t buffer[512];  	/* File copy buffer */
		UINT bw;    /* File read/write count */
		
		sdcConnect(&SDCD1);

		fr = f_mount(&fs, "", 1);

		fr = f_open(&fdst, "TST.SND", FA_OPEN_EXISTING | FA_READ);
		
		//if (!fr) led_rx.on();
		
		/*fr = f_read(&fdst, buffer, 512*2, &bw);
		
		Coord oy,ny;
		
		oy = 128;
		
		for (int c=0;c<512;c++) {
			ny = 128+32-(buffer[c]>>10);
			portapack::display.draw_line({static_cast<Coord>(c/3),oy},{static_cast<Coord>((c+1)/3),ny},{255,127,0});
			oy = ny;
		}*/
	
		/*
		//if (fr) return;

		fr = f_write(&fdst, buffer, br, &bw);
		//if (fr || bw < br) return;*/

		//set_dirty();
		
		f_close(&fdst);

		f_mount(NULL, "", 0);
		
	};
	
	button_done.on_select = [&nav](Button&){ nav.pop(); };
}

void DebugSDView::focus() {
	button_done.focus();
}

void DebugLCRView::paint(Painter& painter) {
	const Point offset = {
		static_cast<Coord>(32),
		static_cast<Coord>(32)
	};
	
	const auto text = to_string_hex(fr, 2);
	painter.draw_string(
		screen_pos() + offset,
		style(),
		text
	);
}

char hexify(char in) {
	if (in > 9) in += 7;
	return in + 0x30;
}

DebugLCRView::DebugLCRView(NavigationView& nav, char * lcrstring, uint8_t checksum) {
	char cstr[15] = "Checksum: 0x  ";
	
	add_children({ {
		&text_lcr1,
		&text_lcr2,
		&text_lcr3,
		&text_lcr4,
		&text_lcr5,
		&text_checksum,
		&button_done
	} });
	
	std::string b = std::string(lcrstring);
	
	text_lcr1.set(b.substr(8+(0*26),26));
	if (strlen(lcrstring) > 34) text_lcr2.set(b.substr(8+(1*26),26));
	if (strlen(lcrstring) > 34+26) text_lcr3.set(b.substr(8+(2*26),26));
	if (strlen(lcrstring) > 34+26+26) text_lcr4.set(b.substr(8+(3*26),26));
	if (strlen(lcrstring) > 34+26+26+26) text_lcr5.set(b.substr(8+(4*26),26));
	
	cstr[12] = hexify(checksum >> 4);
	cstr[13] = hexify(checksum & 15);
	
	text_checksum.set(cstr);
	
	button_done.on_select = [&nav](Button&){ nav.pop(); };
}

void DebugLCRView::focus() {
	button_done.focus();
}

DebugMenuView::DebugMenuView(NavigationView& nav) {
	add_items<7>({ {
		{ "Memory", ui::Color::white(),      [&nav](){ nav.push(new DebugMemoryView    { nav }); } },
		{ "Radio State", ui::Color::white(), [&nav](){ nav.push(new NotImplementedView { nav }); } },
		{ "SD Card", ui::Color::white(),     [&nav](){ nav.push(new DebugSDView        { nav }); } },
		{ "RFFC5072", ui::Color::white(),    [&nav](){ nav.push(new DebugRFFC5072View  { nav }); } },
		{ "MAX2837", ui::Color::white(),     [&nav](){ nav.push(new NotImplementedView { nav }); } },
		{ "Si5351C", ui::Color::white(),     [&nav](){ nav.push(new NotImplementedView { nav }); } },
		{ "WM8731", ui::Color::white(),      [&nav](){ nav.push(new NotImplementedView { nav }); } },
	} });
	on_left = [&nav](){ nav.pop(); };
}

} /* namespace ui */
