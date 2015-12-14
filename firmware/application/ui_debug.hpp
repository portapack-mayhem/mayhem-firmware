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

#ifndef __UI_DEBUG_H__
#define __UI_DEBUG_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"

#include "rffc507x.hpp"
#include "max2837.hpp"
#include "portapack.hpp"

#include <functional>
#include <utility>

namespace ui {

class DebugMemoryView : public View {
public:
	DebugMemoryView(NavigationView& nav);

	void focus() override;

private:
	Text text_title {
		{ 96, 96, 48, 16 },
		"Memory",
	};

	Text text_label_m0_free {
		{ 0, 128, 104, 16 },
		"M0 Free Bytes",
	};

	Text text_label_m0_free_value {
		{ 200, 128, 40, 16 },
	};

	Text text_label_m0_heap_fragmented_free {
		{ 0, 144, 184, 16 },
		"M0 Heap Fragmented Free",
	};

	Text text_label_m0_heap_fragmented_free_value {
		{ 200, 144, 40, 16 },
	};

	Text text_label_m0_heap_fragments {
		{ 0, 160, 136, 16 },
		"M0 Heap Fragments",
	};

	Text text_label_m0_heap_fragments_value {
		{ 200, 160, 40, 16 },
	};

	Button button_done {
		{ 72, 192, 96, 24 },
		"Done"
	};
};

struct RegistersWidgetConfig {
	size_t registers_count;
	size_t legend_length;
	size_t value_length;
	size_t registers_per_row;

	constexpr Dim legend_width() const {
		return legend_length * 8;
	}

	constexpr Dim value_width() const {
		return value_length * 8;
	}

	constexpr size_t registers_row_length() const {
		return (registers_per_row * (value_length + 1)) - 1;
	}

	constexpr Dim registers_row_width() const {
		return registers_row_length() * 8;
	}

	constexpr size_t rows() const {
		return registers_count / registers_per_row;
	}
};

class RegistersWidget : public Widget {
public:
	RegistersWidget(
		RegistersWidgetConfig&& config,
		std::function<uint32_t(const size_t register_number)>&& reader
	);

	void update();

	void paint(Painter& painter) override;

private:
	const RegistersWidgetConfig config;
	const std::function<uint32_t(const size_t register_number)> reader;

	static constexpr Dim row_height = 16;

	void draw_legend(Painter& painter);
	void draw_values(Painter& painter);
};

class RegistersView : public View {
public:
	RegistersView(
		NavigationView& nav,
		const std::string& title,
		RegistersWidgetConfig&& config,
		std::function<uint32_t(const size_t register_number)>&& reader
	);

	void focus();

private:
	Text text_title;

	RegistersWidget registers_widget;

	Button button_update {
		{ 16, 256, 96, 24 },
		"Update"
	};

	Button button_done {
		{ 128, 256, 96, 24 },
		"Done"
	};
};

class DebugMenuView : public MenuView {
public:
	DebugMenuView(NavigationView& nav);
};

} /* namespace ui */

#endif/*__UI_DEBUG_H__*/
