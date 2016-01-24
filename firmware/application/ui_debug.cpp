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
#include "string_format.hpp"

namespace ui {

/* DebugMemoryView *******************************************************/

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

/* TemperatureWidget *****************************************************/

void TemperatureWidget::paint(Painter& painter) {
	const auto logger = portapack::temperature_logger;

	const auto rect = screen_rect();
	const Color color_background { 0, 0, 64 };
	const Color color_foreground = Color::green();
	const Color color_reticle { 128, 128, 128 };

	const Dim graph_width = logger.capacity() * bar_width;
	const Dim graph_height = rect.height() - 16;
	const Rect graph_rect {
		rect.left() + (rect.width() - graph_width) / 2, rect.top() + 8,
		graph_width, rect.height()
	};
	const Rect frame_rect {
		graph_rect.left() - 1, graph_rect.top() - 1,
		graph_rect.width() + 2, graph_rect.height() + 2
	};
	painter.draw_rectangle(frame_rect, color_reticle);
	painter.fill_rectangle(graph_rect, color_background);

	const auto history = logger.history();
	for(size_t i=0; i<history.size(); i++) {
		const Coord x = graph_rect.right() - (history.size() - i) * bar_width;
		const auto sample = history[i];
		const auto temp = temperature(sample);
		const auto y = screen_y(temp, graph_rect);
		const Dim bar_height = graph_rect.bottom() - y;
		painter.fill_rectangle({ x, y, bar_width, bar_height }, color_foreground);
	}

	if( !history.empty() ) {
		const auto sample = history.back();
		const auto temp = temperature(sample);
		const auto last_y = screen_y(temp, graph_rect);
		const Coord x = graph_rect.right() + 8;
		const Coord y = last_y - 8;

		painter.draw_string({ x, y }, style(), temperature_str(temp));
	}

	const auto display_temp_max = display_temp_min + (graph_rect.height() / display_temp_scale);
	for(auto temp=display_temp_min; temp<=display_temp_max; temp+=10) {
		const int32_t tick_length = 6;
		const auto tick_x = graph_rect.left() - tick_length;
		const auto tick_y = screen_y(temp, graph_rect);
		painter.fill_rectangle({ tick_x, tick_y, tick_length, 1 }, color_reticle);
		const auto text_x = graph_rect.left() - temp_len * 8 - 8;
		const auto text_y = tick_y - 8;
		painter.draw_string({ text_x, text_y }, style(), temperature_str(temp));
	}
}

TemperatureWidget::temperature_t TemperatureWidget::temperature(const sample_t sensor_value) const {
	return -45 + sensor_value * 5;
}

std::string TemperatureWidget::temperature_str(const temperature_t temperature) const {
	return to_string_dec_int(temperature, temp_len - 1) + "C";
}

Coord TemperatureWidget::screen_y(
	const temperature_t temperature,
	const Rect& rect
) const {
	const Coord y_raw = rect.bottom() - ((temperature - display_temp_min) * display_temp_scale);
	const auto y_limit = std::min(rect.bottom(), std::max(rect.top(), y_raw));
	return y_limit;
}

/* TemperatureView *******************************************************/

TemperatureView::TemperatureView(NavigationView& nav) {
	add_children({ {
		&text_title,
		&temperature_widget,
		&button_done,
	} });

	button_done.on_select = [&nav](Button&){ nav.pop(); };
}

void TemperatureView::focus() {
	button_done.focus();
}

/* RegistersWidget *******************************************************/

RegistersWidget::RegistersWidget(
	RegistersWidgetConfig&& config,
	std::function<uint32_t(const size_t register_number)>&& reader
) : Widget { },
	config(std::move(config)),
	reader(std::move(reader))
{
}

void RegistersWidget::update() {
	set_dirty();
}

void RegistersWidget::paint(Painter& painter) {
	const Coord left = (size().w - config.row_width()) / 2;

	draw_legend(left, painter);
	draw_values(left, painter);
}

void RegistersWidget::draw_legend(const Coord left, Painter& painter) {
	const auto pos = screen_pos();

	for(size_t i=0; i<config.registers_count; i+=config.registers_per_row) {
		const Point offset {
			left, (i / config.registers_per_row) * row_height
		};

		const auto text = to_string_hex(i, config.legend_length);
		painter.draw_string(
			pos + offset,
			style().invert(),
			text
		);
	}
}

void RegistersWidget::draw_values(
	const Coord left,
	Painter& painter
) {
	const auto pos = screen_pos();

	for(size_t i=0; i<config.registers_count; i++) {
		const Point offset = {
			left + config.legend_width() + 8 + (i % config.registers_per_row) * (config.value_width() + 8),
			(i / config.registers_per_row) * row_height
		};

		const auto value = reader(i);

		const auto text = to_string_hex(value, config.value_length);
		painter.draw_string(
			pos + offset,
			style(),
			text
		);
	}
}

/* RegistersView *********************************************************/

RegistersView::RegistersView(
	NavigationView& nav,
	const std::string& title,
	RegistersWidgetConfig&& config,
	std::function<uint32_t(const size_t register_number)>&& reader
) : registers_widget { std::move(config), std::move(reader) }
{
	add_children({ {
		&text_title,
		&registers_widget,
		&button_update,
		&button_done,
	} });

	button_update.on_select = [this](Button&){
		this->registers_widget.update();
	};
	button_done.on_select = [&nav](Button&){ nav.pop(); };

	registers_widget.set_parent_rect({ 0, 48, 240, 192 });

	text_title.set_parent_rect({
		(240 - title.size() * 8) / 2, 16,
		title.size() * 8, 16
	});
	text_title.set(title);
}

void RegistersView::focus() {
	button_done.focus();
}

/* DebugMenuView *********************************************************/

DebugMenuView::DebugMenuView(NavigationView& nav) {
	add_items<8>({ {
		{ "Memory",      [&nav](){ nav.push<DebugMemoryView>(); } },
		{ "Radio State", [&nav](){ nav.push<NotImplementedView>(); } },
		{ "SD Card",     [&nav](){ nav.push<NotImplementedView>(); } },
		{ "RFFC5072",    [&nav](){ nav.push<RegistersView>(
			"RFFC5072", RegistersWidgetConfig { 31, 2, 4, 4 },
			[](const size_t register_number) { return radio::first_if.read(register_number); }
		); } },
		{ "MAX2837",     [&nav](){ nav.push<RegistersView>(
			"MAX2837", RegistersWidgetConfig { 32, 2, 3, 4 },
			[](const size_t register_number) { return radio::second_if.read(register_number); }
		); } },
		{ "Si5351C",     [&nav](){ nav.push<RegistersView>(
			"Si5351C", RegistersWidgetConfig { 96, 2, 2, 8 },
			[](const size_t register_number) { return portapack::clock_generator.read_register(register_number); }
		); } },
		{ "WM8731",      [&nav](){ nav.push<RegistersView>(
			"WM8731", RegistersWidgetConfig { wolfson::wm8731::reg_count, 1, 3, 4 },
			[](const size_t register_number) { return portapack::audio_codec.read(register_number); }
		); } },
		{ "Temperature", [&nav](){ nav.push<TemperatureView>(); } },
	} });
	on_left = [&nav](){ nav.pop(); };
}

} /* namespace ui */
