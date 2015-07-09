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

#include "radio.hpp"

#include "hackrf_hal.hpp"
using namespace hackrf::one;

namespace ui {

/* BasebandStatsView *****************************************************/

BasebandStatsView::BasebandStatsView() {
	add_children({ {
		&text_used,
		&text_idle,
	} });
}

void BasebandStatsView::on_show() {
	context().message_map[Message::ID::BasebandStatistics] = [this](const Message* const p) {
		this->on_statistics_update(static_cast<const BasebandStatisticsMessage*>(p)->statistics);
	};
}

void BasebandStatsView::on_hide() {
	context().message_map[Message::ID::BasebandStatistics] = nullptr;
}


static std::string ticks_to_percent_string(const uint32_t ticks) {
 	const uint32_t percent_x100 = ticks / (base_m4_clk_f / 10000);
	return
		to_string_dec_uint(percent_x100 / 100, 3) + "." +
		to_string_dec_uint(percent_x100 % 100, 2, '0') + "%";
}

void BasebandStatsView::on_statistics_update(const BasebandStatistics& statistics) {
	text_used.set(ticks_to_percent_string(statistics.baseband_ticks));
	text_idle.set(ticks_to_percent_string(statistics.idle_ticks));
}

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

DebugMenuView::DebugMenuView(NavigationView& nav) {
	add_items<7>({ {
		{ "Memory",      [&nav](){ nav.push(new DebugMemoryView    { nav }); } },
		{ "Radio State", [&nav](){ nav.push(new NotImplementedView { nav }); } },
		{ "SD Card",     [&nav](){ nav.push(new NotImplementedView { nav }); } },
		{ "RFFC5072",    [&nav](){ nav.push(new DebugRFFC5072View  { nav }); } },
		{ "MAX2837",     [&nav](){ nav.push(new NotImplementedView { nav }); } },
		{ "Si5351C",     [&nav](){ nav.push(new NotImplementedView { nav }); } },
		{ "WM8731",      [&nav](){ nav.push(new NotImplementedView { nav }); } },
	} });
	on_left = [&nav](){ nav.pop(); };
}

} /* namespace ui */
