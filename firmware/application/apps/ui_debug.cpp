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

#include "audio.hpp"

#include "ui_sd_card_debug.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "irq_controls.hpp"

namespace ui {

/* DebugMemoryView *******************************************************/

DebugMemoryView::DebugMemoryView(NavigationView& nav) {
	add_children({
		&text_title,
		&text_label_m0_core_free,
		&text_label_m0_core_free_value,
		&text_label_m0_heap_fragmented_free,
		&text_label_m0_heap_fragmented_free_value,
		&text_label_m0_heap_fragments,
		&text_label_m0_heap_fragments_value,
		&button_done
	});

	const auto m0_core_free = chCoreStatus();
	text_label_m0_core_free_value.set(to_string_dec_uint(m0_core_free, 5));

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

	const auto graph_width = static_cast<int>(logger.capacity()) * bar_width;
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
	int y_raw = rect.bottom() - ((temperature - display_temp_min) * display_temp_scale);
	const auto y_limit = std::min(rect.bottom(), std::max(rect.top(), y_raw));
	return y_limit;
}

/* TemperatureView *******************************************************/

TemperatureView::TemperatureView(NavigationView& nav) {
	add_children({
		&text_title,
		&temperature_widget,
		&button_done,
	});

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
	const Coord left = (size().width() - config.row_width()) / 2;

	draw_legend(left, painter);
	draw_values(left, painter);
}

void RegistersWidget::draw_legend(const Coord left, Painter& painter) {
	const auto pos = screen_pos();

	for(size_t i=0; i<config.registers_count; i+=config.registers_per_row()) {
		const Point offset {
			left, static_cast<int>((i / config.registers_per_row()) * row_height)
		};

		const auto text = to_string_hex(i, config.legend_length());
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
			static_cast<int>(left + config.legend_width() + 8 + (i % config.registers_per_row()) * (config.value_width() + 8)),
			static_cast<int>((i / config.registers_per_row()) * row_height)
		};

		const auto value = reader(i);

		const auto text = to_string_hex(value, config.value_length());
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
	add_children({
		&text_title,
		&registers_widget,
		&button_update,
		&button_done,
	});

	button_update.on_select = [this](Button&){
		this->registers_widget.update();
	};
	button_done.on_select = [&nav](Button&){ nav.pop(); };

	registers_widget.set_parent_rect({ 0, 48, 240, 192 });

	text_title.set_parent_rect({
		(240 - static_cast<int>(title.size()) * 8) / 2, 16,
		static_cast<int>(title.size()) * 8, 16
	});
	text_title.set(title);
}

void RegistersView::focus() {
	button_done.focus();
}

/* ControlsSwitchesWidget ************************************************/

void ControlsSwitchesWidget::on_show() {
	display.fill_rectangle(
		screen_rect(),
		Color::black()
	);
}

bool ControlsSwitchesWidget::on_key(const KeyEvent key) {
	key_event_mask = 1 << toUType(key);
	return true;
}

void ControlsSwitchesWidget::paint(Painter& painter) {
	const std::array<Rect, 7> button_rects { {
		{ 64, 32, 16, 16 }, // Right
		{  0, 32, 16, 16 }, // Left
		{ 32, 64, 16, 16 }, // Down
		{ 32,  0, 16, 16 }, // Up
		{ 32, 32, 16, 16 }, // Select
		{ 16, 96, 16, 16 }, // Encoder phase 0
		{ 48, 96, 16, 16 }, // Encoder phase 1
	} };
	const auto pos = screen_pos();
	auto switches_raw = control::debug::switches();
	auto switches_debounced = get_switches_state().to_ulong();
	auto switches_event = key_event_mask;

	for(const auto r : button_rects) {
		const auto c =
			((switches_event & 1) ?
				Color::red() :
				((switches_debounced & 1) ?
					Color::green() :
					((switches_raw & 1) ?
						Color::yellow() :
						Color::blue()
					)
				)
			);
		painter.fill_rectangle(r + pos, c);
		switches_raw >>= 1;
		switches_debounced >>= 1;
		switches_event >>= 1;
	}
}

void ControlsSwitchesWidget::on_frame_sync() {
	set_dirty();
}

/* DebugControlsView *****************************************************/

DebugControlsView::DebugControlsView(NavigationView& nav) {
	add_children({
		&text_title,
		&switches_widget,
		&button_done,
	});

	button_done.on_select = [&nav](Button&){ nav.pop(); };
}

void DebugControlsView::focus() {
	switches_widget.focus();
}

/* DebugPeripheralsMenuView **********************************************/

DebugPeripheralsMenuView::DebugPeripheralsMenuView(NavigationView& nav) {
	add_items({
		{ "RFFC5072",    ui::Color::white(),	nullptr,	[&nav](){ nav.push<RegistersView>(
			"RFFC5072", RegistersWidgetConfig { 31, 16 },
			[](const size_t register_number) { return radio::debug::first_if::register_read(register_number); }
		); } },
		{ "MAX2837",     ui::Color::white(),	nullptr,	[&nav](){ nav.push<RegistersView>(
			"MAX2837", RegistersWidgetConfig { 32, 10 },
			[](const size_t register_number) { return radio::debug::second_if::register_read(register_number); }
		); } },
		{ "Si5351C",     ui::Color::white(),	nullptr,	[&nav](){ nav.push<RegistersView>(
			"Si5351C", RegistersWidgetConfig { 96, 8 },
			[](const size_t register_number) { return portapack::clock_generator.read_register(register_number); }
		); } },
		{ audio::debug::codec_name(), ui::Color::white(),	nullptr,	[&nav](){ nav.push<RegistersView>(
			audio::debug::codec_name(), RegistersWidgetConfig { audio::debug::reg_count(), audio::debug::reg_bits() },
			[](const size_t register_number) { return audio::debug::reg_read(register_number); }
		); } },
	});
	on_left = [&nav](){ nav.pop(); };
}

/* DebugMenuView *********************************************************/

DebugMenuView::DebugMenuView(NavigationView& nav) {
	add_items({
		{ "Memory", 		ui::Color::white(),	nullptr,	[&nav](){ nav.push<DebugMemoryView>(); } },
		//{ "Radio State",	ui::Color::white(),	nullptr,	[&nav](){ nav.push<NotImplementedView>(); } },
		{ "SD Card",		ui::Color::white(),	nullptr,	[&nav](){ nav.push<SDCardDebugView>(); } },
		{ "Peripherals",	ui::Color::white(),	nullptr,	[&nav](){ nav.push<DebugPeripheralsMenuView>(); } },
		{ "Temperature",	ui::Color::white(),	nullptr,	[&nav](){ nav.push<TemperatureView>(); } },
		{ "Controls",		ui::Color::white(),	nullptr,	[&nav](){ nav.push<DebugControlsView>(); } },	});
	on_left = [&nav](){ nav.pop(); };
}

/*DebugLCRView::DebugLCRView(NavigationView& nav, std::string lcr_string) {
	
	std::string debug_text;
	
	add_children({
		&console,
		&button_exit
	});

	for(const auto c : lcr_string) {
		if ((c < 32) || (c > 126))
			debug_text += "[" + to_string_dec_uint(c) + "]";
		else
			debug_text += c;
	}
	
	debug_text += "\n\n";
	debug_text += "Length: " + to_string_dec_uint(lcr_string.length()) + '\n';
	debug_text += "Checksum: " + to_string_dec_uint(lcr_string.back()) + '\n';
	
	console.write(debug_text);
	
	button_exit.on_select = [this, &nav](Button&){
		nav.pop();
	};
}
	
void DebugLCRView::focus() {
	button_exit.focus();
}*/

} /* namespace ui */
